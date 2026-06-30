#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import sys
import termios
import tty
import select


class KeyboardControl(Node):
    def __init__(self):
        super().__init__('keyboard_control')
        # Week 3: унифицированный топик /cmd_vel (не /amr/cmd_vel)
        self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)
        self.get_logger().info('Keyboard control started. Use WASD, Space to stop, Q to quit')
        self.settings = termios.tcgetattr(sys.stdin)

    def get_key(self):
        """Non-blocking key read with raw terminal mode"""
        tty.setraw(sys.stdin.fileno())
        # select позволяет таймаут и не блокировать loop
        ready, _, _ = select.select([sys.stdin], [], [], 0.1)
        if ready:
            key = sys.stdin.read(1)
        else:
            key = ''
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key

    def run(self):
        """Main control loop - FIXED: now inside class"""
        twist = Twist()
        try:
            while rclpy.ok():
                key = self.get_key()
                
                if key == 'w' or key == 'W':
                    twist.linear.x = 0.5
                    twist.angular.z = 0.0
                elif key == 's' or key == 'S':
                    twist.linear.x = -0.5
                    twist.angular.z = 0.0
                elif key == 'a' or key == 'A':
                    twist.linear.x = 0.0
                    twist.angular.z = 0.5
                elif key == 'd' or key == 'D':
                    twist.linear.x = 0.0
                    twist.angular.z = -0.5
                elif key == ' ':
                    # Space = полный стоп
                    twist.linear.x = 0.0
                    twist.angular.z = 0.0
                elif key == 'q' or key == 'Q':
                    break
                elif key == '':
                    # Нет нажатия - отправляем стоп (иначе watchdog на ESP32 сработает)
                    twist.linear.x = 0.0
                    twist.angular.z = 0.0
                    continue  # Не публикуем лишний раз
                
                self.publisher_.publish(twist)
                self.get_logger().info(f'cmd: linear={twist.linear.x:.2f}, angular={twist.angular.z:.2f}')
                
        finally:
            # Гарантированный стоп при выходе
            twist.linear.x = 0.0
            twist.angular.z = 0.0
            self.publisher_.publish(twist)
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)


def main(args=None):
    rclpy.init(args=args)
    node = KeyboardControl()
    try:
        node.run()
    except Exception as e:
        node.get_logger().error(f'Error: {e}')
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
