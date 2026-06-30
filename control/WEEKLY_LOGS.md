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


## Week 3 — День 2 (2026-06-24)

### Выполнено
- [x] W3-Iss2: RPLIDAR A1 диагностика — работает, /scan публикуется
- [x] W3-Iss4: FSM UART-парсер + PWM код написан
- [x] WEMOS Motor Shield v1.0 диагностика: I2C не отвечает

### Hardware диагностика WEMOS Shield
| Параметр | Результат |
|----------|-----------|
| VM (батареи) | 6.4V ✅ |
| Логика | 3.3V ✅ |
| Моторы | 2 Ом / 1.8 Ом ✅ |
| КЗ на корпус | Нет ✅ |
| I2C scan (0x30, 0x60, 0x61, 0x70) | NACK error=2 ❌ |
| Full scan (1-127, 50/100/400kHz) | 0 devices ❌ |
| Blind write 0x40 | NACK ❌ |

**Вывод:** PCA9685 на shield неисправен. Требуется замена.

### Fallback
- [ ] Заказать TB6612FNG (3-7 дней)
- [ ] Демо ESP32 + LED + Protocol (видео)
- [ ] SLAM с RPLIDAR (ручное сканирование)

### Блокеры
- W3-Iss4 (PWM + моторы) — ждём TB6612FNG


## Week 3 — Итог (2026-06-22 ~ 2026-06-30)

### Выполнено
- [x] W3-Iss1: Валидация ESP32-DevKitC V4 (onboard CP2102, loopback 100/100)
- [x] W3-Iss2: Диагностика RPLIDAR A1 — `/scan` 5+ Гц, RViz 360°
- [x] W3-Iss3: Safety-прошивка ESP32 (E-Stop, watchdog 200мс, 4 LED, heartbeat)
- [x] W3-Iss4: FSM UART-парсер + CRC-8, 5 команд ($VEL/$LED/$BUZ/$STO/$PING)
- [x] W3-Iss5: Интеграция serial_bridge (mock + реальный UART, CRC-фикс в процессе)
- [x] W3-Iss6: Launch-файл `week3_demo.launch.py` (RPLIDAR + TF + odometry)
- [x] W3-Iss9: Чистая сборка проходит, CI настроен на все ветки (`'**'`)

### Не выполнено / перенесено
- [ ] W3-Iss4: PWM + движение моторов — **блокер: WEMOS Motor Shield I2C не отвечает**
- [ ] W3-Iss7: Полная демо с движением — **ждём TB6612FNG**
- [ ] W3-Iss8: SLAM Toolbox — **переносится на Week 4**

### Hardware статус
| Компонент | Статус | Примечание |
|-----------|--------|------------|
| ESP32-DevKitC V4 | ✅ Работает | Onboard CP2102, прошивка stable |
| RPLIDAR A1 | ✅ Работает | `/scan`, 5+ Гц, визуализация OK |
| WEMOS Motor Shield v1.0 | ❌ Неисправен | I2C скан: 0 устройств на всех адресах |
| TB6612FNG | 🚚 В пути | Заказан, 2-3 июля |
| Моторы TT | ✅ Подключены | 2 Ом / 1.8 Ом, КЗ нет |

### Архитектурные решения
- Отказ от WEMOS Motor Shield (I2C) → переход на TB6612FNG (PWM)
- `serial_bridge` — termios UART, FSM-парсер RX, CRC-8
- `keyboard_control` — `/cmd_vel` (унифицированный топик)
- `week3_demo.launch.py` — единый launch (без URDF пока)

### Технический долг
- [ ] Исправить CRC в ESP32-прошивке (сейчас `*00` заглушка)
- [ ] Исправить `serial_bridge.cpp` — толерантность к `*00` или правильный CRC
- [ ] Починить URDF (`amr.urdf.xacro` — размеры под 148мм базу)
- [ ] Добавить `WHEEL_RADIUS` (0.031м) в одометрию
- [ ] Убрать `delay()` из `BUZ` dispatch (non-blocking)

### Метрики Week 3
- Закрыто issues: 6 из 10
- Часов затрачено: ~20
- Git коммитов: 5+
- Видео: не записано (блокер: нет движения)
- Документация: WIRING.md обновлён, PROTOCOL.md актуален


