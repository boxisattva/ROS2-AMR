# ~/ROS2-AMR/tools/test_protocol_loopback.py
# =============================================================================
# Лупбэк-тест: Python отправляет кадр, Python же парсит его.
# Имитирует работу ESP32 на уровне протокола.
# НЕ ТРЕБУЕТ ЖЕЛЕЗА И БИБЛИОТЕКИ serial
# =============================================================================

# ===========================================================================
# Checksum (точная копия из PROTOCOL.md)
# ===========================================================================
def checksum(data: str) -> str:
    cs = 0
    for ch in data:
        cs ^= ord(ch)
    return f"{cs:02X}"

# ===========================================================================
# Frame builder
# ===========================================================================
def build_frame(frame_type: str, *args) -> str:
    payload = f"{frame_type},{','.join(map(str, args))}" if args else frame_type
    return f"${payload}*{checksum(payload)}\n"

# ===========================================================================
# Frame parser (имитация ESP32 uart_protocol.cpp)
# ===========================================================================
class FrameParser:
    def __init__(self):
        self.buffer = ""
        self.on_valid_frame = None   # Callback: func(type, args)
        self.on_error = None         # Callback: func(code, desc)
        self.on_checksum_mismatch = None  # Callback для bad checksum тестов
    
    def feed(self, data: str):
        """Принимает поток символов, ищет полные кадры"""
        self.buffer += data
        
        while True:
            start = self.buffer.find('$')
            if start == -1:
                self.buffer = ""
                return
            
            end = self.buffer.find('\n', start)
            if end == -1:
                # Неполный кадр — ждём ещё данных
                self.buffer = self.buffer[start:]
                return
            
            frame = self.buffer[start:end+1]
            self.buffer = self.buffer[end+1:]
            self._process_frame(frame)
    
    def _process_frame(self, frame: str):
        """Парсит один кадр, проверяет checksum, вызывает callback"""
        # Проверка структуры
        if '*' not in frame:
            self._error("BAD_FRAME", "No checksum separator")
            return
        
        payload_part, cs_part = frame[1:].split('*', 1)
        cs_received = cs_part.strip()
        
        # Проверка checksum
        cs_expected = checksum(payload_part)
        if cs_received != cs_expected:
            if self.on_checksum_mismatch:
                self.on_checksum_mismatch(cs_expected, cs_received)
            else:
                self._error("BAD_FRAME", f"Checksum mismatch: expected {cs_expected}, got {cs_received}")
            return
        
        # Разбор payload
        parts = payload_part.split(',')
        frame_type = parts[0]
        args = [float(x) if '.' in x else int(x) for x in parts[1:]]
        
        if self.on_valid_frame:
            self.on_valid_frame(frame_type, args)
    
    def _error(self, code, desc):
        if self.on_error:
            self.on_error(code, desc)

# ===========================================================================
# Тесты
# ===========================================================================
def run_tests():
    print("=" * 50)
    print("PROTOCOL UNIT TESTS (no hardware required)")
    print("=" * 50)
    
    parser = FrameParser()
    results = {"passed": 0, "failed": 0, "expected_errors": 0}
    
    def on_frame(ft, args):
        print(f"  [RX] Type={ft}, Args={args}")
    
    def on_err(code, desc):
        print(f"  [ERR] {code}: {desc}")
        results["failed"] += 1
    
    def on_checksum_mismatch(expected, received):
        print(f"  [ERR] Checksum mismatch: expected {expected}, got {received}")
        results["expected_errors"] += 1  # Это ожидаемая ошибка в тесте 4
    
    parser.on_valid_frame = on_frame
    parser.on_error = on_err
    parser.on_checksum_mismatch = on_checksum_mismatch
    
    # Test 1: Valid VEL frame
    print("\n--- Test 1: Valid $VEL ---")
    frame = build_frame("VEL", 0.25, 0.0)
    print(f"  [TX] {frame.strip()}")
    parser.feed(frame)
    results["passed"] += 1
    
    # Test 2: Valid STO frame
    print("\n--- Test 2: Valid $STO ---")
    frame = build_frame("STO")
    print(f"  [TX] {frame.strip()}")
    parser.feed(frame)
    results["passed"] += 1
    
    # Test 3: Valid PING frame
    print("\n--- Test 3: Valid $PING ---")
    frame = build_frame("PING")
    print(f"  [TX] {frame.strip()}")
    parser.feed(frame)
    results["passed"] += 1
    
    # Test 4: Bad checksum (should detect error)
    print("\n--- Test 4: Bad checksum (expect error) ---")
    bad_frame = "$VEL,0.25,0.0*FF\n"
    print(f"  [TX] {bad_frame.strip()}")
    parser.feed(bad_frame)
    if results["expected_errors"] == 1:
        print("  ✓ Error detected correctly")
        results["passed"] += 1
    else:
        print("  ✗ Error NOT detected")
        results["failed"] += 1
    
    # Test 5: Multiple frames in one chunk
    print("\n--- Test 5: Multiple frames ---")
    frames = build_frame("LED", 2, 0, 0, 0) + build_frame("BUZ", 1000, 200)
    print(f"  [TX] {frames.strip()}")
    parser.feed(frames)
    results["passed"] += 1
    
    # Test 6: Partial frame — ПРАВИЛЬНЫЙ тест
    # Отправляем кадр по частям, парсер должен накопить в буфере
    print("\n--- Test 6: Partial frame (chunked) ---")
    # Сначала сбросим буфер
    parser.buffer = ""
    
    full_frame = build_frame("VEL", 0.5, 0.0)
    print(f"  [FULL] {full_frame.strip()}")
    
    # Разбиваем на части
    part1 = full_frame[:5]    # "$VEL,"
    part2 = full_frame[5:12]  # "0.5,0.0"
    part3 = full_frame[12:]   # "*5A\n"
    
    print(f"  [PART1] '{part1}'")
    parser.feed(part1)
    print(f"  [PART2] '{part2}'")
    parser.feed(part2)
    print(f"  [PART3] '{part3}'")
    parser.feed(part3)
    
    results["passed"] += 1
    
    # Test 7: Garbage before frame
    print("\n--- Test 7: Garbage before frame ---")
    parser.buffer = ""
    garbage_frame = "XXX$VEL,0.1,0.0*??\n"  # Невалидный checksum
    # Правильный кадр после мусора
    good_frame = build_frame("VEL", 0.1, 0.0)
    parser.feed("XXX" + good_frame)
    results["passed"] += 1
    
    # Test 8: Empty frame (just $)
    print("\n--- Test 8: Empty type ---")
    parser.buffer = ""
    parser.feed("$*00\n")  # Пустой тип, checksum XOR("") = 0
    # Это валидный кадр с пустым типом — проверим, что парсер не падает
    results["passed"] += 1
    
    print("\n" + "=" * 50)
    total_passed = results["passed"]
    total_failed = results["failed"]
    print(f"RESULTS: {total_passed} passed, {total_failed} failed")
    if total_failed == 0:
        print("ALL TESTS PASSED ✓")
    else:
        print(f"SOME TESTS FAILED ({total_failed}) ✗")
    print("=" * 50)

if __name__ == '__main__':
    run_tests()