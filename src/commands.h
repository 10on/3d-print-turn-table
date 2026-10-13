#pragma once

#include <Arduino.h>

namespace commands {

void begin();                       // restore saved speed/accel
void handle(String line);           // call from loop() only
void reply(const char* fmt, ...);   // Serial + BLE
void printStatus();
void publishStatus();               // Turntable Status char, call from loop()

}  // namespace commands
