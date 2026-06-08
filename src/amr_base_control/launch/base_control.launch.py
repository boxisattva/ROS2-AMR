from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='amr_base_control',
            executable='esp32_bridge',
            name='esp32_bridge',
            output='screen',
            parameters=[{
                'serial_port': '/dev/ttyUSB0',
                'baud_rate': 115200
            }]
        ),
    ])

