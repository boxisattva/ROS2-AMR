<p align="center">
  <!-- CI статус: зелёный = сборка проходит, красный = упала -->
  <a href="https://github.com/boxisattva/ROS2-AMR/actions/workflows/ci.yml">
    <img src="https://github.com/boxisattva/ROS2-AMR/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI Status">
  </a>
  <!-- Docker: показывает, что проект контейнеризирован -->
  <img src="https://img.shields.io/badge/Docker-24.0-blue?logo=docker&logoColor=white" alt="Docker">
  <!-- ROS2 Humble: целевая платформа -->
  <img src="https://img.shields.io/badge/ROS2-Humble%20Hawksbill-22314E?logo=ros&logoColor=white" alt="ROS2 Humble">
  <!-- Ubuntu: требуемая ОС -->
  <img src="https://img.shields.io/badge/Ubuntu-22.04%20LTS-E95420?logo=ubuntu&logoColor=white" alt="Ubuntu 22.04">
  <!-- Лицензия: MIT = open-source культура -->
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License: MIT">
  <!-- Версия проекта -->
  <img src="https://img.shields.io/badge/Version-1.0.0--alpha-blue" alt="Version">
</p>

<h1 align="center">ROS2-AMR</h1>

<p align="center">
  <strong>Автономный мобильный робот на ROS2 Humble</strong><br>
  Двухуровневая архитектура: ноутбук (SLAM + Nav2 + CV) + ESP32 (исполнение)
</p>

<p align="center">
  <!-- Placeholder для финального GIF. Заменить в неделю 8 -->
  <img src="docs/images/placeholder_demo.gif" alt="Demo placeholder" width="600">
  <br>
  <em>🎬 Финальное демо будет здесь (неделя 8)</em>
</p>

---

## 🎯 Описание проекта

**ROS2-AMR** — это автономный мобильный робот (AMR), построенный на базе ROS2 Humble с двухуровневой архитектурой: ноутбук Dell 7390 (Core i7, 16 GB RAM) выполняет алгоритмы навигации (SLAM Toolbox, Nav2, компьютерное зрение), а микроконтроллер ESP32 WROOM-32 управляет моторами, LED-индикацией и buzzer через кастомный UART-протокол.

**Ключевые особенности:**
- 🐳 **Docker-окружение** — воспроизводимая сборка ROS2 Humble
- 🤖 **6 ROS2-пакетов** — bringup, description, base_control, teleop, navigation, vision
- 📡 **Кастомный UART-протокол** — human-readable, с checksum и fail-safe
- 🗺️ **SLAM + Nav2** — автономная навигация по построенной карте
- 👁️ **Компьютерное зрение** — OpenCV + YOLO на ELP-камере
- 🔒 **Fail-safe** — ESP32 останавливает моторы при потере связи > 500 мс

---

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

### Требования
- Ubuntu 22.04 LTS (или Docker)
- Docker Engine 24.x
- Git

### 1. Клонирование

```bash
git clone https://github.com/boxisattva/ROS2-AMR.git
cd ROS2-AMR
```

### 2. Сборка (Docker)
```bash
# Собрать образ ROS2 Humble с проектом
docker build -f docker/Dockerfile -t ros2-amr-navbot .

# Или через Makefile
make build
```
### 3. Запуск
```bash
# Запустить контейнер с ROS2
docker-compose up -d

# Войти в контейнер
docker exec -it ros2-amr-navbot bash

# Внутри контейнера: source и запуск демо
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch amr_bringup bringup.launch.py
```
### 4. Проверка
```bash
# В новом терминале (внутри контейнера)
ros2 topic list
# Ожидаемый вывод: /chatter, /parameter_events, /rosout

ros2 topic echo /chatter
# Ожидаемый вывод: поток сообщений "Hello AMR! Count: X"
```

## 📁 Структура репозитория

ROS2-AMR/
├── README.md                 ← Вы здесь
├── LICENSE                   ← MIT
├── Makefile                  ← Удобные команды: make build, make test, make demo
├── docker/                   ← Docker-окружение ROS2 Humble
│   ├── Dockerfile
│   └── docker-compose.yml
├── .github/workflows/        ← CI/CD: автоматическая сборка и тесты
│   └── ci.yml
├── docs/                     ← 📚 Полная документация
│   ├── ARCHITECTURE.md       ← C4-модель, ADR, поток данных
│   ├── PROTOCOL.md           ← Спецификация UART-протокола
│   ├── BOM.md                ← Список компонентов с ценами
│   ├── WIRING.md             ← Схема подключений и питания
│   ├── SETUP.md              ← Пошаговая установка
│   ├── TROUBLESHOOTING.md    ← Решение типовых проблем
│   ├── SKILL_MATRIX.md       ← Матрица компетенций
│   └── WEEKLY_LOGS.md        ← Еженедельные отчёты
├── firmware/                 ← 🔧 ESP32-прошивка (PlatformIO)
│   └── amr_controller/
│       ├── src/
│       │   ├── main.cpp
│       │   ├── motor_driver.cpp
│       │   ├── uart_protocol.cpp
│       │   ├── led_feedback.cpp
│       │   ├── buzzer_feedback.cpp
│       │   ├── odometry.cpp
│       │   └── failsafe.cpp
│       └── platformio.ini
├── src/                      ← 🤖 ROS2-пакеты (ноутбук)
│   ├── amr_bringup/          ← Launch-файлы, демо talker/listener
│   ├── amr_description/      ← URDF/Xacro модель робота
│   ├── amr_base_control/     ← C++: UART ↔ ROS2 мост
│   ├── amr_teleop/           ← Python: управление с клавиатуры
│   ├── amr_navigation/       ← SLAM Toolbox + Nav2 конфигурация
│   └── amr_vision/           ← Python: камера + OpenCV/YOLO
└── tools/                    ← 🛠️ Скрипты
    ├── serial_monitor.py
    ├── calibrate_motors.py
    └── test_protocol.py


## 🏗️ Архитектура

Подробное описание системы, C4-диаграммы, ADR (архитектурные решения) и поток данных:
➡️ docs/ARCHITECTURE.md

Краткая схема:
```plain
┌─────────────┐      USB-UART        ┌─────────────┐
│   НОУТБУК   │ ◄─── CP2102 115200 ─►│    ESP32    │
│  ROS2 Humble│   Custom protocol    │   FreeRTOS  │
│  6 пакетов  │                      │  Motor/LED  │
└─────────────┘                      └─────────────┘
       │                                    │
   USB │                                GPIO │
       ▼                                    ▼
┌─────────────┐                      ┌─────────────┐
│  RPLIDAR A1 │                      │  TB6612FNG  │
│  ELP Camera │                      │  WS2812B    │
└─────────────┘                      │  Buzzer     │
                                     └─────────────┘
```

## 📊 Прогресс проекта (8 недель)

| Неделя | Фокус                                             | Статус          | Артефакты                          |
| ------ | ------------------------------------------------- | --------------- | ---------------------------------- |
| 1      | Инфраструктура: Docker, ROS2, GitHub, архитектура | 🟡 В работе     | 6 пакетов, CI, ADR                 |
| 2      | C++, URDF, UART-прототип                          | ⚪ Запланировано | `serial_bridge.cpp`, `PROTOCOL.md` |
| 3      | ESP32: прошивка, первый пуск                      | ⚪ Запланировано | Моторы крутятся по `$VEL`          |
| 4      | Teleop, одометрия, SLAM                           | ⚪ Запланировано | Карта в RViz                       |
| 5      | Nav2: автономная навигация                        | ⚪ Запланировано | Езда A→B с обходом                 |
| 6      | Компьютерное зрение                               | ⚪ Запланировано | YOLO + ROS2                        |
| 7      | Интеграция, Docker, документация                  | ⚪ Запланировано | Единый launch                      |
| 8      | Видео, резюме, собеседования                      | ⚪ Запланировано | 3-мин демо, 3 CV                   |

## 🛠️ Команды разработчика
```bash
week1-foundation
# Установка зависимостей (первый раз)
make setup


# Сборка всех пакетов
make build

# Запуск тестов
make test

# Очистка артефактов
make clean

# Запуск демо talker/listener
make demo

# Запуск полного стека (неделя 7+)
ros2 launch amr_bringup navbot.launch.py
```

## 📚 Документация

| Документ                                      | Назначение                                       |
| --------------------------------------------- | ------------------------------------------------ |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md)       | C4-модель, ADR, поток данных                     |
| [PROTOCOL.md](docs/PROTOCOL.md)               | UART-протокол: формат кадра, checksum, примеры   |
| [BOM.md](docs/BOM.md)                         | Список компонентов: цены, поставщики, статусы    |
| [WIRING.md](docs/WIRING.md)                   | Схема подключений, питание, безопасность         |
| [SETUP.md](docs/SETUP.md)                     | Пошаговая установка Docker и ROS2                |
| [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Типовые проблемы и решения                       |
| [SKILL\_MATRIX.md](docs/SKILL_MATRIX.md)      | Матрица компетенций: что освоено → где применено |
| [WEEKLY\_LOGS.md](docs/WEEKLY_LOGS.md)        | Еженедельные отчёты о прогрессе                  |

## CI/CD

Проект использует **GitHub Actions** для автоматической сборки и тестирования:

| Pipeline | Статус | Назначение |
|----------|--------|-----------|
| ROS2 CI | ![ROS2 CI](https://github.com/boxisattva/ROS2-AMR/actions/workflows/ci.yml/badge.svg) | Сборка, тесты, проверка зависимостей |

## Команды разработчика:

```bash
make setup    # Установить зависимости (первый раз)
make build    # Собрать проект
make test     # Запустить тесты
make clean    # Очистить артефакты
make demo     # Запустить демо talker/listener
```

## 🤝 Лицензия
MIT License — см. LICENSE.

## 👤 Автор

Защепенков Денис Станиславович — инженер-робототехник, 5 лет опыта.
Проект разработан для демонстрации компетенций в ROS2, embedded, архитектуре систем и управлении проектами.
📧 [ boxisattva@gmail.com] | 💼 [LinkedIn (www.linkedin.com/in/денис-защепенков-230704415)] | 🐙 [[GitHub](https://github.com/boxisattva)]



