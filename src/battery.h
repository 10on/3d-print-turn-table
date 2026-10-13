#pragma once

#include <Arduino.h>

namespace battery {

void     begin();
void     update();      // call from loop()
uint16_t millivolts();  // filtered
bool     present();     // false without the divider (VBAT_SENSE): low/cutoff never trip
uint8_t  percent();     // rough, linear between VBAT_EMPTY_MV and VBAT_FULL_MV; 0xFF if absent
bool     low();
bool     cutoff();      // latched until voltage recovers above VBAT_LOW_MV
void     clearCutoff(); // re-latches on the next sample if still below cutoff

}  // namespace battery
