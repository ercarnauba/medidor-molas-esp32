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

bool StepperManager::wasLastHomingSuccessful() const {
    return _lastHomingSuccess;
}

void StepperManager::moveSteps(long steps, StepperDirection dir, uint16_t usDelay) {
    if (steps <= 0) return;

    long maxStepsAbs = (long)(STEPPER_MAX_TRAVEL_MM * _stepsPerMm);

    // ========== INTERTRAVAMENTO DE SEGURANÇA COM SINAL DO MICRO SWITCH ==========
    // Se movimento é BACKWARD (em direção ao home/endstop), verifica se já está acionado
    if (dir == STEPPER_DIR_BACKWARD && isEndstopPressed()) {
        Serial.println("[STEPPER] CRITICO: Micro switch JÁ ESTÁ ACIONADO! Movimento BACKWARD BLOQUEADO para segurança.");
        return;  // Não permite movimento nenhum se endstop já está acionado
    }
    // ===========================================================================

    // Verifica proteção de curso
    if (labs(_positionSteps) > maxStepsAbs) {
        Serial.println("[STEPPER] WARNING: Already at max travel limit, cannot move.");
        return;
    }

    digitalWrite(DIR_PIN, (dir == STEPPER_DIR_FORWARD) ? HIGH : LOW);

    for (long i = 0; i < steps; ++i) {
        // ========== PROTEÇÃO CRÍTICA: Monitorar endstop durante movimento BACKWARD ==========
        // Durante movimento BACKWARD, verifica continuamente o sinal do micro switch
        if (dir == STEPPER_DIR_BACKWARD) {
            if (isEndstopPressed()) {
                Serial.print("[STEPPER] CRITICO: Micro switch ACIONADO durante movimento BACKWARD! ");
                Serial.print("Parado após ");
                Serial.print(i);
                Serial.println(" passos (proteção ativa).");
                break;  // Para imediatamente
            }
        }
        // ====================================================================================
        if (isEndstopPressed()) {
            Serial.println("[STEPPER] Endstop triggered during movement!");
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
            Serial.println("[STEPPER] Max travel limit reached, stopping.");
            break;
        }
    }
}

void StepperManager::homeToEndstop(long maxSteps, uint16_t usDelay) {
    StepperDirection dirHome =
        (STEPPER_HOME_DIR_INT == 0) ? STEPPER_DIR_FORWARD : STEPPER_DIR_BACKWARD;

    digitalWrite(DIR_PIN, (dirHome == STEPPER_DIR_FORWARD) ? HIGH : LOW);

    unsigned long startMs = millis();
    bool homed = false;

    Serial.println("[STEPPER] Starting homing...");

    for (long i = 0; i < maxSteps; ++i) {
        if (isEndstopPressed()) {
            homed = true;
            Serial.println("[STEPPER] Endstop found!");
            break;
        }

        if (millis() - startMs > STEPPER_HOME_TIMEOUT_MS) {
            Serial.println("[STEPPER] ERROR: Homing timeout! Endstop not found or stuck.");
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
        resetPosition();
        _lastHomingSuccess = true;
        Serial.println("[STEPPER] Homing successful, position reset to 0.");
    } else {
        _lastHomingSuccess = false;
        Serial.println("[STEPPER] CRITICAL: Homing failed! Check endstop connection and STEPPER_HOME_DIR_INT.");
    }
}

void StepperManager::moveToPositionMm(float targetMm, uint16_t usDelay) {
    // ========== INTERTRAVAMENTO DE SEGURANÇA COM SINAL DO MICRO SWITCH ==========
    // Se micro switch está acionado, apenas permite movimento FORWARD (longe do home)
    if (isEndstopPressed()) {
        Serial.println("[STEPPER] SEGURANCA: Micro switch ACIONADO - Motor em HOME (posição 0mm).");
        
        // Se target é um movimento BACKWARD ou para posição negativa, bloqueia
        if (targetMm <= 0.0f) {
            Serial.println("[STEPPER] CRITICO: Movimento bloqueado. Motor já está no HOME (endstop acionado).");
            return;
        }
        
        Serial.print("[STEPPER] Permitido: movimento FORWARD para ");
        Serial.print(targetMm, 2);
        Serial.println(" mm (longe do endstop).");
    }
    // ===========================================================================
    
    // Proteção de limite máximo de curso
    if (targetMm < 0.0f) {
        Serial.print("[STEPPER] SEGURANCA: Posição negativa solicitada (");
        Serial.print(targetMm, 2);
        Serial.println(" mm), limitando a 0mm.");
        targetMm = 0.0f;
    }
    
    if (targetMm > STEPPER_MAX_TRAVEL_MM) {
        Serial.print("[STEPPER] SEGURANCA: Posição ultrapassa máximo (");
        Serial.print(targetMm, 2);
        Serial.print(" mm > ");
        Serial.print(STEPPER_MAX_TRAVEL_MM);
        Serial.println(" mm), limitando ao máximo.");
        targetMm = STEPPER_MAX_TRAVEL_MM;
    }
    
    long targetSteps = (long)round(targetMm * _stepsPerMm);
    long deltaSteps  = targetSteps - _positionSteps;

    if (deltaSteps == 0) return;

    StepperDirection dir = (deltaSteps > 0) ? STEPPER_DIR_FORWARD : STEPPER_DIR_BACKWARD;
    long steps = labs(deltaSteps);

    moveSteps(steps, dir, usDelay);
}
