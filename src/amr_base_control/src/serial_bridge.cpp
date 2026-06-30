#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// ============================================================================
// serial_bridge.cpp — ROS2 ↔ ESP32 UART bridge
// Подписывается: /cmd_vel → шлёт $VEL на ESP32
// Публикует: /odom (из $ODO), /motor_status (из $ACK/$STA)
// ============================================================================

#define SERIAL_BAUD B115200
#define SERIAL_TIMEOUT_MS 100
#define WATCHDOG_MS 500

class SerialBridge : public rclcpp::Node
{
public:
    SerialBridge() : Node("serial_bridge")
    {
        // Параметры
        this->declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
        this->declare_parameter<bool>("mock", false);
        
        std::string port = this->get_parameter("serial_port").as_string();
        bool mock = this->get_parameter("mock").as_bool();
        
        RCLCPP_INFO(this->get_logger(), "Serial bridge starting on %s", port.c_str());
        
        // Publishers
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
        motor_status_pub_ = this->create_publisher<std_msgs::msg::String>("motor_status", 10);
        
        // Subscriber
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10,
            std::bind(&SerialBridge::cmdVelCallback, this, std::placeholders::_1));
        
        if (!mock) {
            // Открытие UART
            serial_fd_ = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
            if (serial_fd_ < 0) {
                RCLCPP_ERROR(this->get_logger(), "Failed to open %s: %s", port.c_str(), strerror(errno));
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
            
            RCLCPP_INFO(this->get_logger(), "Serial port %s opened at 115200 baud", port.c_str());
        } else {
            RCLCPP_INFO(this->get_logger(), "MOCK mode: no serial connection");
        }
        
        // Таймер для чтения UART (100 Гц)
        read_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(10),
            std::bind(&SerialBridge::readSerial, this));
        
        // Watchdog таймер
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
    int serial_fd_ = -1;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr motor_status_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    rclcpp::TimerBase::SharedPtr read_timer_;
    rclcpp::TimerBase::SharedPtr watchdog_timer_;
    
    std::string rx_buffer_;
    bool motors_active_ = false;
    rclcpp::Time last_cmd_time_;
    
    // CRC-8 (XOR)
    uint8_t calcCRC(const std::string& data)
    {
        uint8_t crc = 0;
        for (char c : data) {
            crc ^= (uint8_t)c;
        }
        return crc;
    }
    
    // Форматирование кадра: $PAYLOAD*CRC\n
    std::string formatFrame(const std::string& payload)
    {
        char crc_str[3];
        snprintf(crc_str, sizeof(crc_str), "%02X", calcCRC(payload));
        return "$" + payload + "*" + crc_str + "\n";
    }
    
    // Отправка в UART
    void sendSerial(const std::string& frame)
    {
        if (serial_fd_ >= 0) {
            write(serial_fd_, frame.c_str(), frame.length());
            RCLCPP_DEBUG(this->get_logger(), "TX: %s", frame.c_str());
        } else {
            RCLCPP_DEBUG(this->get_logger(), "MOCK TX: %s", frame.c_str());
        }
    }
    
    // Callback /cmd_vel
    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        float v = msg->linear.x;
        float w = msg->angular.z;
        
        // Формируем $VEL,<v>,<w>*CRC
        std::string payload = "VEL," + std::to_string(v) + "," + std::to_string(w);
        // Убираем лишние нули после запятой
        payload.erase(payload.find_last_not_of('0') + 1, std::string::npos);
        if (payload.back() == '.') payload += '0';
        
        std::string frame = formatFrame(payload);
        sendSerial(frame);
        
        motors_active_ = true;
        last_cmd_time_ = this->now();
        
        RCLCPP_INFO(this->get_logger(), "cmd_vel: v=%.2f, w=%.2f → %s", v, w, payload.c_str());
    }
    
    // Чтение из UART
    void readSerial()
    {
        if (serial_fd_ < 0) return;
        
        char buf[256];
        int n = read(serial_fd_, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            rx_buffer_ += buf;
            processBuffer();
        }
    }
    
    // Парсинг входящих кадров
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
            
            // Проверка CRC
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
        
        // Очистка старого буфера
        if (rx_buffer_.length() > 1024) {
            rx_buffer_.clear();
        }
    }
    
    // Обработка принятого кадра
    void dispatchRx(const std::string& payload)
    {
        RCLCPP_DEBUG(this->get_logger(), "RX: %s", payload.c_str());
        
        if (payload.substr(0, 4) == "ACK,") {
            // $ACK,OK или $ACK,ERR,...
            std_msgs::msg::String msg;
            msg.data = payload;
            motor_status_pub_->publish(msg);
            RCLCPP_INFO(this->get_logger(), "Motor status: %s", payload.c_str());
        }
        else if (payload.substr(0, 4) == "STA,") {
            // $STA,ESTOP,TRIGGERED и т.д.
            std_msgs::msg::String msg;
            msg.data = payload;
            motor_status_pub_->publish(msg);
            RCLCPP_WARN(this->get_logger(), "Status: %s", payload.c_str());
        }
        else if (payload.substr(0, 4) == "ODO,") {
            // $ODO,<v_left>,<v_right>,<dt_ms>
            parseOdometry(payload);
        }
        else if (payload.substr(0, 5) == "PONG,") {
            RCLCPP_INFO(this->get_logger(), "ESP32: %s", payload.c_str());
        }
    }
    
    // Парсинг одометрии
    void parseOdometry(const std::string& payload)
    {
        // $ODO,<v_left>,<v_right>,<dt_ms>
        size_t p1 = payload.find(',');
        size_t p2 = payload.find(',', p1 + 1);
        size_t p3 = payload.find(',', p2 + 1);
        
        if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) {
            return;
        }
        
        float v_left = std::stof(payload.substr(p1 + 1, p2 - p1 - 1));
        float v_right = std::stof(payload.substr(p2 + 1, p3 - p2 - 1));
        // float dt_ms = std::stof(payload.substr(p3 + 1));
        
        // Публикация Odometry (упрощённая, без интеграции позиции)
        nav_msgs::msg::Odometry odom;
        odom.header.stamp = this->now();
        odom.header.frame_id = "odom";
        odom.child_frame_id = "base_link";
        
        // Средняя скорость
        float v = (v_left + v_right) / 2.0f;
        odom.twist.twist.linear.x = v;
        odom.twist.twist.angular.z = (v_right - v_left) / 0.148f;  // WHEEL_BASE
        
        odom_pub_->publish(odom);
    }
    
    // Watchdog
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
