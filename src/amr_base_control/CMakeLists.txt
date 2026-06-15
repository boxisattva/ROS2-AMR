cmake_minimum_required(VERSION 3.8)
project(amr_base_control)

# C++17 для std::clamp
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# Исполняемый файл serial_bridge
add_executable(serial_bridge src/serial_bridge.cpp)
ament_target_dependencies(serial_bridge rclcpp geometry_msgs std_msgs)

target_include_directories(serial_bridge PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(serial_bridge PUBLIC c_std_99 cxx_std_17)

install(TARGETS serial_bridge
  DESTINATION lib/${PROJECT_NAME})

# Launch файлы (если есть)
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
