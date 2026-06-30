#!/usr/bin/env python3
# ============================================================================
# CRC-8 калькулятор для UART-протокола ROS2-AMR-NavBot
# Формат кадра: $<PAYLOAD>*<CRC>\n
# CRC = XOR всех байтов между '$' и '*' (без '$' и без '*')
# ============================================================================

def crc8(data: str) -> str:
    """Вычисляет CRC-8 (XOR) для строки payload."""
    crc = 0
    for c in data:
        crc ^= ord(c)
    return f"{crc:02X}"


def print_crc(payload: str):
    """Выводит payload и его CRC в формате протокола."""
    crc = crc8(payload)
    print(f"${payload}*{crc}")


def main():
    print("=" * 50)
    print("CRC-8 калькулятор для ROS2-AMR-NavBot")
    print("=" * 50)
    print()
    
    # Стандартные команды
    print("--- Стандартные команды ---")
    print_crc("PING")
    print_crc("STO")
    print_crc("PONG,OK")
    print_crc("ACK,OK")
    print_crc("ACK,ERR,BAD_CRC")
    print()
    
    # Примеры с параметрами
    print("--- Примеры с параметрами ---")
    print_crc("VEL,0.3,0.0")
    print_crc("VEL,0.0,1.0")
    print_crc("VEL,-0.3,0.0")
    print_crc("VEL,0.2,0.5")
    print_crc("LED,0,1,0,0")
    print_crc("LED,1,0,0,1")
    print_crc("BUZ,2000,100")
    print_crc("BUZ,0,500")
    print()
    
    # Статусы
    print("--- Статусные сообщения ---")
    print_crc("STA,ESTOP,TRIGGERED")
    print_crc("STA,ESTOP,CLEARED")
    print_crc("STA,WATCHDOG,TIMEOUT")
    print()
    
    # Интерактивный режим
    print("--- Интерактивный режим ---")
    print("Введите payload (без $ и *), или 'q' для выхода:")
    
    while True:
        try:
            user_input = input("> ").strip()
            if user_input.lower() in ('q', 'quit', 'exit'):
                break
            if not user_input:
                continue
            
            crc = crc8(user_input)
            print(f"  ${user_input}*{crc}")
            print()
        except (EOFError, KeyboardInterrupt):
            break
    
    print("Готово.")


if __name__ == "__main__":
    main()
