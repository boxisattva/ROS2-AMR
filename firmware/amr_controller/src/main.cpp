#include <Arduino.h>
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(25, OUTPUT);  // LED Ready
    pinMode(26, OUTPUT);  // LED Found
    digitalWrite(25, HIGH);
    
    Serial.println("=== FULL I2C SCAN ===");
    
    Wire.begin();
    
    // Пробуем разные скорости
    const uint32_t SPEEDS[] = {50000, 100000, 400000};
    const char* SPEED_NAMES[] = {"50kHz", "100kHz", "400kHz"};
    
    for (int s = 0; s < 3; s++) {
        Wire.setClock(SPEEDS[s]);
        Serial.print("Speed: ");
        Serial.println(SPEED_NAMES[s]);
        
        int nDevices = 0;
        
        for (byte addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            byte error = Wire.endTransmission();
            
            if (error == 0) {
                Serial.print("  FOUND: 0x");
                if (addr < 16) Serial.print("0");
                Serial.println(addr, HEX);
                digitalWrite(26, HIGH);
                nDevices++;
            }
        }
        
        Serial.print("  Devices: ");
        Serial.println(nDevices);
        delay(100);
    }
    
    // Попробуем "blind write" на 0x40 (PCA9685 default)
    Serial.println("=== Blind write to 0x40 ===");
    Wire.beginTransmission(0x40);
    Wire.write(0x00);  // MODE1
    Wire.write(0x00);  // Wake up
    byte err = Wire.endTransmission();
    Serial.print("Result: ");
    Serial.println(err);
    
    // Проверим, ответит ли сейчас
    Wire.beginTransmission(0x40);
    err = Wire.endTransmission();
    Serial.print("After wake: ");
    Serial.println(err);
    
    Serial.println("=== Done ===");
}

void loop() {
    delay(10000);
}
