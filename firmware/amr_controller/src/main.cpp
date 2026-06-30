#include <Arduino.h>
#include <string.h>
#include <esp_system.h>

// ============================================================================
// NAVBOT BASE — Mock mode (без моторов), с диагностикой ребута
// ============================================================================

#define SERIAL_BAUD   115200
#define HEARTBEAT_MS  500
#define WATCHDOG_MS   200

// LED
#define PIN_LED_READY    25
#define PIN_LED_ESTOP    26
#define PIN_LED_WATCHDOG 32
#define PIN_LED_MOTION   33

// Buzzer
#define PIN_BUZZER 14

// E-Stop
#define PIN_ESTOP 4

// FSM
#define BUF_SIZE 64

enum ParserState { WAIT_DOLLAR, READ_PAYLOAD, READ_CRC1, READ_CRC2, VALIDATE };

struct Command {
    char type[8];
    float args[4];
    int arg_count;
    uint8_t rx_crc;
    bool valid;
};

class UartParser {
public:
    UartParser() { reset(); }
    void reset() {
        state = WAIT_DOLLAR;
        buf_idx = 0;
        memset(buffer, 0, BUF_SIZE);
    }
    bool feed(char c, Command& cmd) {
        switch (state) {
            case WAIT_DOLLAR:
                if (c == '$') { state = READ_PAYLOAD; buf_idx = 0; memset(buffer, 0, BUF_SIZE); }
                break;
            case READ_PAYLOAD:
                if (c == '*') { buffer[buf_idx] = '\0'; state = READ_CRC1; }
                else if (buf_idx < BUF_SIZE - 1) buffer[buf_idx++] = c;
                else reset();
                break;
            case READ_CRC1:
                if (isxdigit(c)) { rx_crc = hexValue(c) << 4; state = READ_CRC2; }
                else reset();
                break;
            case READ_CRC2:
                if (isxdigit(c)) { rx_crc |= hexValue(c); state = VALIDATE; }
                else reset();
                break;
            case VALIDATE:
                if (c == '\n' || c == '\r') { /* skip */ }
                return parse(buffer, rx_crc, cmd);
        }
        return false;
    }
private:
    ParserState state;
    char buffer[BUF_SIZE];
    uint8_t buf_idx;
    uint8_t rx_crc;
    uint8_t hexValue(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    }
    uint8_t calcCRC(const char* data) {
        uint8_t crc = 0;
        while (*data) crc ^= (uint8_t)(*data++);
        return crc;
    }
    bool parse(char* payload, uint8_t expected_crc, Command& cmd) {
        if (calcCRC(payload) != expected_crc) {
            strcpy(cmd.type, "ERR");
            cmd.valid = false;
            reset();
            return true;
        }
        char* token = strtok(payload, ",");
        if (!token) { cmd.valid = false; reset(); return true; }
        strncpy(cmd.type, token, 7);
        cmd.type[7] = '\0';
        cmd.arg_count = 0;
        while ((token = strtok(NULL, ",")) && cmd.arg_count < 4) {
            cmd.args[cmd.arg_count++] = atof(token);
        }
        cmd.valid = true;
        reset();
        return true;
    }
};

// Состояние
bool estop_active = false;
bool motors_enabled = false;
unsigned long last_cmd_time = 0;
UartParser parser;
Command cmd;

// Прототипы
void printResetReason();
void initPins();
void setLED(int r, int e, int w, int m);
void checkEStop();
void processSerial();
void checkWatchdog();
void heartbeatLED();
void dispatch(const Command& cmd);

// ============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    // ДИАГНОСТИКА: причина ребута
    printResetReason();
    
    initPins();
    setLED(1, 0, 0, 0);
    
    // Buzzer короткий писк (без tone(), только digitalWrite)
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    
    Serial.println("$PONG,INIT,READY*00");
}

void loop() {
    checkEStop();
    processSerial();
    checkWatchdog();
    heartbeatLED();
}

// ============================================================================
void printResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.print("RESET_REASON: ");
    switch(reason) {
        case ESP_RST_POWERON:  Serial.println("POWERON"); break;
        case ESP_RST_SW:       Serial.println("SOFTWARE"); break;
        case ESP_RST_PANIC:    Serial.println("PANIC/EXCEPTION"); break;
        case ESP_RST_BROWNOUT: Serial.println("BROWNOUT"); break;
        case ESP_RST_WDT:      Serial.println("WATCHDOG"); break;
        case ESP_RST_SDIO:     Serial.println("SDIO"); break;
        default:               Serial.println("OTHER"); break;
    }
}

void initPins() {
    pinMode(PIN_LED_READY, OUTPUT);    digitalWrite(PIN_LED_READY, LOW);
    pinMode(PIN_LED_ESTOP, OUTPUT);    digitalWrite(PIN_LED_ESTOP, LOW);
    pinMode(PIN_LED_WATCHDOG, OUTPUT); digitalWrite(PIN_LED_WATCHDOG, LOW);
    pinMode(PIN_LED_MOTION, OUTPUT);   digitalWrite(PIN_LED_MOTION, LOW);
    pinMode(PIN_BUZZER, OUTPUT);       digitalWrite(PIN_BUZZER, LOW);
    pinMode(PIN_ESTOP, INPUT_PULLUP);
}

void setLED(int r, int e, int w, int m) {
    digitalWrite(PIN_LED_READY,    r ? HIGH : LOW);
    digitalWrite(PIN_LED_ESTOP,    e ? HIGH : LOW);
    digitalWrite(PIN_LED_WATCHDOG, w ? HIGH : LOW);
    digitalWrite(PIN_LED_MOTION,   m ? HIGH : LOW);
}

void checkEStop() {
    bool pressed = (digitalRead(PIN_ESTOP) == LOW);
    if (pressed && !estop_active) {
        estop_active = true;
        motors_enabled = false;
        setLED(0, 1, 0, 0);
        Serial.println("$STA,ESTOP,TRIGGERED*00");
    } else if (!pressed && estop_active) {
        estop_active = false;
        setLED(1, 0, 0, 0);
        Serial.println("$STA,ESTOP,CLEARED*00");
    }
}

void processSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        if (parser.feed(c, cmd)) {
            if (cmd.valid) dispatch(cmd);
            else Serial.println("$ACK,ERR,BAD_CRC*00");
        }
    }
}

void checkWatchdog() {
    if (motors_enabled && (millis() - last_cmd_time > WATCHDOG_MS)) {
        motors_enabled = false;
        setLED(0, 0, 1, 0);
        Serial.println("$STA,WATCHDOG,TIMEOUT*00");
    }
}

void heartbeatLED() {
    if (!estop_active && !motors_enabled) {
        bool pulse = ((millis() / HEARTBEAT_MS) % 2 == 0);
        setLED(pulse ? 1 : 0, 0, 0, 0);
    }
}

void dispatch(const Command& cmd) {
    if (strcmp(cmd.type, "VEL") == 0) {
        if (estop_active) {
            Serial.println("$ACK,ERR,ESTOP*00");
            return;
        }
        float v = cmd.args[0];
        float w = cmd.args[1];
        motors_enabled = true;
        last_cmd_time = millis();
        setLED(0, 0, 0, 1);
        Serial.println("$ACK,OK*00");
    }
    else if (strcmp(cmd.type, "LED") == 0) {
        setLED((int)cmd.args[0], (int)cmd.args[1], (int)cmd.args[2], (int)cmd.args[3]);
        Serial.println("$ACK,OK*00");
    }
    else if (strcmp(cmd.type, "BUZ") == 0) {
        // Без tone() — только digitalWrite
        digitalWrite(PIN_BUZZER, HIGH);
        delay((int)cmd.args[1]);
        digitalWrite(PIN_BUZZER, LOW);
        Serial.println("$ACK,OK*00");
    }
    else if (strcmp(cmd.type, "STO") == 0) {
        motors_enabled = false;
        setLED(0, 0, 0, 0);
        Serial.println("$ACK,OK*00");
    }
    else if (strcmp(cmd.type, "PING") == 0) {
        Serial.println("$PONG,OK*00");
    }
    else {
        Serial.println("$ACK,ERR,UNKNOWN*00");
    }
}