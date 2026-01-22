#include "stepper_manager.h"
#include "config.h"

StepperManager stepperManager;

void StepperManager::begin() {
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN,  OUTPUT);
    pinMode(EN_PIN,   OUTPUT);
    pinMode(ENDSTOP_PIN, INPUT_PULLUP);
    
    // Estados iniciais - EN_PIN = HIGH desabilita no A4988 (padrão)
    digitalWrite(STEP_PIN, LOW);
    digitalWrite(DIR_PIN, LOW);
    digitalWrite(EN_PIN, HIGH);  // Desabilitado inicialmente
    
    Serial.println("[STEPPER] Driver STEP/DIR inicializado (TMC2209 em STEP/DIR)");
    Serial.print("  STEP_PIN (GPIO"); Serial.print(STEP_PIN); Serial.println(") = OUTPUT LOW");
    Serial.print("  DIR_PIN (GPIO"); Serial.print(DIR_PIN); Serial.println(") = OUTPUT LOW");
    Serial.print("  EN_PIN (GPIO"); Serial.print(EN_PIN); Serial.println(") = OUTPUT HIGH (disabled)");

    setStepsPerMm(STEPPER_STEPS_PER_MM);
    resetPosition();

    // Habilita o driver
    delay(100);
    enable(true);
    Serial.println("[STEPPER] Motor driver ENABLED!");
}

void StepperManager::enable(bool on) {
    // EN = LOW habilita o A4988 (padrão)
    digitalWrite(EN_PIN, on ? LOW : HIGH);
    Serial.print("[STEPPER] Motor ");
    Serial.print(on ? "ENABLED" : "DISABLED");
    Serial.print(" (EN_PIN=");
    Serial.print(on ? "LOW" : "HIGH");
    Serial.println(")");
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
    
    Serial.print("[STEPPER] moveSteps: passos=");
    Serial.print(steps);
    Serial.print(", direcao=");
    Serial.print(dir == STEPPER_DIR_FORWARD ? "AVANCO" : "RETROCESSO");
    Serial.print(", atraso=");
    Serial.print(usDelay);
    Serial.println(" µs");

    long maxStepsAbs = (long)(STEPPER_MAX_TRAVEL_MM * _stepsPerMm);

    // Segurança: endstop
    if (dir == STEPPER_DIR_BACKWARD && isEndstopPressed()) {
        Serial.println("[STEPPER] SEGURANCA: Micro switch ja pressionado. Movimento bloqueado.");
        return;
    }

    // Proteção de curso
    if (labs(_positionSteps) > maxStepsAbs) {
        Serial.println("[STEPPER] AVISO: Limite de curso maximo atingido.");
        return;
    }

    // Define direção (invertido para TMC2209 no seu hardware: HIGH = FORWARD)
    digitalWrite(DIR_PIN, (dir == STEPPER_DIR_FORWARD) ? HIGH : LOW);
    delayMicroseconds(20);  // Setup time para mudar direção

    // Gera pulsos STEP
    for (long i = 0; i < steps; i++) {
        // Verifica endstop durante movimento
        if (dir == STEPPER_DIR_BACKWARD && isEndstopPressed()) {
            Serial.print("[STEPPER] Micro switch acionado apos ");
            Serial.print(i);
            Serial.println(" passos.");
            break;
        }

        // Pulso STEP (ativo alto)
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(usDelay);

        // Atualiza posição
        _positionSteps += (dir == STEPPER_DIR_FORWARD) ? 1 : -1;

        // Proteção de curso máximo por passo
        if (labs(_positionSteps) > maxStepsAbs) {
            Serial.println("[STEPPER] Max travel limit reached, stopping.");
            break;
        }
    }

    Serial.print("[STEPPER] Concluido. Posicao: ");
    Serial.print(getPositionMm(), 2);
    Serial.println(" mm");
}

void StepperManager::homeToEndstop(long maxSteps, uint16_t usDelay) {
    StepperDirection dirHome = STEPPER_DIR_BACKWARD;  // Volta para home (para baixo)
    digitalWrite(DIR_PIN, LOW);  // LOW = backward (ajustado ao invertido acima)

    Serial.println("[STEPPER] Homing iniciado...");

    for (long i = 0; i < maxSteps; ++i) {
        if (isEndstopPressed()) {
            resetPosition();
            _lastHomingSuccess = true;
            Serial.println("[STEPPER] Home encontrado!");
            // Recuar 30mm para posição inicial segura
            long backoffSteps = (long)(STEPPER_HOME_BACKOFF_MM * _stepsPerMm);
            Serial.print("[STEPPER] Recuando ");
            Serial.print(STEPPER_HOME_BACKOFF_MM);
            Serial.println(" mm a partir do home...");
            moveSteps(backoffSteps, STEPPER_DIR_FORWARD, usDelay);
            return;
        }

        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(usDelay);
    }

    Serial.println("[STEPPER] AVISO: Homing falhou - micro switch nao encontrado!");
    _lastHomingSuccess = false;
}

void StepperManager::homeToEndstopWithMonitor(long maxSteps, uint16_t usDelay,
                                             bool (*monitorFunc)(void*), void* ctx) {
    StepperDirection dirHome = STEPPER_DIR_BACKWARD;  // Volta para home (para baixo)
    digitalWrite(DIR_PIN, LOW);  // LOW = backward (ajustado ao invertido acima)

    Serial.println("[STEPPER] Homing (monitorado) iniciado...");

    for (long i = 0; i < maxSteps; ++i) {
        // Verifica endstop antes do passo
        if (isEndstopPressed()) {
            resetPosition();
            _lastHomingSuccess = true;
            Serial.println("[STEPPER] Home encontrado!");
            // Recuar para posição inicial segura
            long backoffSteps = (long)(STEPPER_HOME_BACKOFF_MM * _stepsPerMm);
            Serial.print("[STEPPER] Recuando ");
            Serial.print(STEPPER_HOME_BACKOFF_MM);
            Serial.println(" mm a partir do home...");
            moveSteps(backoffSteps, STEPPER_DIR_FORWARD, usDelay);
            return;
        }

        // Avalia callback de monitoramento (ex.: balança)
        if (monitorFunc && monitorFunc(ctx)) {
            Serial.print("[STEPPER] Homing abortado pelo monitor externo apos ");
            Serial.print(i);
            Serial.println(" passos.");
            _lastHomingSuccess = false;
            return;
        }

        // Executa um passo
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(usDelay);
    }

    Serial.println("[STEPPER] AVISO: Homing falhou - micro switch nao encontrado!");
    _lastHomingSuccess = false;
}

void StepperManager::moveToPositionMm(float targetMm, uint16_t usDelay) {
    // Proteção de limite máximo de curso
    if (targetMm < 0.0f) {
        targetMm = 0.0f;
    }
    
    if (targetMm > STEPPER_MAX_TRAVEL_MM) {
        targetMm = STEPPER_MAX_TRAVEL_MM;
    }
    
    long targetSteps = (long)round(targetMm * _stepsPerMm);
    long deltaSteps  = targetSteps - _positionSteps;

    if (deltaSteps == 0) return;

    StepperDirection dir = (deltaSteps > 0) ? STEPPER_DIR_FORWARD : STEPPER_DIR_BACKWARD;
    long steps = labs(deltaSteps);

    moveSteps(steps, dir, usDelay);
}

bool StepperManager::checkAndHandleStall() {
    return false;  // A4988 não tem StallGuard
}
