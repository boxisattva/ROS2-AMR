# Week 4: Teleop + Одометрия + SLAM + Исправление багов

**Ветка:** `week4-integration`
**Цель:** получить работающую телеоперацию, одометрию с интеграцией позиции, корректное tf tree и первую карту в RViz.

## Поток A — Теория

| День | Тема | Применение |
|------|------|------------|
| Пн | tf2: frames, transforms, static_transform_publisher | base_link → laser_frame → odom |
| Вт | Одометрия дифференциального привода | dead reckoning, интегрирование pose |
| Ср | SLAM Toolbox: конфигурация, scan matching, сохранение карты | Построение карты помещения |
| Чт | Nav2 basics: costmaps, AMCL | Подготовка к Week 5 |
| Пт | Code review и best practices C++/Python | Рефакторинг перед merge |

## Поток B — Практика

### W4-ISS-01: Исправление критических багов Week 2–3
**Цель:** serial_bridge и teleop запускаются и работают в mock-режиме.

- Исправить `amr_base_control/CMakeLists.txt`: убрать `${catkin_LIBRARIES}`.
- Исправить `amr_base_control/package.xml`: добавить `nav_msgs`, `tf2`.
- Исправить `amr_base_control/launch/base_control.launch.py`: `esp32_bridge` → `serial_bridge`, добавить `mock:=true`.
- Исправить `amr_teleop/launch/teleop.launch.py`: убрать remap `/cmd_vel -> /amr/cmd_vel`.
- Исправить `serial_bridge.cpp`: подписка на `/cmd_vel`, tf broadcaster, интеграция позиции из `v_left`, `v_right`, `dt_ms`.
- Проверка:
  ```bash
  colcon build --packages-select amr_base_control amr_teleop
  source install/setup.bash
  ros2 launch amr_base_control base_control.launch.py mock:=true
  ros2 launch amr_teleop teleop.launch.py
  ```

### W4-ISS-02: Доработка URDF и tf tree
**Цель:** URDF содержит все фреймы для SLAM/Nav2.

- Добавить caster wheel, `laser_frame`, `camera_frame`, inertial, collision.
- Проверить в RViz:
  ```bash
  ros2 launch amr_description display.launch.py
  ```
- Обновить `week3_demo.launch.py`: заменить `odometry_publisher` на `serial_bridge`.

### W4-ISS-03: Интеграция одометрии и телеоперации
**Цель:** WASD → `/cmd_vel` → serial_bridge → `/odom` + tf.

- Проверить частоту `/cmd_vel` (10 Гц).
- Проверить `/odom` с изменяющейся позой.
- Проверить tf tree:
  ```bash
  ros2 run tf2_tools view_frames.py
  ```

### W4-ISS-04: Подключение реального железа
**Цель:** Работа с ESP32 + TB6612FNG + моторы (при наличии TB6612FNG).

- Железный аудит: `ls /dev/ttyUSB*`, питание TB6612FNG, отсутствие КЗ на моторах.
- Обновить ESP32-прошивку:
  - Реализовать `motor_driver.cpp` для TB6612FNG.
  - Убрать `delay()` из обработчика `BUZ`.
  - Исправить CRC в служебных сообщениях.
  - Унифицировать watchdog: 500 мс.
- End-to-end тест:
  ```bash
  ros2 launch amr_base_control base_control.launch.py mock:=false serial_port:=/dev/ttyUSB0
  ros2 launch amr_teleop teleop.launch.py
  ```

### W4-ISS-05: SLAM Toolbox — первая карта
**Цель:** Построить и сохранить карту помещения.

- Создать `amr_navigation/config/slam_toolbox.yaml`.
- Создать `amr_navigation/launch/slam.launch.py`.
- Проехать по комнате с teleop, сохранить карту:
  ```bash
  ros2 run nav2_map_server map_saver_cli -f ~/ROS2-AMR/src/amr_navigation/maps/week4_map
  ```

## Поток C — TL-артефакты

| День | Артефакт |
|------|----------|
| Пн | `docs/BOM.md` — статус TB6612FNG, аудит компонентов. |
| Вт | `docs/WIRING.md` — схема с TB6612FNG, pinout ESP32. |
| Ср | `docs/PROTOCOL.md` v1.1 — исправить BNF, watchdog 500 мс. |
| Чт | `docs/TROUBLESHOOTING.md` — проблемы Week 4. |
| Пт | `docs/SKILL_MATRIX.md` — teleop, odometry, SLAM. |
| Сб | `docs/PLAN_REVIEW_W4.md` + `docs/WEEKLY_LOGS.md` Week 4. |
| Вс | Code review, merge `week4-integration` → `main`, tag `v0.3.0`. |

## Критерии приемки

- [ ] `colcon build` проходит без ошибок.
- [ ] `serial_bridge` запускается в mock-режиме.
- [ ] `teleop` публикует `/cmd_vel`.
- [ ] `/odom` показывает изменяющуюся позу.
- [ ] tf tree: `odom -> base_link -> laser_frame`.
- [ ] URDF отображается в RViz.
- [ ] `/scan` публикуется и виден в RViz.
- [ ] SLAM Toolbox строит карту, карта сохранена.
- [ ] Документы обновлены и версионированы.
- [ ] `week4-integration` слита в `main` с tag `v0.3.0`.

## Риски и митигации

| Риск | Митигация |
|------|-----------|
| TB6612FNG не пришел / неисправен | Продолжить в mock-режиме; SLAM с ручным сканированием. |
| Одометрия сильно дрейфует | Калибровка `tools/calibrate_motors.py`, настройка wheel_base. |
| SLAM Toolbox не строит карту | Проверить tf tree, частоту `/scan`, качество одометрии. |
| Сборка ломается после merge | `make clean && make build` перед merge. |
