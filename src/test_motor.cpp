// Bring-up test: swings the table +-90 deg forever, no WiFi/BLE.
// Build/flash: make test-motor

#include <Arduino.h>

#include "battery.h"
#include "config.h"
#include "motor.h"

constexpr float SWING_DEG = 90.0f;
constexpr float TEST_SPEED_DPS = 45.0f;
constexpr float TEST_ACCEL_DPS2 = 90.0f;
constexpr uint32_t PAUSE_MS = 1000;

static void waitMotion() {
    while (motor::isRunning()) {
        digitalWrite(PIN_LED, LOW);
        battery::update();
        delay(10);
    }
    digitalWrite(PIN_LED, HIGH);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("motor test");

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    battery::begin();
    motor::begin();
    motor::setSpeed(TEST_SPEED_DPS);
    motor::setAccel(TEST_ACCEL_DPS2);
    Serial.printf("TMC UART: %s (version 0x%02X)\n", motor::driverOk() ? "ok" : "not responding, standalone mode",
                  motor::driverVersion());
    Serial.printf("speed %.1f deg/s, accel %.1f deg/s2, %.2f steps/deg\n",
                  motor::speed(), motor::accel(), STEPS_PER_DEG);
}

void loop() {
    static bool forward = true;

    Serial.printf("rotate %+.0f", forward ? SWING_DEG : -SWING_DEG);
    if (battery::present()) Serial.printf("  vbat %u mV", battery::millivolts());
    Serial.println();
    motor::rotate(forward ? SWING_DEG : -SWING_DEG);
    waitMotion();
    Serial.printf("pos %.2f deg\n", motor::position());

    forward = !forward;
    delay(PAUSE_MS);
}
