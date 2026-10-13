#pragma once

#include <Arduino.h>

namespace buzzer {

struct Note {
    uint16_t freqHz;  // ignored by an active buzzer
    uint16_t onMs;
    uint16_t offMs;
};

void begin();
void update();  // call from loop()
void play(const Note* notes, uint8_t count);  // replaces whatever is playing

void boot();
void connected();
void disconnected();
void lowBattery();
void cutoff();
void test();

}  // namespace buzzer
