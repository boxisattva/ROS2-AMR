# tools/test_uart.py
import serial
import sys

def crc8(data: str) -> str:
    crc = 0
    for c in data:
        crc ^= ord(c)
    return f"{crc:02X}"

def send_cmd(port, payload):
    frame = f"${payload}*{crc8(payload)}\n"
    with serial.Serial(port, 115200, timeout=1) as ser:
        # Очистить буфер
        ser.reset_input_buffer()
        ser.write(frame.encode())
        print(f"TX: {frame.strip()}")
        
        # Читаем ответ
        response = ser.readline().decode().strip()
        if response:
            print(f"RX: {response}")
        else:
            print("RX: [timeout, no response]")

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    cmd = sys.argv[2] if len(sys.argv) > 2 else "PING"
    
    send_cmd(port, cmd)
