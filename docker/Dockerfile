FROM ros:humble-ros-base-jammy

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    nano \
    git \
    python3-pip \
    python3-colcon-common-extensions \
    python3-rosdep \
    ros-humble-desktop \
    ros-humble-xacro \
    ros-humble-urdf \
    ros-humble-robot-state-publisher \
    ros-humble-joint-state-publisher-gui \
    ros-humble-nav2-bringup \
    ros-humble-slam-toolbox \
    ros-humble-rplidar-ros \
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    ros-humble-teleop-twist-keyboard \
    ros-humble-rviz2 \
    libserial-dev \
    && rm -rf /var/lib/apt/lists/*

RUN rosdep init || true && rosdep update

WORKDIR /ros2_ws

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]

RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc && \
    echo "source /ros2_ws/install/setup.bash 2>/dev/null || true" >> /root/.bashrc
