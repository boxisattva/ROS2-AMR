import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class ListenerNode(Node):
    def __init__(self):
        super().__init__('listener')
        self.subscription = self.create_subscription(
            String,
            '/chatter',
            self.listener_callback,
            10
        )
        self.get_logger().info('Listener node started. Subscribed to /chatter')

    def listener_callback(self, msg):
        self.get_logger().info(f'Received: "{msg.data}"')




def main(args=None):
    rclpy.init(args=args)
    node = ListenerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Listener stopped by user')
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
