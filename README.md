# ROS2-AMR
PET project for the implementation of a semi-autonomous mobile robot in the framework of training July-August

&gt; Автономная мобильная платформа на ROS2 Humble с SLAM-навигацией и компьютерным зрением.
&gt; Проект создан в рамках 8-недельного плана обучения (июль–август 2026).

![Демо](docs/demos/final_demo.gif)


---

## 🚀 Быстрый старт

```bash
# 1. Клонировать
git clone https://github.com/ВАШ_НИК/ros2-amr-navbot.git
cd ros2-amr-navbot

# 2. Запустить в Docker
docker-compose up

# 3. Запустить все модули
ros2 launch amr_bringup navbot.launch.py
