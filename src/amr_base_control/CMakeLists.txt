cmake_minimum_required(VERSION 3.8)
project(amr_base_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# === Зависимости ===
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)

# === Исполняемые файлы ===
add_executable(serial_bridge src/serial_bridge.cpp)
ament_target_dependencies(serial_bridge rclcpp geometry_msgs nav_msgs std_msgs tf2 tf2_ros)

# === Установка ===
install(TARGETS serial_bridge
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()