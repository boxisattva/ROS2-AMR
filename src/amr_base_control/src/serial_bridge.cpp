#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <cmath>

// ============================================================================
// serial_bridge.cpp — ROS2 ↔ ESP32 UART bridge
// Подписывается: /cmd_vel → шлёт $VEL на ESP32
// Публикует: /odom (из $ODO или mock), tf odom→base_link, /motor_status
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SERIAL_BAUD B115200
#define SERIAL_TIMEOUT_MS 100
#define WATCHDOG_MS 500

class SerialBridge : public rclcpp::Node
{
public:
    SerialBridge() : Node("serial_bridge")
    {
        // --- Параметры ---
        this->declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
        this->declare_parameter<bool>("mock", false);
        this->declare_parameter<double>("wheel_base", 0.148);
        this->declare_parameter<double>("wheel_radius", 0.031);

        port_ = this->get_parameter("serial_port").as_string();
        mock_ = this->get_parameter("mock").as_bool();
        wheel_base_ = this->get_parameter("wheel_base").as_double();
        wheel_radius_ = this->get_parameter("wheel_radius").as_double();

        RCLCPP_INFO(this->get_logger(), "Serial bridge starting on %s (mock=%d)",
                    port_.c_str(), mock_);

        // --- Publishers и TF broadcaster ---
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
        motor_status_pub_ = this->create_publisher<std_msgs::msg::String>("motor_status", 10);
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        // --- Subscriber /cmd_vel ---
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10,
            std::bind(&SerialBridge::cmdVelCallback, this, std::placeholders::_1));

        // --- Открытие UART (только если не mock) ---
        if (!mock_) {
            serial_fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
            if (serial_fd_ < 0) {
                RCLCPP_ERROR(this->get_logger(), "Failed to open %s: %s",
                             port_.c_str(), strerror(errno));
                return;
            }

            struct termios tty;
            memset(&tty, 0, sizeof(tty));
            tcgetattr(serial_fd_, &tty);

            cfsetospeed(&tty, SERIAL_BAUD);
            cfsetispeed(&tty, SERIAL_BAUD);

            tty.c_cflag |= (CLOCAL | CREAD);
            tty.c_cflag &= ~PARENB;
            tty.c_cflag &= ~CSTOPB;
            tty.c_cflag &= ~CSIZE;
            tty.c_cflag |= CS8;
            tty.c_cflag &= ~CRTSCTS;

            tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            tty.c_oflag &= ~OPOST;

            tcsetattr(serial_fd_, TCSANOW, &tty);
            fcntl(serial_fd_, F_SETFL, 0);

            RCLCPP_INFO(this->get_logger(), "Serial port %s opened at 115200 baud", port_.c_str());
        } else {
            RCLCPP_INFO(this->get_logger(), "MOCK mode: no serial connection");
        }

        // --- Таймер чтения UART (100 Гц) ---
        read_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(10),
            std::bind(&SerialBridge::readSerial, this));

        // --- Watchdog таймер ---
        watchdog_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(WATCHDOG_MS),
            std::bind(&SerialBridge::checkWatchdog, this));
    }

    ~SerialBridge()
    {
        if (serial_fd_ >= 0) {
            close(serial_fd_);
        }
    }

private:
    // --- Параметры и состояние ---
    std::string port_;
    bool mock_ = false;
    double wheel_base_ = 0.148;
    double wheel_radius_ = 0.031;

    // Текущая поза робота в odom
    double x_ = 0.0;
    double y_ = 0.0;
    double theta_ = 0.0;

    // Последние скорости (для twist в odom)
    double last_v_ = 0.0;
    double last_w_ = 0.0;

    // Mock-интегрирование по времени
    rclcpp::Time last_integrate_time_;
    bool first_integrate_ = true;

    // --- ROS2 объекты ---
    int serial_fd_ = -1;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr motor_status_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    rclcpp::TimerBase::SharedPtr read_timer_;
    rclcpp::TimerBase::SharedPtr watchdog_timer_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

    std::string rx_buffer_;
    bool motors_active_ = false;
    rclcpp::Time last_cmd_time_;

    // --- CRC-8 (XOR) ---
    uint8_t calcCRC(const std::string& data)
    {
        uint8_t crc = 0;
        for (char c : data) {
            crc ^= (uint8_t)c;
        }
        return crc;
    }

    // --- Форматирование кадра: $PAYLOAD*CRC\n ---
    std::string formatFrame(const std::string& payload)
    {
        char crc_str[3];
        snprintf(crc_str, sizeof(crc_str), "%02X", calcCRC(payload));
        return "$" + payload + "*" + crc_str + "\n";
    }

    // --- Обрезка лишних нулей у float ---
    std::string formatFloat(double value)
    {
        std::string s = std::to_string(value);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') {
            s += '0';
        }
        return s;
    }

    // --- Отправка в UART ---
    void sendSerial(const std::string& frame)
    {
        if (serial_fd_ >= 0) {
            write(serial_fd_, frame.c_str(), frame.length());
            RCLCPP_DEBUG(this->get_logger(), "TX: %s", frame.c_str());
        } else {
            RCLCPP_DEBUG(this->get_logger(), "MOCK TX: %s", frame.c_str());
        }
    }

    // --- Callback /cmd_vel ---
    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        double v = msg->linear.x;
        double w = msg->angular.z;

        // В mock-режиме интегрируем pose прямо из команды управления
        if (mock_) {
            rclcpp::Time now = this->now();
            if (first_integrate_) {
                last_integrate_time_ = now;
                first_integrate_ = false;
            }
            double dt = (now - last_integrate_time_).seconds();
            integrateAndPublish(v, w, dt, now);
            last_integrate_time_ = now;
        }

        // Формируем $VEL,<v>,<w>*CRC
        std::string payload = "VEL," + formatFloat(v) + "," + formatFloat(w);
        std::string frame = formatFrame(payload);
        sendSerial(frame);

        // Сохраняем время последней команды для watchdog
        motors_active_ = true;
        last_cmd_time_ = this->now();

        RCLCPP_INFO(this->get_logger(), "cmd_vel: v=%.2f, w=%.2f → %s", v, w, payload.c_str());
    }

    // --- Интегрирование дифференциального привода ---
    void integrateAndPublish(double v, double w, double dt, const rclcpp::Time& stamp)
    {
        if (dt <= 0.0) {
            return;
        }

        // Сохраняем скорости для twist в odom
        last_v_ = v;
        last_w_ = w;

        // Интегрируем pose
        double delta_x = v * std::cos(theta_) * dt;
        double delta_y = v * std::sin(theta_) * dt;
        double delta_theta = w * dt;

        x_ += delta_x;
        y_ += delta_y;
        theta_ += delta_theta;

        // Нормализация угла в [-pi, pi]
        while (theta_ > M_PI) theta_ -= 2.0 * M_PI;
        while (theta_ < -M_PI) theta_ += 2.0 * M_PI;

        publishOdometry(stamp);
    }

    // --- Публикация /odom + tf odom→base_link ---
    void publishOdometry(const rclcpp::Time& stamp)
    {
        nav_msgs::msg::Odometry odom;
        odom.header.stamp = stamp;
        odom.header.frame_id = "odom";
        odom.child_frame_id = "base_link";

        odom.pose.pose.position.x = x_;
        odom.pose.pose.position.y = y_;
        odom.pose.pose.position.z = 0.0;

        tf2::Quaternion q;
        q.setRPY(0.0, 0.0, theta_);
        odom.pose.pose.orientation.x = q.x();
        odom.pose.pose.orientation.y = q.y();
        odom.pose.pose.orientation.z = q.z();
        odom.pose.pose.orientation.w = q.w();

        odom.twist.twist.linear.x = last_v_;
        odom.twist.twist.angular.z = last_w_;

        odom_pub_->publish(odom);

        geometry_msgs::msg::TransformStamped tf;
        tf.header.stamp = stamp;
        tf.header.frame_id = "odom";
        tf.child_frame_id = "base_link";
        tf.transform.translation.x = x_;
        tf.transform.translation.y = y_;
        tf.transform.translation.z = 0.0;
        tf.transform.rotation = odom.pose.pose.orientation;

        tf_broadcaster_->sendTransform(tf);
    }

    // --- Чтение из UART ---
    void readSerial()
    {
        if (serial_fd_ < 0) {
            return;
        }

        char buf[256];
        int n = read(serial_fd_, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            rx_buffer_ += buf;
            processBuffer();
        }
    }

    // --- Парсинг входящих кадров ---
    void processBuffer()
    {
        size_t start = rx_buffer_.find('$');
        while (start != std::string::npos) {
            size_t star = rx_buffer_.find('*', start);
            if (star == std::string::npos) break;

            size_t nl = rx_buffer_.find('\n', star);
            if (nl == std::string::npos) break;

            std::string payload = rx_buffer_.substr(start + 1, star - start - 1);
            std::string crc_str = rx_buffer_.substr(star + 1, 2);

            uint8_t expected_crc = calcCRC(payload);
            uint8_t rx_crc = (uint8_t)strtol(crc_str.c_str(), nullptr, 16);

            if (expected_crc == rx_crc) {
                dispatchRx(payload);
            } else {
                RCLCPP_WARN(this->get_logger(), "Bad CRC: %s", payload.c_str());
            }

            rx_buffer_.erase(0, nl + 1);
            start = rx_buffer_.find('$');
        }

        if (rx_buffer_.length() > 1024) {
            rx_buffer_.clear();
        }
    }

    // --- Обработка принятого кадра ---
    void dispatchRx(const std::string& payload)
    {
        RCLCPP_DEBUG(this->get_logger(), "RX: %s", payload.c_str());

        if (payload.substr(0, 4) == "ACK,") {
            std_msgs::msg::String msg;
            msg.data = payload;
            motor_status_pub_->publish(msg);
            RCLCPP_INFO(this->get_logger(), "Motor status: %s", payload.c_str());
        }
        else if (payload.substr(0, 4) == "STA,") {
            std_msgs::msg::String msg;
            msg.data = payload;
            motor_status_pub_->publish(msg);
            RCLCPP_WARN(this->get_logger(), "Status: %s", payload.c_str());
        }
        else if (payload.substr(0, 4) == "ODO,") {
            parseOdometry(payload);
        }
        else if (payload.substr(0, 5) == "PONG,") {
            RCLCPP_INFO(this->get_logger(), "ESP32: %s", payload.c_str());
        }
    }

    // --- Парсинг одометрии из $ODO,<v_left>,<v_right>,<dt_ms> ---
    void parseOdometry(const std::string& payload)
    {
        size_t p1 = payload.find(',');
        size_t p2 = payload.find(',', p1 + 1);
        size_t p3 = payload.find(',', p2 + 1);

        if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) {
            return;
        }

        double v_left = std::stod(payload.substr(p1 + 1, p2 - p1 - 1));
        double v_right = std::stod(payload.substr(p2 + 1, p3 - p2 - 1));
        double dt_ms = std::stod(payload.substr(p3 + 1));
        double dt = dt_ms / 1000.0;

        // Дифференциальный привод
        double v = (v_left + v_right) / 2.0;
        double w = (v_right - v_left) / wheel_base_;

        integrateAndPublish(v, w, dt, this->now());
    }

    // --- Watchdog ---
    void checkWatchdog()
    {
        if (motors_active_ && (this->now() - last_cmd_time_).seconds() > (WATCHDOG_MS / 1000.0)) {
            RCLCPP_WARN(this->get_logger(), "Watchdog: sending STO");
            sendSerial(formatFrame("STO"));
            motors_active_ = false;
        }
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SerialBridge>());
    rclcpp::shutdown();
    return 0;
}