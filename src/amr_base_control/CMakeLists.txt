cmake_minimum_required(VERSION 3.8)
project(amr_base_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(esp32_bridge src/esp32_bridge.cpp)
ament_target_dependencies(esp32_bridge rclcpp std_msgs)

install(TARGETS
  esp32_bridge
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)


ament_package()
