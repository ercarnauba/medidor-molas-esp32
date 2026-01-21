#include "tmc2209_manager.h"
#include "config.h"

TMC2209Manager tmc2209Manager;

bool TMC2209Manager::begin() {
    Serial.println("[TMC2209] Desativado (usando apenas STEP/DIR, sem UART)");
    return false;
}

void TMC2209Manager::setCurrent(uint16_t currentRMS, uint16_t currentHold) {}
void TMC2209Manager::setMicrosteps(uint16_t microsteps) {}
void TMC2209Manager::setStallGuardThreshold(uint8_t threshold) {}
bool TMC2209Manager::isStallDetected() { return false; }
void TMC2209Manager::clearStall() {}
bool TMC2209Manager::hasUntreatedStall() const { return false; }
void TMC2209Manager::markStallTreated() {}
int TMC2209Manager::getStallGuardValue() { return -1; }
bool TMC2209Manager::isCommunicationOK() { return false; }
void TMC2209Manager::enableStealthChop(bool enable) {}
String TMC2209Manager::getDiagnostics() { return "Modo STEP/DIR - sem diagnosticos"; }

