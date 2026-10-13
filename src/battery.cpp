#include "battery.h"

#include "config.h"

namespace battery {

static float filteredMv = 0.0f;
static bool cutoffLatched = false;
static uint32_t lastSample = 0;

static float sampleMv() {
    return analogReadMilliVolts(PIN_VBAT) * VBAT_DIVIDER * VBAT_CALIBRATION;
}

void begin() {
    if (!VBAT_SENSE) return;
    analogSetPinAttenuation(PIN_VBAT, ADC_11db);
    filteredMv = sampleMv();
}

void update() {
    if (!VBAT_SENSE) return;
    if (millis() - lastSample < 100) return;
    lastSample = millis();

    // EMA ~2 s: motor current pulls the pack down in short spikes
    filteredMv += (sampleMv() - filteredMv) * 0.05f;

    if (filteredMv < VBAT_CUTOFF_MV) {
        cutoffLatched = true;
    } else if (filteredMv > VBAT_LOW_MV) {
        cutoffLatched = false;
    }
}

uint16_t millivolts() { return static_cast<uint16_t>(filteredMv); }
bool low() { return VBAT_SENSE && filteredMv < VBAT_LOW_MV; }
bool present() { return VBAT_SENSE; }

uint8_t percent() {
    if (!VBAT_SENSE) return 0xFF;
    const float pct = (filteredMv - VBAT_EMPTY_MV) * 100.0f / (VBAT_FULL_MV - VBAT_EMPTY_MV);
    return static_cast<uint8_t>(constrain(pct, 0.0f, 100.0f));
}

bool cutoff() { return cutoffLatched; }
void clearCutoff() { cutoffLatched = false; }

}  // namespace battery
