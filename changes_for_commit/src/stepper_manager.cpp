#include "stepper_manager.h"
#include "config.h"

StepperManager stepperManager;

void StepperManager::begin() {
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN,  OUTPUT);
    pinMode(EN_PIN,   OUTPUT);
    pinMode(ENDSTOP_PIN, INPUT_PULLUP);

    setStepsPerMm(STEPPER_STEPS_PER_MM);
    resetPosition();

    // Desabilita o driver inicialmente
    enable(false);
}

void StepperManager::enable(bool on) {
    // Na maioria dos drivers, EN = LOW habilita
    digitalWrite(EN_PIN, on ? LOW : HIGH);
}

bool StepperManager::isEndstopPressed() const {
    // Pressionado = LOW (endstop para GND com pull-up interno)
    return (digitalRead(ENDSTOP_PIN) == LOW);
}

void StepperManager::setStepsPerMm(float s) {
    _stepsPerMm = s;
}

float StepperManager::getStepsPerMm() const {
    return _stepsPerMm;
}

void StepperManager::resetPosition() {
    _positionSteps = 0;
}

float StepperManager::getPositionMm() const {
    return _positionSteps / _stepsPerMm;
}

void StepperManager::moveSteps(long steps, StepperDirection dir, uint16_t usDelay) {
    if (steps <= 0) return;

    long maxStepsAbs = (long)(STEPPER_MAX_TRAVEL_MM * _stepsPerMm);

    digitalWrite(DIR_PIN, (dir == STEPPER_DIR_FORWARD) ? HIGH : LOW);

    for (long i = 0; i < steps; ++i) {
        // Proteção: se endstop acionado, interrompe
        if (isEndstopPressed()) {
            break;
        }

        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(usDelay);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(usDelay);

        // Atualiza posição em passos
        if (dir == STEPPER_DIR_FORWARD) {
            _positionSteps++;
        } else {
            _positionSteps--;
        }

        // Proteção de curso máximo
        if (labs(_positionSteps) > maxStepsAbs) {
            break;
        }
    }
}

void StepperManager::homeToEndstop(long maxSteps, uint16_t usDelay) {
    StepperDirection dirHome =
        (STEPPER_HOME_DIR_INT == 0) ? STEPPER_DIR_FORWARD : STEPPER_DIR_BACKWARD;

    digitalWrite(DIR_PIN, (dirHome == STEPPER_DIR_FORWARD) ? HIGH : LOW);

    const unsigned long HOME_TIMEOUT_MS = 20000; // 20s
    unsigned long startMs = millis();
    bool homed = false;

    for (long i = 0; i < maxSteps; ++i) {
        if (isEndstopPressed()) {
            homed = true;
            break;
        }

        if (millis() - startMs > HOME_TIMEOUT_MS) {
            Serial.println("[STEPPER] Home timeout");
            break;
        }

        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(usDelay);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(usDelay);

        if (dirHome == STEPPER_DIR_FORWARD) {
            _positionSteps++;
        } else {
            _positionSteps--;
        }
    }

    if (homed) {
        // Após o home, define posição como zero de referência
        resetPosition();
    } else {
        Serial.println("[STEPPER] Home not achieved; position not reset.");
    }
}

void StepperManager::moveToPositionMm(float targetMm, uint16_t usDelay) {
    long targetSteps = (long)round(targetMm * _stepsPerMm);
    long deltaSteps  = targetSteps - _positionSteps;

    if (deltaSteps == 0) return;

    StepperDirection dir = (deltaSteps > 0) ? STEPPER_DIR_FORWARD : STEPPER_DIR_BACKWARD;
    long steps = labs(deltaSteps);

    moveSteps(steps, dir, usDelay);
}
