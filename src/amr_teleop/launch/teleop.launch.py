from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='amr_teleop',
            executable='keyboard_control',
            name='keyboard_control',
            output='screen',
            remappings=[('/cmd_vel', '/amr/cmd_vel')]
        ),
    ])
