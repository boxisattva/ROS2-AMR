from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'mock',
            default_value='true',
            description='Run serial_bridge in mock mode without real hardware'
        ),
        DeclareLaunchArgument(
            'serial_port',
            default_value='/dev/ttyUSB0',
            description='Serial port for ESP32'
        ),
        Node(
            package='amr_base_control',
            executable='serial_bridge',
            name='serial_bridge',
            output='screen',
            parameters=[{
                'serial_port': LaunchConfiguration('serial_port'),
                'mock': LaunchConfiguration('mock'),
                'baud_rate': 115200,
                'wheel_base': 0.148,
                'wheel_radius': 0.031
            }]
        ),
    ])