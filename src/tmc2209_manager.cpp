#include "tmc2209_manager.h"
#include "config.h"

TMC2209Manager tmc2209Manager;

bool TMC2209Manager::begin() {
#ifdef ARDUINO_ARCH_ESP32
    if (!TMC_UART_ENABLED) {
        return false;
    }

    static HardwareSerial serialTMC(2);
    serialTMC.begin(TMC_UART_BAUD, SERIAL_8N1, TMC_UART_RX_PIN, TMC_UART_TX_PIN);
    _serial = &serialTMC;

    _driver = new TMC2209Stepper(_serial, TMC_R_SENSE, TMC_UART_ADDR);
    _driver->begin();
    _driver->pdn_disable(true);       // PDN pin high -> habilita UART
    _driver->I_scale_analog(false);   // Usa VIO para current scale
    _driver->toff(4);                 // Driver ON (chopper)

    // Corrente RMS e hold (holdMultiplier usa fração da RMS)
    float holdMult = (float)TMC_CURRENT_HOLD / (float)TMC_CURRENT_RMS;
    holdMult = constrain(holdMult, 0.0f, 1.0f);
    _driver->rms_current(TMC_CURRENT_RMS, holdMult);

    // Microsteps
    _driver->microsteps(TMC_DEFAULT_MICROSTEPS);

    // StallGuard threshold
    _driver->SGTHRS(TMC_STALLGUARD_THRESHOLD);

    // Modo silencioso padrão
    _driver->en_spreadCycle(false); // false -> stealthChop ON

    pinMode(TMC_DIAG_PIN, INPUT_PULLUP);

    _stallDetectedFlag = false;
    _stallUntreated = false;
    _lastStallTime = 0;

    int conn = _driver->test_connection();
    if (conn != 0) {
        return false;
    }

    Serial.println("[TMC2209] UART inicializada");
    return true;
#else
    return false;
#endif
}

void TMC2209Manager::setCurrent(uint16_t currentRMS, uint16_t currentHold) {
    if (!_driver) return;
    float holdMult = (float)currentHold / (float)currentRMS;
    holdMult = constrain(holdMult, 0.0f, 1.0f);
    _driver->rms_current(currentRMS, holdMult);
}

void TMC2209Manager::setMicrosteps(uint16_t microsteps) {
    if (!_driver) return;
    _driver->microsteps(microsteps);
}

void TMC2209Manager::setStallGuardThreshold(uint8_t threshold) {
    if (!_driver) return;
    _driver->SGTHRS(threshold);
}

bool TMC2209Manager::isStallDetected() {
    if (!_driver) return false;

    bool detected = (digitalRead(TMC_DIAG_PIN) == LOW);
    if (detected) {
        _stallDetectedFlag = true;
        _stallUntreated = true;
        _lastStallTime = millis();
    }
    return detected;
}

void TMC2209Manager::clearStall() {
    _stallDetectedFlag = false;
}

bool TMC2209Manager::hasUntreatedStall() const {
    return _stallUntreated;
}

void TMC2209Manager::markStallTreated() {
    _stallUntreated = false;
}

int TMC2209Manager::getStallGuardValue() {
    if (!_driver) return -1;
    return _driver->SG_RESULT();
}

bool TMC2209Manager::isCommunicationOK() {
    if (!_driver) return false;
    return (_driver->test_connection() == 0);
}

void TMC2209Manager::enableStealthChop(bool enable) {
    if (!_driver) return;
    _driver->en_spreadCycle(!enable);
}

String TMC2209Manager::getDiagnostics() {
    if (!_driver) {
        return String("[TMC2209] UART desabilitada (TMC_UART_ENABLED=false)");
    }

    String diag;
    diag.reserve(128);
    diag += "[TMC2209] ";
    diag += isCommunicationOK() ? "OK" : "FAIL";
    diag += " | RMS="; diag += TMC_CURRENT_RMS;
    diag += "mA | HOLD="; diag += TMC_CURRENT_HOLD; diag += "mA";
    diag += " | uSteps="; diag += TMC_DEFAULT_MICROSTEPS;
    diag += " | SGTHRS="; diag += TMC_STALLGUARD_THRESHOLD;
    diag += " | SG_RESULT="; diag += getStallGuardValue();
    return diag;
}

