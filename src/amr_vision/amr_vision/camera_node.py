#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
import cv2
from cv_bridge import CvBridge


class CameraNode(Node):
    def __init__(self):
        super().__init__('camera_node')
        self.publisher_ = self.create_publisher(Image, '/amr/camera/image_raw', 10)
        self.bridge = CvBridge()
        self.camera_id = self.declare_parameter('camera_id', 0).value
        self.publish_rate = self.declare_parameter('publish_rate', 30.0).value
        
        self.timer = self.create_timer(1.0 / self.publish_rate, self.timer_callback)
        self.cap = cv2.VideoCapture(self.camera_id)
        
        self.get_logger().info(f'Camera node started. ID: {self.camera_id}')


    def timer_callback(self):
        ret, frame = self.cap.read()
        if ret:
            msg = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
            self.publisher_.publish(msg)

    def destroy_node(self):
        self.cap.release()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = CameraNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

