import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command


def generate_launch_description():
    # Путь к URDF (универсальный, через home)
    urdf_path = os.path.join(
        os.path.expanduser('~'),
        'ROS2-AMR',
        'src',
        'amr_description',
        'urdf',
        'amr.urdf.xacro'
    )

    return LaunchDescription([
        # 1. Robot State Publisher (URDF)
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            parameters=[{
                'robot_description': Command(['xacro ', urdf_path])
            }]
        ),

        # 2. RPLIDAR A1
        Node(
            package='rplidar_ros',
            executable='rplidar_node',
            name='rplidar',
            parameters=[{
                'serial_port': '/dev/ttyUSB1',
                'serial_baudrate': 115200,
                'frame_id': 'laser_frame',
                'angle_compensate': True,
                'scan_mode': 'Standard'
            }]
        ),

        # 3. TF: base_link -> laser_frame
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=['0.06', '0', '0.08', '0', '0', '0', 'base_link', 'laser_frame'],
            name='laser_tf'
        ),

        # 4. Odometry (dead reckoning)
        Node(
            package='amr_base_control',
            executable='odometry_publisher',
            name='odometry'
        ),

        # 5. serial_bridge (реальный ESP32)
        Node(
            package='amr_base_control',
            executable='serial_bridge',
            name='serial_bridge',
            parameters=[{'mock': False}]
        ),
    ])
