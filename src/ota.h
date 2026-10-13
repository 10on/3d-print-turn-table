#pragma once

#include <stdint.h>

// WiFi + HTTP OTA upload (POST /update), off by default: C3 shares one radio between WiFi and BLE,
// a live STA degrades the remote's link. Turned on by the "w" command.
namespace ota {

enum class State : uint8_t { Off, Connecting, Ready };

void start();
void stop();
void update();  // call from loop()
State state();
bool active();
bool uploadInProgress();
uint32_t ip();  // IPv4, network byte order as in IPAddress; 0 when not Ready

}  // namespace ota
