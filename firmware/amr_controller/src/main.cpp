#include <Arduino.h>
#include <Wire.h>
//#include <WEMOS_Motor.h>

// ==================== ПРОТОТИПЫ (forward declarations) ====================
void setLED(bool on);
void setBuzzer(bool on);
void stopMotors();
void setMotorI2C(uint8_t motor, int16_t speed);
void processCommand(String cmd);
bool validateCRC(String cmd);

// ==================== ПИНЫ ====================
#define I2C_SDA         21
#define I2C_SCL         22
#define LED_PIN         2
#define BUZZER_PIN      23
#define ESTOP_PIN       4

// ==================== ГЛОБАЛЬНЫЕ ====================
uint8_t shieldAddr = 0;
bool estopActive = false;
unsigned long lastHeartbeat = 0;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ESTOP_PIN, INPUT_PULLUP);
  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  delay(500);
  
  // I2C сканер
  Serial.print("I2C scan: ");
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      Serial.print(" ");
      if (addr >= 0x2D && addr <= 0x30) shieldAddr = addr;
    }
  }
  Serial.println();
  
  if (shieldAddr != 0) {
    Serial.print("Shield found at 0x");
    Serial.println(shieldAddr, HEX);
    setLED(true);
  } else {
    Serial.println("ERROR: Shield NOT found!");
    for (int i = 0; i < 5; i++) {
      setLED(true); delay(100); setLED(false); delay(100);
    }
  }
  
  Serial.println("$PONG,INIT*00");
}

// ==================== LOOP ====================
void loop() {
  unsigned long now = millis();
  
  // E-Stop
  if (digitalRead(ESTOP_PIN) == LOW) {
    if (!estopActive) {
      estopActive = true;
      stopMotors();
      setLED(true);
      Serial.println("$STA,2,ESTOP*00");
    }
  } else {
    if (estopActive) {
      estopActive = false;
      setLED(false);
      Serial.println("$STA,0,OK*00");
    }
  }
  
  // Heartbeat
  if (!estopActive && (now - lastHeartbeat >= 500)) {
    lastHeartbeat = now;
    setLED(!digitalRead(LED_PIN));
  }
  
  // UART
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    processCommand(cmd);
  }
}

// ==================== РЕАЛИЗАЦИИ ФУНКЦИЙ (внизу) ====================
void setLED(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void setBuzzer(bool on) {
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
}

void setMotorI2C(uint8_t motor, int16_t speed) {
  if (shieldAddr == 0) return;
  // TODO: реализовать после даташита
}

void stopMotors() {
  setMotorI2C(0, 0);
  setMotorI2C(1, 0);
}

void processCommand(String cmd) {
  if (!validateCRC(cmd)) {
    Serial.println("$ERR,1,BAD_CRC*00");
    return;
  }
  
  if (cmd.startsWith("$PING")) {
    Serial.println("$PONG,OK*00");
    setLED(true); delay(50); setLED(false);
  }
  else if (cmd.startsWith("$VEL")) {
    if (estopActive) {
      Serial.println("$ACK,ERR,ESTOP*00");
      return;
    }
    Serial.println("$ACK,OK*00");
    setBuzzer(true); delay(50); setBuzzer(false);
  }
  else if (cmd.startsWith("$STO")) {
    stopMotors();
    Serial.println("$ACK,OK*00");
  }
  else if (cmd.startsWith("$LED")) {
    Serial.println("$ACK,OK*00");
  }
  else if (cmd.startsWith("$BUZ")) {
    Serial.println("$ACK,OK*00");
  }
  else {
    Serial.println("$ACK,ERR,UNKNOWN*00");
  }
}

bool validateCRC(String cmd) {
  int starIdx = cmd.indexOf('*');
  if (starIdx < 0 || starIdx + 3 > cmd.length()) return false;
  
  String payload = cmd.substring(1, starIdx);
  String crcStr = cmd.substring(starIdx + 1, starIdx + 3);
  
  uint8_t expected = (uint8_t)strtol(crcStr.c_str(), NULL, 16);
  uint8_t actual = 0;
  for (unsigned int i = 0; i < payload.length(); i++) {
    actual ^= (uint8_t)payload[i];
  }
  return expected == actual;
}
