#!/usr/bin/env python3
"""
keyboard_control.py — телеуправление роботом с клавиатуры.

Назначение:
  Читать клавиши WASD/Space/Q из терминала и публиковать
  geometry_msgs/Twist в топик /cmd_vel.

Важно:
  Этот узел работает только если stdin подключен к терминалу (TTY).
  Запуск через `ros2 launch` без отдельного терминала приведет к ошибке termios.
  Решение: teleop.launch.py запускает этот узел через xterm.
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import sys
import termios
import tty
import select


class KeyboardControl(Node):
    def __init__(self):
        # Вызываем конструктор базового класса Node.
        # Результат: в ROS2 появляется узел с именем keyboard_control.
        super().__init__('keyboard_control')

        # Publisher: отправляем команды скорости в топик /cmd_vel.
        # Twist состоит из двух частей:
        #   linear.x  — линейная скорость вперед/назад (м/с)
        #   angular.z — угловая скорость поворота (рад/с)
        self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)

        # Настраиваем скорости.
        self.linear_speed = 0.5   # м/с
        self.angular_speed = 0.5  # рад/с

        # Проверяем, что stdin — настоящий терминал.
        # termios работает только с TTY. Без TTY tcgetattr упадет.
        if not sys.stdin.isatty():
            error_msg = (
                'stdin is not a TTY. '
                'Run with: ros2 run amr_teleop keyboard_control, '
                'or use launch with a terminal emulator (xterm/gnome-terminal).'
            )
            self.get_logger().error(error_msg)
            raise RuntimeError(error_msg)

        # Сохраняем текущие настройки терминала,
        # чтобы восстановить их при выходе (важно для Ctrl+C и завершения).
        try:
            self.settings = termios.tcgetattr(sys.stdin)
        except termios.error as e:
            self.get_logger().error(f'Failed to get terminal attributes: {e}')
            raise

        self.get_logger().info(
            'Keyboard control started. Use WASD, Space to stop, Q to quit'
        )

    def get_key(self):
        """
        Считывает одну клавишу без блокировки.

        Алгоритм:
          1. tty.setraw переводит терминал в "сырой" режим:
             каждое нажатие сразу передается программе, без ожидания Enter.
          2. select.select ждет ввода 0.1 секунды.
             Если клавиша не нажата — возвращает пустую строку.
          3. Восстанавливаем настройки терминала.
        """
        tty.setraw(sys.stdin.fileno())

        # select.select(читаем, пишем, ошибки, таймаут_сек)
        ready, _, _ = select.select([sys.stdin], [], [], 0.1)
        if ready:
            key = sys.stdin.read(1)
        else:
            key = ''

        # Восстанавливаем терминал после каждого чтения.
        # Если не восстановить, терминал останется в raw-режиме.
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key

    def make_stop_twist(self):
        """Создает Twist-сообщение с нулевыми скоростями (полный стоп)."""
        twist = Twist()
        twist.linear.x = 0.0
        twist.angular.z = 0.0
        return twist

    def run(self):
        """
        Главный цикл.

        Логика управления:
          W — вперед
          S — назад
          A — поворот налево на месте
          D — поворот направо на месте
          Space — экстренная остановка
          Q — выход
          нет нажатия — отправляем стоп, чтобы робот не продолжал движение
        """
        twist = Twist()

        try:
            while rclpy.ok():
                key = self.get_key()

                if key == 'w' or key == 'W':
                    twist.linear.x = self.linear_speed
                    twist.angular.z = 0.0

                elif key == 's' or key == 'S':
                    twist.linear.x = -self.linear_speed
                    twist.angular.z = 0.0

                elif key == 'a' or key == 'A':
                    twist.linear.x = 0.0
                    twist.angular.z = self.angular_speed

                elif key == 'd' or key == 'D':
                    twist.linear.x = 0.0
                    twist.angular.z = -self.angular_speed

                elif key == ' ':
                    # Пробел = стоп.
                    twist = self.make_stop_twist()

                elif key == 'q' or key == 'Q':
                    # Q = выход.
                    break

                elif key == '':
                    # Ничего не нажато.
                    # Раньше здесь был continue, из-за чего в /cmd_vel
                    # "зависала" последняя ненулевая скорость.
                    # Теперь явно отправляем стоп.
                    twist = self.make_stop_twist()

                # Публикуем команду в /cmd_vel.
                self.publisher_.publish(twist)
                self.get_logger().info(
                    f'cmd: linear={twist.linear.x:.2f}, angular={twist.angular.z:.2f}'
                )

        finally:
            # Гарантированная остановка при любом выходе:
            # Ctrl+C, Q или исключение.
            self.publisher_.publish(self.make_stop_twist())
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)


def main(args=None):
    # Инициализируем ROS2.
    rclpy.init(args=args)

    node = KeyboardControl()
    try:
        node.run()
    except Exception as e:
        node.get_logger().error(f'Error: {e}')
        # Пробрасываем ошибку, чтобы launch/process увидел неудачу.
        raise
    finally:
        # Гарантированно освобождаем ресурсы ROS2.
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()