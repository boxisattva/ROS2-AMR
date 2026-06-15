cmake_minimum_required(VERSION 3.8)
project(amr_description)

find_package(ament_cmake REQUIRED)
find_package(urdf REQUIRED)
find_package(xacro REQUIRED)

# Установка папок в share (чтобы ros2 launch их находил)
install(DIRECTORY
  urdf
  launch
  config
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
