#include <Arduino.h>

#include "battery.h"
#include "ble.h"
#include "buzzer.h"
#include "commands.h"
#include "config.h"
#include "leds.h"
#include "motor.h"
#include "ota.h"

static void pollSerial() {
    static String buf;
    while (Serial.available()) {
        const char c = Serial.read();
        if (c == '\n' || c == '\r') {
            commands::handle(buf);
            buf = "";
        } else if (buf.length() < 64) {
            buf += c;
        }
    }
}

static void checkBattery() {
    static bool wasCutoff = false;
    static uint32_t lastLowBeep = 0;

    if (battery::cutoff() && !wasCutoff) {
        commands::reply("battery cutoff %u mV, motor disabled\n", battery::millivolts());
        motor::setEnabled(false);
        buzzer::cutoff();
    } else if (battery::low() && !battery::cutoff() && millis() - lastLowBeep > 60000) {
        lastLowBeep = millis();
        buzzer::lowBattery();
    }
    wasCutoff = battery::cutoff();
}

static void checkBle() {
    static bool wasConnected = false;
    if (ble::connected() != wasConnected) {
        wasConnected = ble::connected();
        wasConnected ? buzzer::connected() : buzzer::disconnected();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("mid_table boot, reset reason: %d\n", esp_reset_reason());

    battery::begin();
    leds::begin();
    buzzer::begin();
    if (battery::present()) Serial.printf("VBAT: %u mV\n", battery::millivolts());

    motor::begin();
    Serial.printf("TMC UART: %s (version 0x%02X)\n", motor::driverOk() ? "ok" : "not responding, standalone mode",
                  motor::driverVersion());
    commands::begin();

    ble::begin(BLE_NAME);
    buzzer::boot();
}

void loop() {
    ota::update();
    battery::update();
    checkBattery();
    checkBle();
    buzzer::update();

    pollSerial();
    String line;
    while (ble::poll(line)) {
        commands::handle(line);
    }

    commands::publishStatus();
    leds::update();
}
