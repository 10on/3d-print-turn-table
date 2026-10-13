#include "leds.h"

#include <Arduino.h>

#include "battery.h"
#include "ble.h"
#include "config.h"
#include "motor.h"
#include "ota.h"

namespace leds {

static bool blink(uint32_t periodMs) {
    return (millis() % periodMs) < periodMs / 2;
}

void begin() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_LED_PWR, OUTPUT);
    pinMode(PIN_LED_BLE, OUTPUT);
    update();
}

void update() {
    digitalWrite(PIN_LED, motor::isRunning() ? LOW : HIGH);

    // OTA mode: LEDs alternate. Slow = connecting WiFi, fast = ready, very fast = uploading
    if (ota::active()) {
        const uint32_t period = ota::uploadInProgress()                  ? 150
                                : ota::state() == ota::State::Ready ? 500
                                                                    : 1200;
        const bool phase = blink(period);
        digitalWrite(PIN_LED_PWR, phase);
        digitalWrite(PIN_LED_BLE, !phase);
        return;
    }

    // Power: solid = ok, slow blink = low, fast blink = cutoff
    bool pwr = true;
    if (battery::cutoff()) {
        pwr = blink(250);
    } else if (battery::low()) {
        pwr = blink(1000);
    }
    digitalWrite(PIN_LED_PWR, pwr);

    // BLE: blink = advertising, solid = connected
    digitalWrite(PIN_LED_BLE, ble::connected() ? true : blink(1000));
}

}  // namespace leds
