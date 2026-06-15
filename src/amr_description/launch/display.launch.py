from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
import os


def generate_launch_description():
    # Путь к robot_state_publisher.launch.py
    rsp_launch = os.path.join(
        '/ros2_ws/src/amr_description/launch',
        'robot_state_publisher.launch.py'
    )

    return LaunchDescription([
        # 1. Запускаем robot_state_publisher (TF + robot_description)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(rsp_launch)
        ),

        # 2. Запускаем joint_state_publisher_gui (ручное кручение колёс)
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui',
            name='joint_state_publisher_gui',
            output='screen'
        ),

        # 3. Запускаем RViz (визуализация)
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=['-d', '/ros2_ws/src/amr_description/config/navbot.rviz']
        ),
    ])
