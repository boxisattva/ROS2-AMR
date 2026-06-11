# Базовый образ Ubuntu 22.04 с ROS2 Humble
FROM ros:humble-ros-base-jammy

# Отключаем интерактивный режим apt
ENV DEBIAN_FRONTEND=noninteractive

# Установка зависимостей
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    python3-pip \
    python3-colcon-common-extensions \
    python3-rosdep \
    ros-humble-desktop \
    && rm -rf /var/lib/apt/lists/*

# Инициализация rosdep
RUN rosdep init || true && rosdep update

# Создание рабочего пространства
WORKDIR /ros2_ws

# Точка входа — запуск bash с sourced ROS2
ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/humble/setup.bash && exec bash"]
