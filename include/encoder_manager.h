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
    volatile unsigned long _lastEncoderTime = 0;  // Debounce encoder
    volatile unsigned long _lastButtonISRTime = 0;  // Debounce botão na ISR
    volatile bool _buttonClicked = false;
    volatile bool _buttonLongPressed = false;
    volatile bool _buttonPending = false;  // Flag para prevenir acionamentos múltiplos
    unsigned long _lastClickMillis = 0;
    unsigned long _pressStartMillis = 0;
    unsigned long _lastButtonChange = 0;
    int _lastButtonReading = HIGH;
    int _buttonStableState = HIGH;
    bool _longPressReported = false;
    static constexpr unsigned long DEBOUNCE_MS = 350;      // debounce aumentado para evitar falso clique
    static constexpr unsigned long LONG_PRESS_MS = 1000;   // long press timeout
};

extern EncoderManager encoderManager;

// Protótipos das ISRs
void IRAM_ATTR encoderISR();
void IRAM_ATTR encoderButtonISR();

#endif
