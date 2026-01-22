#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include "config.h"

class ScaleManager {
public:
    void begin();
    void tare(uint8_t times = 10);
    void update();

    float getWeightKg() const;

    void setCalibFactor(float factor);
    float getCalibFactor() const;

    // Calibração automática com peso conhecido (ex.: 5 kg)
    void calibrateWithKnownWeight(float knownKg);

    // EEPROM
    void loadCalibrationFromEEPROM();
    void saveCalibrationToEEPROM();

    // Leituras rápidas e prontidão
    bool isReady() const;
    float peekWeightKgFast();

    // Leitura bruta do HX711 (contagens) para depuração
    long getRawReading();
    long getRawReadingAbsolute();

private:
    float _calibFactor = SCALE_CALIB_DEFAULT;
    float _currentKg   = 0.0f;
    long  _lastRaw     = 0;
};

extern ScaleManager scaleManager;

#endif
