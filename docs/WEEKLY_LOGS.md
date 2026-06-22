# WEEKLY_LOGS.md

&gt; **Еженедельные отчёты — ROS2-AMR**  
&gt; **Формат:** Что сделано | Что не сделано | Почему | Что дальше

---

## Week 1: Фундамент (05.06.2026 — 13.06.2026)

### 🎯 Цель недели
Рабочее окружение ROS2 Humble в Docker, GitHub-репозиторий с архитектурой, первые Python-узлы, полная документация.

### ✅ Сделано

| Задача | Issue | Артефакт | Статус |
|--------|-------|----------|--------|
| Docker-образ ROS2 Humble (полный) | #1 | `docker/Dockerfile` с xacro, nav2, rviz2 | ✅ Готово |
| Docker-compose + volume mount | #1 | `docker-compose.yml`, `entrypoint.sh` | ✅ Готово |
| Makefile (build + docker) | #4 | `Makefile` с up/down/shell | ✅ Готово |
| Узлы talker/listener | #2 | `amr_bringup/talker.py`, `listener.py` | ✅ Готово |
| 6 пакетов ROS2 | #3 | `src/amr_*` (6 пакетов, launch-файлы) | ✅ Готово |
| CI/CD pipeline | #4 | `.github/workflows/ci.yml` | ✅ Готово |
| Архитектурная документация | #5 | `docs/ARCHITECTURE.md` (C4, 6 ADR) | ✅ Готово |
| Спецификация протокола | #6 | `docs/PROTOCOL.md` (UART v1.0) | ✅ Готово |
| Список компонентов (актуальные цены) | #7 | `docs/BOM.md` (Dell, ESP32, RPLIDAR) | ✅ Готово |
| Схема подключений | #8 | `docs/WIRING.md` (ASCII, питание, safety) | ✅ Готово |
| README + SKILL_MATRIX | #9 | `README.md`, `docs/SKILL_MATRIX.md` | ✅ Готово |
| Чистая сборка + Git SSH | #10 | Проверка на чистой машине | ✅ Пройдена |

### ⚠️ Что не сделано / отложено

| Задача | Причина | Новый срок |
|--------|---------|------------|
| C++ `serial_bridge` компилируется без ошибок | Требует `libserial-dev`, проверено в контейнере | Неделя 2 |
| URDF отображается в RViz | Требует `robot_state_publisher`, пакет установлен | Неделя 2 |
| Gazebo-симуляция | Нет приоритета — реальное железо в пути (14.06) | Неделя 2 (опционально) |
| Фото проводки | Шасси ещё не собрано в финальную конфигурацию | Неделя 3 |

### 🐛 Проблемы и решения

| Проблема | Решение | Урок |
|----------|---------|------|
| Docker Hub недоступен (TLS timeout) | Зеркала `mirror.gcr.io`, кэш образов | В РФ нужен VPN или зеркала для Docker |
| `cp: cannot stat '/workspace/src/*'` | Volume mount `-v $(pwd)/src:/ros2_ws/src` | Никогда не копировать код в образ — только mount |
| `nav2_bringup` not found | Добавить в Dockerfile: `ros-humble-nav2-bringup` | Базовый образ ROS2 — минимальный, всё нужное ставить явно |
| Git push: password auth disabled | Переключиться на SSH (`git@github.com`) | GitHub отключил HTTPS-пароли в 2021 |
| Контейнер с таким именем уже существует | `docker rm -f ros2-amr-navbot` перед `up` | Старые контейнеры блокируют новые |

### 📊 Метрики

| Параметр | Значение |
|----------|----------|
| Календарных дней | 9 (05.06 — 13.06) |
| Коммитов | 15+ |
| Строк кода | ~900 (Python) + ~250 (C++ заготовки) |
| Документов | 8 md-файлов |
| Время на проект | ~18 часов (вечера, выходные) |
| Блокирующие факторы | Ожидание RPLIDAR и камеры (поступление 14.06) |

### 🧠 Выводы и инсайты

1. **Docker-compose спасает.** `docker-compose up -d` + `docker exec` удобнее, чем длинные `docker run` с кучей флагов.
2. **Entrypoint экономит время.** Автоматический `source /opt/ros/humble/setup.bash` — не нужно вводить каждый раз.
3. **SSH для Git — must have.** После настройки `git push` работает без паролей.
4. **Базовый образ ROS2 — минимальный.** `ros:humble-ros-base-jammy` не содержит ни xacro, ни nav2. Всё нужно явно прописывать в Dockerfile.
5. **Volume mount &gt; COPY.** При разработке код должен быть на хосте и монтироваться в контейнер. COPY в Dockerfile — только для CI/CD.

### 📅 План Week 2 (13.06 — 20.06)

| День | Поток A (Теория) | Поток B (Практика) | Поток C (Портфолио) |
|------|------------------|--------------------|---------------------|
| 1 (Пн) | C++ для ROS2 (rclcpp) | `serial_bridge.cpp`: заготовка publisher/subscriber | `docs/PROTOCOL.md` v1.1 |
| 2 (Вт) | URDF / Xacro | `navbot.urdf.xacro`: base_link, 2 wheels, caster | `display.launch.py` |
| 3 (Ср) | tf2 | `robot_state_publisher` + RViz | Скриншот модели |
| 4 (Чт) | UART практика | Python: отправка `$VEL` через CP2102 | `tools/serial_monitor.py` |
| 5 (Пт) | Парсинг протокола | Python: разбор кадров `$ODO` | `tools/test_protocol.py` |
| 6 (Сб) | Интеграция | Первый тест: ноутбук → CP2102 → ESP32 (эхо) | `WEEKLY_LOGS.md` W2 |
| 7 (Вс) | Ревизия | Чистая сборка Week 2 | `PLAN_REVIEW_W2.md` |

### 🔗 Связанные документы
- [ARCHITECTURE.md](ARCHITECTURE.md) — обоснование архитектуры
- [BOM.md](BOM.md) — статус компонентов (RPLIDAR приходит 14.06)
- [SKILL_MATRIX.md](SKILL_MATRIX.md) — обновление навыков
- [PROTOCOL.md](PROTOCOL.md) — UART v1.0


## Week 3 — День 1 (2026-06-22)

### Выполнено
- [x] W3-Iss1: Валидация ESP32-DevKitC V4
  - Blink прошивается, loopback 100/100, все 6 GPIO целы
  - Onboard CP2102 работает стабильно, внешний модуль не нужен
- [x] W3-Iss3: Базовая прошивка safety
  - 4 красных LED (GPIO25/26/32/33) + резисторы 220Ω
  - E-Stop (GPIO4, INPUT_PULLUP): мгновенный стоп
  - Watchdog 200 мс: стоп при потере связи
  - Heartbeat LED: пульс 500 мс в idle
- [x] W3-Iss4: FSM UART-парсер + PWM
  - Парсер: WAIT_DOLLAR → READ_PAYLOAD → READ_CRC1 → READ_CRC2 → VALIDATE
  - CRC-8 XOR, 5 команд dispatch
  - Differential drive: v_left = v - ω·L/2, v_right = v + ω·L/2
  - PWM через ledc (каналы 0/1), deadband 0.01 м/с, clamp [0,255]
  - Инструмент `tools/crc_calc.py` для расчёта CRC

### Проблемы и решения
- Проблема: `ledcSetup`/`ledcAttachPin` вызывали reboot loop
- Решение: убраны из Issue 3, добавлены в Issue 4 после валидации blink
- Проблема: мусор от bootloader'а после прошивки
- Решение: нажатие EN для чистого ребута (в `platformio.ini` добавить `upload_resetmethod`)

### Завтра (День 2)
- W3-Iss2: Диагностика RPLIDAR A1 (железо уже на руках)
- W3-Iss5: Интеграция serial_bridge (ноутбук ↔ ESP32, end-to-end тест)
- Подключение моторов к TB6612FNG (физическая сборка)


