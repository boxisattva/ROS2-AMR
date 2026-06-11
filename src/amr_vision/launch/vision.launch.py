from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='amr_vision',
            executable='camera_node',
            name='camera_node',
            output='screen',
            parameters=[{
                'camera_id': 0,
                'publish_rate': 30.0
            }]
        ),
    ])
