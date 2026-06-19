#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/string.hpp>

// C headers for serial port
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sstream>
#include <iomanip>
#include <string>

class SerialBridge : public rclcpp::Node
{
public:
    SerialBridge() : Node("serial_bridge"), fd_(-1)
    {
        // ==================== ПАРАМЕТРЫ ROS2 ====================
        // Параметры позволяют менять порт и скорость без перекомпиляции
        this->declare_parameter<std::string>("port", "/dev/ttyUSB0");
        this->declare_parameter<int>("baud", 115200);

        std::string port = this->get_parameter("port").as_string();
        int baud = this->get_parameter("baud").as_int();

        // ==================== ПОДПИСКА НА /cmd_vel ====================
        // Nav2 или teleop публикуют сюда Twist-сообщения
        cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel",
            10,  // queue size
            std::bind(&SerialBridge::cmdVelCallback, this, std::placeholders::_1));

        // ==================== ПУБЛИКАЦИЯ /motor_status ====================
        // Для диагностики: что отправлено, успешно ли
        status_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/motor_status",
            10);

        // ==================== ОТКРЫТИЕ SERIAL PORT ====================
        // O_RDWR = чтение+запись, O_NOCTTY = не делать терминал управляющим
        fd_ = open(port.c_str(), O_RDWR | O_NOCTTY);
        if (fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open %s", port.c_str());
            RCLCPP_ERROR(this->get_logger(), "Serial bridge will run in MOCK mode (logging only)");
            // Не выходим — работаем в режиме логирования без железа
            mock_mode_ = true;
        } else {
            mock_mode_ = false;
            configureSerial(baud);
        }

        RCLCPP_INFO(this->get_logger(), "SerialBridge: %s @ %d baud (mock=%s)",
                    port.c_str(), baud, mock_mode_ ? "true" : "false");
    }

    ~SerialBridge()
    {
        if (fd_ >= 0) {
            close(fd_);
            RCLCPP_INFO(this->get_logger(), "Serial port closed");
        }
    }

private:
    // ==================== НАСТРОЙКА TERMIOS ====================
    // 115200 baud, 8 data bits, no parity, 1 stop bit (8N1)
    void configureSerial(int baud)
    {
        struct termios tty;
        memset(&tty, 0, sizeof(tty));

        if (tcgetattr(fd_, &tty) != 0) {
            RCLCPP_ERROR(this->get_logger(), "tcgetattr failed");
            return;
        }

        // Скорость
        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);

        // 8N1: 8 data bits, no parity, 1 stop bit
        tty.c_cflag &= ~PARENB;        // no parity
        tty.c_cflag &= ~CSTOPB;        // 1 stop bit
        tty.c_cflag &= ~CSIZE;         // clear size mask
        tty.c_cflag |= CS8;            // 8 bits
        tty.c_cflag |= CREAD | CLOCAL; // enable read, ignore modem control

        // Raw mode: без обработки, без эхо
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_oflag &= ~OPOST;

        tcsetattr(fd_, TCSANOW, &tty);
        tcflush(fd_, TCIOFLUSH);
    }

    // ==================== CALLBACK /cmd_vel ====================
    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        float linear_x = msg->linear.x;
        float angular_z = msg->angular.z;

        // Ограничение скорости (safety)
        linear_x = std::clamp(linear_x, -0.5f, 0.5f);
        angular_z = std::clamp(angular_z, -1.0f, 1.0f);

        // Форматирование $VEL-команды
        std::stringstream ss;
        ss << "$VEL," << std::fixed << std::setprecision(2) << linear_x << "," << angular_z;
        std::string payload = ss.str().substr(1); // без '$' для CRC

        // Расчёт XOR checksum
        uint8_t crc = calculateCRC(payload);
        std::string cmd = "$" + payload + "*" + toHex(crc) + "\r\n";

        // Отправка в serial port (или лог в mock mode)
        if (!mock_mode_) {
            ssize_t written = write(fd_, cmd.c_str(), cmd.length());
            if (written < 0) {
                RCLCPP_ERROR(this->get_logger(), "Write to serial failed");
                publishStatus("ERR,WRITE_FAILED");
                return;
            }
        } else {
            // MOCK mode: просто логируем, что БЫЛО бы отправлено
            RCLCPP_INFO(this->get_logger(), "[MOCK] TX: %s", cmd.c_str());
        }

        // Публикация статуса
        publishStatus("OK," + cmd);

        RCLCPP_DEBUG(this->get_logger(), "TX: %s", cmd.c_str());
    }

    // ==================== CRC-8 (XOR) ====================
    uint8_t calculateCRC(const std::string& data)
    {
        uint8_t crc = 0;
        for (char c : data) {
            crc ^= static_cast<uint8_t>(c);
        }
        return crc;
    }

    // ==================== HEX конвертация ====================
    std::string toHex(uint8_t value)
    {
        const char* hex = "0123456789ABCDEF";
        std::string result;
        result += hex[value >> 4];   // старший полубайт
        result += hex[value & 0x0F]; // младший полубайт
        return result;
    }

    // ==================== ПУБЛИКАЦИЯ СТАТУСА ====================
    void publishStatus(const std::string& status)
    {
        std_msgs::msg::String msg;
        msg.data = status;
        status_pub_->publish(msg);
    }

    // ==================== ПОЛЯ ====================
    int fd_;                    // file descriptor serial port
    bool mock_mode_;            // true = нет железа, только логи
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
};

// ==================== MAIN ====================
int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SerialBridge>();

    if (!node) {
        RCLCPP_ERROR(rclcpp::get_logger("serial_bridge"), "Node creation failed");
        return 1;
    }

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
