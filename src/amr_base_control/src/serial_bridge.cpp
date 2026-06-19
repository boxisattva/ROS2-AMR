#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/string.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <algorithm>

class SerialBridge : public rclcpp::Node
{
public:
    SerialBridge() : Node("serial_bridge"), fd_(-1)
    {
        this->declare_parameter<std::string>("port", "/dev/ttyUSB0");
        this->declare_parameter<int>("baud", 115200);
        this->declare_parameter<bool>("mock", false);

        port_ = this->get_parameter("port").as_string();
        baud_ = this->get_parameter("baud").as_int();
        mock_ = this->get_parameter("mock").as_bool();

        status_pub_ = this->create_publisher<std_msgs::msg::String>("/motor_status", 10);
        
        cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 10,
            std::bind(&SerialBridge::cmdVelCallback, this, std::placeholders::_1));

        if (!mock_) {
            if (!openSerial()) {
                RCLCPP_ERROR(this->get_logger(), 
                    "Failed to open serial port %s. Running in degraded mode.", port_.c_str());
                publishStatus("ERR: serial port not open");
            }
        } else {
            RCLCPP_WARN(this->get_logger(), "MOCK MODE: output logged, not written to port");
        }

        read_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(50),
            std::bind(&SerialBridge::readSerial, this));

        RCLCPP_INFO(this->get_logger(), 
            "SerialBridge: port=%s baud=%d mock=%s", 
            port_.c_str(), baud_, mock_ ? "true" : "false");
    }

    ~SerialBridge()
    {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

private:
    bool openSerial()
    {
        fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (fd_ < 0) {
            return false;
        }

        fcntl(fd_, F_SETFL, 0);

        struct termios tty;
        if (tcgetattr(fd_, &tty) != 0) {
            close(fd_);
            fd_ = -1;
            return false;
        }

        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);

        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag |= CREAD | CLOCAL;

        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_oflag &= ~OPOST;

        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 0;

        tcflush(fd_, TCIFLUSH);

        if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
            close(fd_);
            fd_ = -1;
            return false;
        }
        return true;
    }

    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        // FIX: std::clamp требует одинаковый тип. msg->linear.x — double, литералы тоже double.
        double linear_x = std::clamp(msg->linear.x, -0.5, 0.5);
        double angular_z = std::clamp(msg->angular.z, -1.0, 1.0);

        std::stringstream ss;
        ss << "VEL," << std::fixed << std::setprecision(2) << linear_x << "," << angular_z;
        std::string payload = ss.str();

        uint8_t crc = calculateCRC(payload);
        std::string frame = "$" + payload + "*" + toHex(crc) + "\n";

        if (mock_) {
            RCLCPP_INFO(this->get_logger(), "[MOCK] TX: %s", frame.c_str());
            publishStatus("MOCK: " + frame);
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            if (fd_ >= 0) {
                ssize_t n = write(fd_, frame.c_str(), frame.length());
                if (n < 0) {
                    RCLCPP_ERROR(this->get_logger(), "Serial write failed");
                    publishStatus("ERR: serial write failed");
                } else {
                    RCLCPP_DEBUG(this->get_logger(), "TX: %s", frame.c_str());
                }
            } else {
                publishStatus("ERR: serial port not open");
            }
        }
    }

    void readSerial()
    {
        if (fd_ < 0 || mock_) return;

        char buf[256];
        ssize_t n = read(fd_, buf, sizeof(buf));
        if (n > 0) {
            rx_buffer_.append(buf, n);
            
            size_t pos;
            while ((pos = rx_buffer_.find('\n')) != std::string::npos) {
                std::string line = rx_buffer_.substr(0, pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                rx_buffer_.erase(0, pos + 1);

                if (!line.empty()) {
                    RCLCPP_INFO(this->get_logger(), "RX: %s", line.c_str());
                    publishStatus(line);
                }
            }
        }
    }

    uint8_t calculateCRC(const std::string& data)
    {
        uint8_t crc = 0;
        for (char c : data) {
            crc ^= static_cast<uint8_t>(c);
        }
        return crc;
    }

    std::string toHex(uint8_t v)
    {
        const char* hex = "0123456789ABCDEF";
        std::string s;
        s += hex[v >> 4];
        s += hex[v & 0x0F];
        return s;
    }

    void publishStatus(const std::string& text)
    {
        auto msg = std_msgs::msg::String();
        msg.data = text;
        status_pub_->publish(msg);
    }

    std::string port_;
    int baud_;
    bool mock_;
    int fd_;
    std::mutex mutex_;
    std::string rx_buffer_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::TimerBase::SharedPtr read_timer_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SerialBridge>();
    if (node) {
        rclcpp::spin(node);
    }
    rclcpp::shutdown();
    return 0;
}
