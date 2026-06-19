cmake_minimum_required(VERSION 3.8)
project(amr_base_control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# =============================================================================
# Зависимости ROS2
# =============================================================================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)      # <-- ДОБАВЛЕНО: Odometry
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)            # <-- ДОБАВЛЕНО: Quaternion
find_package(tf2_ros REQUIRED)        # <-- ДОБАВЛЕНО: TransformBroadcaster

# =============================================================================
# serial_bridge: UART ↔ ROS2 мост
# =============================================================================
add_executable(serial_bridge src/serial_bridge.cpp)
ament_target_dependencies(serial_bridge rclcpp geometry_msgs std_msgs)

# =============================================================================
# odometry_publisher: dead reckoning + TF
# =============================================================================
add_executable(odometry_publisher src/odometry_publisher.cpp)
ament_target_dependencies(odometry_publisher rclcpp geometry_msgs nav_msgs std_msgs tf2 tf2_ros)

# =============================================================================
# Установка
# =============================================================================
install(TARGETS
  serial_bridge
  odometry_publisher
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
