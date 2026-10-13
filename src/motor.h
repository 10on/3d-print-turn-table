#pragma once

#include <Arduino.h>

namespace motor {

void  begin();
bool  driverOk();
uint8_t driverVersion();          // IOIN.VERSION, 0x21 = TMC2209
bool  reinitDriver();              // re-send UART config, e.g. after VM came up late
uint8_t driverWrites();            // IFCNT: UART writes the driver accepted

void  setSpeed(float degPerSec);
void  setAccel(float degPerSec2);
float speed();
float accel();

void  rotate(float deg);           // relative, table degrees
void  runContinuous(bool forward);
void  stop();                      // decelerate
void  emergencyStop();

bool  isRunning();
float position();                  // table degrees since boot
void  setEnabled(bool on);

}  // namespace motor
