from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        # xterm запускает keyboard_control в отдельном окне терминала.
        # Почему: ros2 launch не предоставляет интерактивный stdin,
        # поэтому termios падает с "Inappropriate ioctl for device".
        # xterm дает настоящий TTY, и клавиатура работает.
        #
        # ВАЖНО: prefix должен быть строкой, а не списком.
        # Пробел в конце строки нужен, чтобы отделить prefix от самой команды.
        Node(
            package='amr_teleop',
            executable='keyboard_control',
            name='keyboard_control',
            output='screen',
            prefix='xterm -hold -e ',
        ),
    ])