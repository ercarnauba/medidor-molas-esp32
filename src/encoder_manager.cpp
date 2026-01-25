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
    _lastEncoderTime = 0;
    _lastButtonISRTime = 0;
    _lastButtonReading = digitalRead(ENC_SW_PIN);
    _buttonStableState = _lastButtonReading;
    _lastButtonChange = millis();

    // Contar todas as bordas e acumular para 1 passo por detente
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
                // Botão foi liberado - limpar flags órfãs se não foram processadas
                noInterrupts();
                if (!_buttonLongPressed && _buttonClicked) {
                    // Clique curto pendente - manter (será processado por wasButtonClicked)
                } else if (_buttonLongPressed) {
                    // Long press detectado - manter (será processado por wasButtonLongPressed)
                } else {
                    // Nenhum evento válido - limpar tudo (bounce ou evento perdido)
                    _buttonPending = false;
                    _buttonClicked = false;
                }
                interrupts();
                _pressStartMillis = 0;
                _longPressReported = false;
            }
        }

        // Detecta long press quando botão permanece pressionado por LONG_PRESS_MS
        if (_buttonStableState == LOW && !_longPressReported && _pressStartMillis > 0 &&
            (now - _pressStartMillis) >= LONG_PRESS_MS) {
            noInterrupts();
            _buttonLongPressed = true;
            _buttonClicked = false;  // Cancela clique curto se existir
            interrupts();
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
    bool longPressed = _buttonLongPressed;
    
    // Se long press está ativo, ignorar clique curto
    if (longPressed) {
        interrupts();
        return false;
    }
    
    if (clicked) {
        _buttonClicked = false;
        _buttonPending = false;  // Libera para próximo clique após processar
        _lastButtonISRTime = 0;  // Reseta timer de debounce na ISR
    }
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
        _buttonClicked = false;      // Limpa clique pendente se houver
        _buttonPending = false;      // Libera para próximo clique
        _lastButtonISRTime = 0;      // Reseta timer de debounce na ISR
        pressed = true;
    }
    interrupts();
    return pressed;
}

// ==== ISRs ====

void IRAM_ATTR encoderISR() {
    // Edge detection com acumulação: converte múltiplas bordas em 1 passo lógico
    int stateCLK = digitalRead(ENC_CLK_PIN);
    int stateDT  = digitalRead(ENC_DT_PIN);

    if (stateCLK != _encPtr->_lastStateCLK) {
        _encPtr->_lastStateCLK = stateCLK;
        int dir = (stateDT != stateCLK) ? +1 : -1;

        int accum = _encPtr->_edgeAccum + dir;
        if (accum >= ENCODER_EDGES_PER_STEP) {
            _encPtr->_position++;
            accum -= ENCODER_EDGES_PER_STEP;
        } else if (accum <= -ENCODER_EDGES_PER_STEP) {
            _encPtr->_position--;
            accum += ENCODER_EDGES_PER_STEP;
        }
        _encPtr->_edgeAccum = accum;
    }
}

void IRAM_ATTR encoderButtonISR() {
    // Se ja ha evento pendente, ignora completamente (ainda processando)
    if (_encPtr->_buttonPending) {
        return;
    }
    
    // Debounce de tempo: 150ms para cobrir bounce mecanico severo
    unsigned long now = micros();
    if ((now - _encPtr->_lastButtonISRTime) < 150000) {
        return;  // Bounce - ignorar
    }
    _encPtr->_lastButtonISRTime = now;
    
    // Aceita evento
    _encPtr->_buttonClicked = true;
    _encPtr->_buttonPending = true;  // Bloqueia proximas interrupcoes
}
