# Удобные команды для разработки ROS2-проекта
# Использование: make <target>

# Переменные — пути и настройки
WORKSPACE := $(shell pwd)           # Текущая директория (~/ROS2-AMR)
ROS_DISTRO := humble                # Версия ROS2
BUILD_TYPE := Release               # Тип сборки: Release / Debug

# Цвета для красивого вывода (опционально)
GREEN := \033[0;32m
RED := \033[0;31m
NC := \033[0m                      # No Color

# Target: build — собрать все пакеты
# Что делает: запускает colcon build с оптимизацией
# Когда использовать: после изменения кода или при первой сборке
build:
	@echo "$(GREEN)>>> Building ROS2 packages...$(NC)"
	@source /opt/ros/$(ROS_DISTRO)/setup.bash && \
		colcon build \
		--cmake-args -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		--symlink-install  # symlink-install: Python-изменения не требуют пересборки
	@echo "$(GREEN)>>> Build complete. Source with: source install/setup.bash$(NC)"

# =============================================================================
# Target: test — запустить тесты
# =============================================================================
# Что делает: собирает и запускает тесты, показывает результаты
# Когда использовать: перед коммитом, чтобы убедиться что ничего не сломано
test: build
	@echo "$(GREEN)>>> Running tests...$(NC)"
	@source /opt/ros/$(ROS_DISTRO)/setup.bash && \
		source $(WORKSPACE)/install/setup.bash && \
		colcon test --return-code-on-test-failure && \
		colcon test-result --verbose
	@echo "$(GREEN)>>> All tests passed$(NC)"

# =============================================================================
# Target: clean — полная очистка артефактов сборки
# =============================================================================
# Что делает: удаляет build/, install/, log/ — возвращает проект в исходное состояние
# Когда использовать: при странных ошибках сборки, перед чистой сборкой
clean:
	@echo "$(RED)>>> Cleaning build artifacts...$(NC)"
	@rm -rf build/ install/ log/
	@echo "$(GREEN)>>> Clean complete. Run 'make build' to rebuild$(NC)"


# =============================================================================
# Target: setup — первоначальная настройка окружения
# =============================================================================
# Что делает: устанавливает зависимости через rosdep
# Когда использовать: после git clone на новой машине
setup:
	@echo "$(GREEN)>>> Setting up dependencies...$(NC)"
	@sudo apt-get update
	@rosdep update
	@rosdep install --from-paths src --ignore-src -y
	@echo "$(GREEN)>>> Setup complete$(NC)"

# =============================================================================
# Target: demo — запуск демо talker/listener
# =============================================================================
# Что делает: собирает проект и запускает launch-файл amr_bringup
# Когда использовать: для быстрой проверки, что всё работает
demo: build
	@echo "$(GREEN)>>> Starting demo...$(NC)"
	@source /opt/ros/$(ROS_DISTRO)/setup.bash && \
		source $(WORKSPACE)/install/setup.bash && \
		ros2 launch amr_bringup bringup.launch.py


# =============================================================================
# Docker-цели
# =============================================================================

# Запуск контейнера (docker-compose up -d)
up:
	@echo "$(GREEN)>>> Starting Docker container...$(NC)"
	@docker-compose up -d
	@echo "$(GREEN)>>> Container running. Use: make shell$(NC)"

# Вход в контейнер
shell:
	@docker exec -it ros2-amr-navbot bash

# Остановка контейнера
down:
	@echo "$(RED)>>> Stopping Docker container...$(NC)"
	@docker-compose down

# Проверка парсера
check-urdf:
	@xacro src/amr_description/urdf/navbot.urdf.xacro -o /tmp/navbot.urdf && \
		check_urdf /tmp/navbot.urdf


# =============================================================================
# Специальная цель: .PHONY — объявляет, что эти цели не являются файлами
# =============================================================================
# Без этого make будет искать файл с именем "build" и игнорировать команду,
# если такой файл существует
.PHONY: build test clean setup demo up shell down
