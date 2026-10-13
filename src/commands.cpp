#include "commands.h"

#include <Preferences.h>
#include <stdarg.h>

#include "battery.h"
#include "ble.h"
#include "buzzer.h"
#include "config.h"
#include "motor.h"
#include "ota.h"

namespace commands {

static Preferences prefs;
static bool forward = true;

// Nearest preset for the remote's "2/4" display, speed may be set freely over NUS
static uint8_t speedIndex() {
    uint8_t best = 0;
    for (uint8_t i = 1; i < SPEED_PRESET_COUNT; i++) {
        if (fabsf(SPEED_PRESETS_DPS[i] - motor::speed()) < fabsf(SPEED_PRESETS_DPS[best] - motor::speed())) {
            best = i;
        }
    }
    return best;
}

static void setSpeed(float dps) {
    motor::setSpeed(dps);
    prefs.putFloat("spd", dps);
}

void begin() {
    prefs.begin("table", false);
    motor::setSpeed(prefs.getFloat("spd", DEFAULT_SPEED_DPS));
    motor::setAccel(prefs.getFloat("acc", DEFAULT_ACCEL_DPS2));
}

void reply(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.print(buf);
    ble::send(buf);
}

void publishStatus() {
    static uint32_t last = 0;
    if (millis() - last < 200) return;
    last = millis();

    uint8_t state = motor::isRunning() ? 1 : 0;
    if (ota::active()) state = 2;
    if (battery::cutoff()) state = 3;

    const uint32_t ip = ota::ip();
    const uint8_t status[10] = {
        state,
        static_cast<uint8_t>(forward ? 0 : 1),
        speedIndex(),
        SPEED_PRESET_COUNT,
        battery::percent(),
        static_cast<uint8_t>((battery::low() ? 0x01 : 0) | (battery::cutoff() ? 0x02 : 0) |
                             (motor::driverOk() ? 0x04 : 0) |
                             (ota::state() == ota::State::Ready ? 0x08 : 0)),
        // IPv4 while OTA is ready, a.b.c.d in byte order
        static_cast<uint8_t>(ip), static_cast<uint8_t>(ip >> 8),
        static_cast<uint8_t>(ip >> 16), static_cast<uint8_t>(ip >> 24),
    };
    ble::setStatus(status, sizeof(status));
}

void printStatus() {
    char vbat[24] = "n/a";
    if (battery::present()) {
        snprintf(vbat, sizeof(vbat), "%u%s", battery::millivolts(),
                 battery::cutoff() ? " CUTOFF" : battery::low() ? " LOW" : "");
    }
    static const char* const otaText[] = {"off", "connecting", "ready"};
    reply("pos %.2f\nspeed %.1f\naccel %.1f\nrunning %d\nvbat %s\ntmc %s\nota %s\n",
          motor::position(), motor::speed(), motor::accel(), motor::isRunning(), vbat,
          motor::driverOk() ? "ok" : "n/a", otaText[static_cast<uint8_t>(ota::state())]);
}

static void printHelp() {
    reply("r <deg>    rotate relative\n"
          "s <deg/s>  set speed\n"
          "p <0..%u>   speed preset\n"
          "a <deg/s2> set accel\n"
          "f / b      run forward / backward\n"
          "x          stop (decelerate)\n"
          "!          emergency stop\n"
          "e / d      enable (hold) / disable motor\n"
          "c          clear battery cutoff\n"
          "t          re-init TMC driver over UART\n"
          "w [0]      WiFi + OTA on (off after 5 min idle) / off\n"
          "z          buzzer test\n"
          "?          status\n",
          SPEED_PRESET_COUNT - 1);
}

void handle(String line) {
    line.trim();
    if (line.isEmpty()) return;
    Serial.printf("> %s\n", line.c_str());

    const char cmd = line[0];
    const float arg = line.substring(1).toFloat();

    if (battery::cutoff() && (cmd == 'r' || cmd == 'f' || cmd == 'b' || cmd == 'e')) {
        reply("err battery cutoff %u mV\n", battery::millivolts());
        return;
    }

    switch (cmd) {
        case 'r':
            forward = arg >= 0;
            motor::rotate(arg);
            break;
        case 's':
            if (arg <= 0) { reply("err speed\n"); return; }
            setSpeed(arg);
            break;
        case 'p': {
            const int idx = static_cast<int>(arg);
            if (idx < 0 || idx >= SPEED_PRESET_COUNT) { reply("err preset\n"); return; }
            setSpeed(SPEED_PRESETS_DPS[idx]);
            break;
        }
        case 'a':
            if (arg <= 0) { reply("err accel\n"); return; }
            motor::setAccel(arg);
            prefs.putFloat("acc", arg);
            break;
        case 'w':
            if (line.length() > 1 && arg == 0) ota::stop(); else ota::start();
            return;
        case 'f': forward = true; motor::runContinuous(true); break;
        case 'b': forward = false; motor::runContinuous(false); break;
        case 'c': battery::clearCutoff(); break;
        case 'x': motor::stop(); break;
        case '!': motor::emergencyStop(); break;
        case 'e': motor::setEnabled(true); break;
        case 'd': motor::setEnabled(false); break;
        case '?': printStatus(); return;
        case 'z': buzzer::test(); break;
        case 't': {
            const bool ok = motor::reinitDriver();
            reply("tmc %s version 0x%02X ifcnt %u\n", ok ? "ok" : "n/a", motor::driverVersion(),
                  motor::driverWrites());
            return;
        }
        default: printHelp(); return;
    }
    reply("ok\n");
}

}  // namespace commands
