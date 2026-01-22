#include "encoder_manager.h"
#include "config.h"

EncoderManager encoderManager;

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// Ponteiro para a instância global, usado na ISR
static EncoderManager* _encPtr = &encoderManager;

void EncoderManager::begin() {
    pinMode(ENC_CLK_PIN, INPUT_PULLUP);
    pinMode(ENC_DT_PIN,  INPUT_PULLUP);
    pinMode(ENC_SW_PIN,  INPUT_PULLUP);

    _lastStateCLK = digitalRead(ENC_CLK_PIN);
    _lastButtonReading = digitalRead(ENC_SW_PIN);
    _buttonStableState = _lastButtonReading;
    _lastButtonChange = millis();

    attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_SW_PIN),  encoderButtonISR, FALLING);
}

void EncoderManager::update() {
    unsigned long now = millis();
    int reading = digitalRead(ENC_SW_PIN);

    if (reading != _lastButtonReading) {
        _lastButtonChange = now;
        _lastButtonReading = reading;
    }

    if ((now - _lastButtonChange) > DEBOUNCE_MS) {
        if (reading != _buttonStableState) {
            _buttonStableState = reading;
            if (_buttonStableState == LOW) {
                _pressStartMillis = now;
                _longPressReported = false;
            } else {
                _pressStartMillis = 0;
                _longPressReported = false;
            }
        }

        if (_buttonStableState == LOW && !_longPressReported && _pressStartMillis > 0 &&
            (now - _pressStartMillis) >= LONG_PRESS_MS) {
            _buttonLongPressed = true;
            _longPressReported = true;
        }
    }
}

long EncoderManager::getPosition() {
    noInterrupts();
    long pos = _position;
    interrupts();
    return pos;
}

void EncoderManager::setPosition(long pos) {
    noInterrupts();
    _position = pos;
    interrupts();
}

bool EncoderManager::wasButtonClicked() {
    unsigned long now = millis();
    
    // Primeiro verifica debounce
    if (now - _lastClickMillis <= DEBOUNCE_MS) {
        return false;  // Ainda em debounce, ignorar
    }
    
    // Depois lê flag de clique
    noInterrupts();
    bool clicked = _buttonClicked;
    if (clicked) _buttonClicked = false;
    interrupts();

    if (clicked) {
        _lastClickMillis = now;
        return true;
    }
    
    return false;
}

bool EncoderManager::wasButtonLongPressed() {
    bool pressed = false;
    noInterrupts();
    if (_buttonLongPressed) {
        _buttonLongPressed = false;
        pressed = true;
    }
    interrupts();
    return pressed;
}

// ==== ISRs ====

void IRAM_ATTR encoderISR() {
    int stateCLK = digitalRead(ENC_CLK_PIN);
    int stateDT  = digitalRead(ENC_DT_PIN);

    if (stateCLK != _encPtr->_lastStateCLK) {
        // Se DT != CLK, gira num sentido; senão, no outro
        if (stateDT != stateCLK) {
            _encPtr->_position++;
        } else {
            _encPtr->_position--;
        }
        _encPtr->_lastStateCLK = stateCLK;
    }
}

void IRAM_ATTR encoderButtonISR() {
    // Marca apenas que o botão foi pressionado; debounce é tratado no contexto principal
    // NÃO chamar millis() aqui - não é seguro em ISRs
    // NÃO setar novamente se já foi setado para evitar phantom clicks
    if (!_encPtr->_buttonClicked) {
        _encPtr->_buttonClicked = true;
    }
}
