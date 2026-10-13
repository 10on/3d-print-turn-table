#include "buzzer.h"

#include "config.h"

namespace buzzer {

static const Note* melody = nullptr;
static uint8_t melodyLen = 0;
static uint8_t index = 0;
static bool sounding = false;
static uint32_t stepAt = 0;

static void on(uint16_t freqHz) {
    if (BUZZER_PASSIVE) {
        tone(PIN_BUZZER, freqHz);
    } else {
        digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_LOW ? LOW : HIGH);
    }
}

static void off() {
    if (BUZZER_PASSIVE) {
        noTone(PIN_BUZZER);  // detaches LEDC, pin is left LOW
        pinMode(PIN_BUZZER, OUTPUT);
    }
    // Idle level matters: LOW keeps a low-level trigger module sounding
    digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_LOW ? HIGH : LOW);
}

void begin() {
    pinMode(PIN_BUZZER, OUTPUT);
    off();
}

void play(const Note* notes, uint8_t count) {
    off();
    melody = notes;
    melodyLen = count;
    index = 0;
    sounding = false;
    stepAt = millis();
}

void update() {
    if (!melody || millis() < stepAt) return;

    if (sounding) {
        off();
        sounding = false;
        stepAt = millis() + melody[index].offMs;
        if (++index >= melodyLen) melody = nullptr;
        return;
    }

    on(melody[index].freqHz);
    sounding = true;
    stepAt = millis() + melody[index].onMs;
}

static const Note BOOT[]         = {{2000, 60, 0}};
static const Note CONNECTED[]    = {{1800, 50, 40}, {2600, 50, 0}};
static const Note DISCONNECTED[] = {{2600, 50, 40}, {1800, 50, 0}};
static const Note LOW_BATTERY[]  = {{1500, 120, 0}};
static const Note CUTOFF[]       = {{1200, 200, 100}, {1200, 200, 100}, {1200, 200, 0}};
static const Note TEST[]         = {{1000, 150, 50}, {2000, 150, 50}, {3000, 150, 0}};

void boot() { play(BOOT, 1); }
void connected() { play(CONNECTED, 2); }
void disconnected() { play(DISCONNECTED, 2); }
void lowBattery() { play(LOW_BATTERY, 1); }
void cutoff() { play(CUTOFF, 3); }
void test() { play(TEST, 3); }

}  // namespace buzzer
