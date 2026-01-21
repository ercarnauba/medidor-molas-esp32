#ifndef STEPPER_MANAGER_H
#define STEPPER_MANAGER_H

#include <Arduino.h>

enum StepperDirection {
    STEPPER_DIR_FORWARD  = 0,
    STEPPER_DIR_BACKWARD = 1
};

class StepperManager {
public:
    void begin();
    void enable(bool on);

    // Movimento em passos "bruto"
    void moveSteps(long steps, StepperDirection dir, uint16_t usDelay = 800);

    // Homing até o fim de curso
    void homeToEndstop(long maxSteps, uint16_t usDelay = 800);

    // Verifica se o último homing foi bem-sucedido
    bool wasLastHomingSuccessful() const;

    // Configuração / posição
    void setStepsPerMm(float s);
    float getStepsPerMm() const;

    void resetPosition(); // zera posição em passos
    float getPositionMm() const;

    // Movimento absoluto em mm (a partir de zero definido na home)
    void moveToPositionMm(float targetMm, uint16_t usDelay = 800);

    // Leitura do endstop
    bool isEndstopPressed() const;

    // StallGuard - verifica se houve detecção de stall e trata
    bool checkAndHandleStall();

private:
    float _stepsPerMm     = 1600.0f;
    long  _positionSteps  = 0;
    bool  _lastHomingSuccess = false;
};

extern StepperManager stepperManager;

#endif
