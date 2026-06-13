#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class ESP32Bridge : public rclcpp::Node
{
public:
    ESP32Bridge() : Node("esp32_bridge")
    {
        RCLCPP_INFO(this->get_logger(), "ESP32 Bridge node started");
        
        // Week 2: Implement serial communication with ESP32
        // Week 2: Subscribe to /cmd_vel, publish to /odom
        
        publisher_ = this->create_publisher<std_msgs::msg::String>("esp32_status", 10);
    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ESP32Bridge>());
    rclcpp::shutdown();
    return 0;
    }
