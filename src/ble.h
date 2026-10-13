#pragma once

#include <Arduino.h>

// Two GATT services feeding the same text command queue:
//  - Turntable protocol of the BT remote (xiao-camera-remote,
//    devices/turntable_device.cpp) under its own service 7ab1e101-...-cafe:
//    the remote tells tables apart by service UUID. Binary commands are
//    translated to text ones
//  - Nordic UART Service: text commands for debugging from a phone
namespace ble {

void begin(const char* name);
bool connected();
bool poll(String& line);       // next received command, call from loop()
void send(const char* text);   // NUS TX, no-op when nobody is connected
void setStatus(const uint8_t* data, size_t len);  // Turntable Status, notifies

}  // namespace ble
