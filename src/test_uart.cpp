// Bring-up test: raw single-wire UART to the TMC2208, no libraries.
// Sends a read request for IOIN every second and dumps what comes back.
// GPIO21 (TX, via 1k) and GPIO20 (RX) share PDN_UART, so our own 4 bytes
// echo back first, then the driver's 8-byte reply.
// Build/flash: make test-uart

#include <Arduino.h>

#include "config.h"

constexpr uint8_t REG_GCONF = 0x00;
constexpr uint8_t REG_IFCNT = 0x02;
constexpr uint8_t REG_IOIN  = 0x06;

static uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        for (int j = 0; j < 8; j++) {
            if ((crc >> 7) ^ (b & 0x01)) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
            b >>= 1;
        }
    }
    return crc;
}

static void readRegister(uint8_t reg, const char* name) {
    uint8_t req[4] = {0x05, 0x00, reg, 0};
    req[3] = crc8(req, 3);

    while (Serial1.available()) Serial1.read();
    Serial1.write(req, sizeof(req));
    Serial1.flush();

    uint8_t buf[16];
    size_t n = 0;
    const uint32_t start = millis();
    while (millis() - start < 30 && n < sizeof(buf)) {
        if (Serial1.available()) buf[n++] = Serial1.read();
    }

    Serial.printf("%-6s %2u bytes:", name, n);
    for (size_t i = 0; i < n; i++) Serial.printf(" %02X", buf[i]);
    Serial.println();

    if (n == 0) {
        Serial.println("  no echo: GPIO21 -> 1k -> PDN_UART <- GPIO20 not connected");
        return;
    }
    if (n < 12) {
        Serial.println("  echo only: driver silent (VM off? PDN jumper? VIO?)");
        return;
    }

    const uint8_t* r = buf + 4;
    const bool crcOk = crc8(r, 7) == r[7];
    const uint32_t value = (uint32_t(r[3]) << 24) | (uint32_t(r[4]) << 16) | (uint32_t(r[5]) << 8) | r[6];
    Serial.printf("  value 0x%08lX  crc %s\n", (unsigned long)value, crcOk ? "ok" : "BAD");
    if (reg == REG_IOIN) {
        Serial.printf("  version 0x%02lX (0x20 = TMC2208), ENN %lu, MS1 %lu, MS2 %lu\n",
                      (unsigned long)(value >> 24), (unsigned long)(value & 0x01),
                      (unsigned long)((value >> 2) & 0x01), (unsigned long)((value >> 3) & 0x01));
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("tmc2208 uart test");

    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, HIGH);  // keep the motor off

    Serial1.begin(115200, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
}

void loop() {
    readRegister(REG_IOIN, "IOIN");
    readRegister(REG_GCONF, "GCONF");
    readRegister(REG_IFCNT, "IFCNT");
    Serial.println();
    delay(1000);
}
