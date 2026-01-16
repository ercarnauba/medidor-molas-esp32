#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>

class EncoderManager {
    // Permite que as ISRs acessem os membros privados
    friend void IRAM_ATTR encoderISR();
    friend void IRAM_ATTR encoderButtonISR();

public:
    void begin();

    // Atualiza estado do botão (inclui detecção de long press)
    void update();

    long getPosition();
    void setPosition(long pos);

    bool wasButtonClicked();     // retorna true apenas uma vez por clique
    bool wasButtonLongPressed(); // retorna true apenas uma vez por long press

private:
    volatile long _position      = 0;
    volatile int  _lastStateCLK  = HIGH;
    volatile bool _buttonClicked = false;
    volatile bool _buttonLongPressed = false;
    unsigned long _lastClickMillis = 0;
    unsigned long _pressStartMillis = 0;
    unsigned long _lastButtonChange = 0;
    int _lastButtonReading = HIGH;
    int _buttonStableState = HIGH;
    bool _longPressReported = false;
    static constexpr unsigned long DEBOUNCE_MS = 50;
    static constexpr unsigned long LONG_PRESS_MS = 800;
};

extern EncoderManager encoderManager;

// Protótipos das ISRs
void IRAM_ATTR encoderISR();
void IRAM_ATTR encoderButtonISR();

#endif
