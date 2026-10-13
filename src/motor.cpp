#include "motor.h"

#include <FastAccelStepper.h>
#include <TMCStepper.h>

#include "config.h"

namespace motor {

static FastAccelStepperEngine engine;
static FastAccelStepper* stepper = nullptr;
// Module is sold as TMC2208 but answers VERSION 0x21 = TMC2209 (register superset).
// MS1 = MS2 = GND -> UART address 0.
static TMC2209Stepper driver(&Serial1, TMC_R_SENSE, 0);
static bool tmcOk = false;
static uint8_t tmcVersion = 0;
static float speedDps = DEFAULT_SPEED_DPS;
static float accelDps2 = DEFAULT_ACCEL_DPS2;

static void configureDriver() {
    driver.begin();
    driver.pdn_disable(true);       // PDN_UART pin is UART, not power-down
    driver.I_scale_analog(false);   // current from UART, not VREF
    driver.mstep_reg_select(true);  // microsteps from UART, not MS1/MS2
    driver.toff(5);
    driver.rms_current(TMC_RMS_MA);
    driver.microsteps(MICROSTEPS);
    driver.intpol(true);            // interpolate to 256
    driver.en_spreadCycle(false);   // stealthChop: quiet
    driver.pwm_autoscale(true);

    // IOIN.VERSION: 0x20 TMC2208, 0x21 TMC2209; anything else = UART not wired
    tmcVersion = driver.version();
    tmcOk = tmcVersion == 0x20 || tmcVersion == 0x21;
}

static void setupDriver() {
    Serial1.begin(115200, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
    configureDriver();
}

bool reinitDriver() {
    configureDriver();
    return tmcOk;
}

uint8_t driverWrites() { return driver.IFCNT(); }

void begin() {
    setupDriver();

    engine.init();
    stepper = engine.stepperConnectToPin(PIN_STEP);
    if (!stepper) {
        Serial.println("motor: stepperConnectToPin failed");
        return;
    }
    stepper->setDirectionPin(PIN_DIR);
    stepper->setEnablePin(PIN_EN, true);
    stepper->setAutoEnable(true);
    stepper->setDelayToDisable(2000);  // keep holding torque 2s after stop

    setSpeed(speedDps);
    setAccel(accelDps2);
}

bool driverOk() { return tmcOk; }
uint8_t driverVersion() { return tmcVersion; }

void setSpeed(float degPerSec) {
    speedDps = degPerSec;
    if (!stepper) return;
    stepper->setSpeedInHz(static_cast<uint32_t>(degPerSec * STEPS_PER_DEG));
    stepper->applySpeedAcceleration();
}

void setAccel(float degPerSec2) {
    accelDps2 = degPerSec2;
    if (!stepper) return;
    stepper->setAcceleration(static_cast<int32_t>(degPerSec2 * STEPS_PER_DEG));
    stepper->applySpeedAcceleration();
}

float speed() { return speedDps; }
float accel() { return accelDps2; }

void rotate(float deg) {
    if (!stepper) return;
    stepper->move(lroundf(deg * STEPS_PER_DEG));
}

void runContinuous(bool forward) {
    if (!stepper) return;
    forward ? stepper->runForward() : stepper->runBackward();
}

void stop() {
    if (stepper) stepper->stopMove();
}

void emergencyStop() {
    if (stepper) stepper->forceStop();
}

bool isRunning() { return stepper && stepper->isRunning(); }

float position() {
    return stepper ? stepper->getCurrentPosition() / STEPS_PER_DEG : 0.0f;
}

void setEnabled(bool on) {
    if (!stepper) return;
    if (on) {
        stepper->setAutoEnable(false);
        stepper->enableOutputs();
    } else {
        stepper->forceStop();
        stepper->disableOutputs();
        stepper->setAutoEnable(true);
    }
}

}  // namespace motor
