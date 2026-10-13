#pragma once

#include <Arduino.h>

// ESP32-C3 SuperMini. Free GPIO: 0-10, 20, 21.
// Avoid 2/8/9 (strapping; 8 = onboard LED, 9 = BOOT button).
constexpr uint8_t PIN_STEP    = 4;
constexpr uint8_t PIN_DIR     = 3;
constexpr uint8_t PIN_EN      = 5;   // active LOW
constexpr uint8_t PIN_TMC_RX  = 20;  // -> PDN_UART directly
constexpr uint8_t PIN_TMC_TX  = 21;  // -> PDN_UART via 1k
constexpr uint8_t PIN_LED     = 8;   // onboard, active LOW: motor running
constexpr uint8_t PIN_LED_PWR = 6;   // external, active HIGH: battery state
constexpr uint8_t PIN_LED_BLE = 7;   // external, active HIGH: BLE state
constexpr uint8_t PIN_BUZZER  = 0;

// Buzzer module: a tone drives both passive and active buzzers, a DC level
// only active ones. Unknown module -> keep the tone.
// Beeps nonstop when it should be quiet -> low-level trigger (PNP), set ACTIVE_LOW.
constexpr bool BUZZER_PASSIVE    = true;
constexpr bool BUZZER_ACTIVE_LOW = true;   // HM-FMD: PNP (S9012), low-level trigger

// TMC2209 (module sold as TMC2208, see motor.cpp)
constexpr float    TMC_R_SENSE   = 0.11f;  // most StepStick-style modules
constexpr uint16_t TMC_RMS_MA    = 500;    // 20 mm pancake NEMA 17, ~0.7-1.0 A rated: check the label
constexpr uint16_t MICROSTEPS    = 256;    // set over UART; MS1/MS2 stay GND = UART address 0

// Mechanics
constexpr uint16_t MOTOR_FULL_STEPS = 200;   // 1.8 deg NEMA 17
constexpr float    GEAR_RATIO       = 1.0f;  // motor turns per table turn
constexpr float    STEPS_PER_DEG    = MOTOR_FULL_STEPS * MICROSTEPS * GEAR_RATIO / 360.0f;

// Speed presets for the Turntable BLE protocol (remote): 120/60/30/15 s per rev
constexpr float   SPEED_PRESETS_DPS[] = {2.0f, 3.0f, 4.5f, 6.0f, 9.0f, 12.0f, 24.0f, 45.0f};
constexpr uint8_t SPEED_PRESET_COUNT  = sizeof(SPEED_PRESETS_DPS) / sizeof(SPEED_PRESETS_DPS[0]);

// Defaults (table degrees)
constexpr float DEFAULT_SPEED_DPS  = 6.0f;
constexpr float DEFAULT_ACCEL_DPS2 = 60.0f;

// Battery: 3S3P Li-ion, 9.0-12.6 V. Divider VBAT - 220k - PIN_VBAT - 20k - GND
// 12.6 V -> 1.05 V at the pin (C3 ADC is usable up to ~2.5 V)
constexpr bool     VBAT_SENSE        = true;   // false if the divider isn't fitted: GPIO1 floats
constexpr uint8_t  PIN_VBAT          = 1;      // ADC1_CH1
constexpr float    VBAT_DIVIDER      = (220.0f + 20.0f) / 20.0f;
constexpr float    VBAT_CALIBRATION  = 1.0f;   // trim against a multimeter
constexpr uint16_t VBAT_LOW_MV       = 10200;  // 3.4 V/cell: warn
constexpr uint16_t VBAT_CUTOFF_MV    = 9600;   // 3.2 V/cell: stop motor, BMS cuts much lower
constexpr uint16_t VBAT_EMPTY_MV     = 9600;   // 0 %
constexpr uint16_t VBAT_FULL_MV      = 12450;  // 100 %

constexpr const char* BLE_NAME = "MidTable";
