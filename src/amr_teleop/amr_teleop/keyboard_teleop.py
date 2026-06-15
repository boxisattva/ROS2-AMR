#!/usr/bin/env python3
"""
keyboard_teleop.py — управление роботом с клавиатуры через ROS2

Архитектура:
- Таймер 10 Гц публикует Twist в /cmd_vel
- Чтение клавиши через termios + select (non-blocking, без внешних библиотек)
- Toggle mode: W/A/S/D = задать скорость, SPACE = стоп, Q = выход
- Safety: при выходе публикуется Twist(0,0) — робот останавливается
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

import sys
import termios      # управление терминалом (raw mode)
import tty          # установка raw mode
import select       # non-blocking проверка ввода


class KeyboardTeleop(Node):
    def __init__(self):
        super().__init__('keyboard_teleop')

        # ==================== ПАРАМЕТРЫ ROS2 ====================
        # Можно переопределить при запуске:
        # ros2 run amr_teleop keyboard_teleop --ros-args -p max_linear:=0.5
        self.declare_parameter('max_linear', 0.3)    # м/с
        self.declare_parameter('max_angular', 1.0)   # рад/с
        
        self.max_linear = self.get_parameter('max_linear').value
        self.max_angular = self.get_parameter('max_angular').value

        # ==================== PUBLISHER ====================
        # /cmd_vel — стандартный топик для скорости в ROS2
        # Nav2, serial_bridge, всё что угодно — слушают его
        self.pub = self.create_publisher(Twist, '/cmd_vel', 10)

        # ==================== ТАЙМЕР 10 Гц ====================
        # Публикуем Twist каждые 0.1 сек, даже если клавиша не нажата
        # Это нужно для serial_bridge (он ждёт постоянный поток)
        self.timer = self.create_timer(0.1, self.timer_callback)

        # ==================== СОСТОЯНИЕ СКОРОСТИ ====================
        # Текущая целевая скорость. Меняется клавишами, сбрасывается пробелом
        self.linear_x = 0.0
        self.angular_z = 0.0

        # ==================== НАСТРОЙКА ТЕРМИНАЛА ====================
        # Сохраняем текущие настройки, чтобы восстановить при выходе
        # setcbreak: символы передаются сразу (без Enter), но Ctrl+C работает
        self.settings = termios.tcgetattr(sys.stdin)
        tty.setcbreak(sys.stdin.fileno())

        self.get_logger().info('=== Keyboard Teleop ===')
        self.get_logger().info('W = forward | S = backward | A = left | D = right')
        self.get_logger().info('SPACE = stop | Q = quit')
        self.get_logger().info('Toggle mode: press W once → robot moves, SPACE → stop')

    def timer_callback(self):
        """Вызывается 10 раз в секунду. Читает клавишу (если есть) и публикует Twist."""
        
        # select([stdin], [], [], 0.0) — проверяет, есть ли данные в stdin
        # timeout=0.0 означает "не ждать, проверить и вернуться сразу"
        rlist, _, _ = select.select([sys.stdin], [], [], 0.0)
        
        if rlist:
            # Читаем 1 символ (блокировки не будет, т.к. select подтвердил наличие данных)
            key = sys.stdin.read(1)
            
            # ==================== ОБРАБОТКА КЛАВИШ ====================
            if key in ('w', 'W'):
                self.linear_x = self.max_linear
                self.angular_z = 0.0
                self.get_logger().info('→ Forward')
                
            elif key in ('s', 'S'):
                self.linear_x = -self.max_linear
                self.angular_z = 0.0
                self.get_logger().info('→ Backward')
                
            elif key in ('a', 'A'):
                self.linear_x = 0.0
                self.angular_z = self.max_angular
                self.get_logger().info('→ Rotate left')
                
            elif key in ('d', 'D'):
                self.linear_x = 0.0
                self.angular_z = -self.max_angular
                self.get_logger().info('→ Rotate right')
                
            elif key == ' ':
                self.linear_x = 0.0
                self.angular_z = 0.0
                self.get_logger().info('→ STOP')
                
            elif key in ('q', 'Q'):
                self.get_logger().info('→ Quit')
                self.stop_and_exit()
                return

        # ==================== ПУБЛИКАЦИЯ /cmd_vel ====================
        # Публикуем даже если клавиша не нажата — serial_bridge ждёт поток
        msg = Twist()
        msg.linear.x = self.linear_x
        msg.angular.z = self.angular_z
        self.pub.publish(msg)

    def stop_and_exit(self):
        """Остановка робота и завершение узла."""
        self.publish_zero_twist()
        # Восстанавливаем терминал
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        rclpy.shutdown()

    def publish_zero_twist(self):
        """Emergency stop: публикуем нулевую скорость."""
        msg = Twist()
        msg.linear.x = 0.0
        msg.angular.z = 0.0
        self.pub.publish(msg)
        self.get_logger().info('Emergency stop published')

    def destroy_node(self):
        """Вызывается при штатном завершении. Восстанавливаем терминал."""
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        self.publish_zero_twist()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = KeyboardTeleop()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # Гарантированная остановка при любом выходе (Ctrl+C, Exception, etc.)
        node.publish_zero_twist()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
