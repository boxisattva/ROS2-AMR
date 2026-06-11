#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import sys
import termios
import tty


class KeyboardControl(Node):
    def __init__(self):
        super().__init__('keyboard_control')
        self.publisher_ = self.create_publisher(Twist, '/amr/cmd_vel', 10)
        self.get_logger().info('Keyboard control started. Use WASD, Q to quit')
        self.settings = termios.tcgetattr(sys.stdin)

    def get_key(self):
        tty.setraw(sys.stdin.fileno())
        key = sys.stdin.read(1)
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key

def run(self):
        twist = Twist()
        while True:
            key = self.get_key()
            if key == 'w':
                twist.linear.x = 0.5
            elif key == 's':
                twist.linear.x = -0.5
            elif key == 'a':
                twist.angular.z = 0.5
            elif key == 'd':
                twist.angular.z = -0.5
            elif key == 'q':
                break
            else:
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
