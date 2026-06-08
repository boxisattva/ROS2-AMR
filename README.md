# ROS2-AMR
PET project for the implementation of a semi-autonomous mobile robot in the framework of training July-August

&gt; Автономная мобильная платформа на ROS2 Humble с SLAM-навигацией и компьютерным зрением.
&gt; Проект создан в рамках 8-недельного плана обучения (июль–август 2026).

![Демо](docs/demos/final_demo.gif)

## Структура пакетов

| Пакет | Язык | Назначение | Launch-файл |
|-------|------|-----------|-------------|
| `amr_bringup` | Python | Запуск системы, демо talker/listener | `bringup.launch.py` |
| `amr_description` | CMake | URDF/Xacro модель робота | `display.launch.py` |
| `amr_base_control` | C++ | Мост с ESP32, управление моторами | `base_control.launch.py` |
| `amr_teleop` | Python | Управление с клавиатуры | `teleop.launch.py` |
| `amr_navigation` | CMake | SLAM + Nav2 навигация | `navigation.launch.py` |
| `amr_vision` | Python | Камера + компьютерное зрение | `vision.launch.py` |

---

## 🚀 Быстрый старт

```bash
# 1. Клонировать
git clone https://github.com/boxisattva/ROS2-AMR.git
cd ROS2-AMR

# 2. Запустить в Docker
docker-compose up --build # или colcon build

# 3. Запустить все модули
ros2 launch amr_bringup bringup.launch.py
