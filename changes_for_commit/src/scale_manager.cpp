#include "scale_manager.h"
#include <HX711.h>
#include <EEPROM.h>
#include "config.h"

static HX711 scale;
ScaleManager scaleManager;

// Layout na EEPROM (ESP32)
static const int EEPROM_ADDR_MAGIC = 0;
static const int EEPROM_ADDR_CALIB = 4;
static const uint32_t EEPROM_MAGIC = 0xA5A5DEAD;

void ScaleManager::begin() {
#ifdef ESP32
    EEPROM.begin(64);   // tamanho suficiente
#endif

    // Carrega fator salvo (se existir)
    loadCalibrationFromEEPROM();

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(_calibFactor);
    tare();
}

void ScaleManager::tare(uint8_t times) {
    scale.tare(times);
}

void ScaleManager::update() {
    if (scale.is_ready()) {
        _currentKg = scale.get_units(5);  // média de 5 leituras em kg
    }
}

float ScaleManager::getWeightKg() const {
    return _currentKg;
}

void ScaleManager::setCalibFactor(float factor) {
    if (factor <= 0.0f) return;
    _calibFactor = factor;
    scale.set_scale(_calibFactor);
}

float ScaleManager::getCalibFactor() const {
    return _calibFactor;
}

void ScaleManager::calibrateWithKnownWeight(float knownKg) {
    if (knownKg <= 0.0f) return;

    // Pressupõe que já foi feito tare() sem peso
    // Fórmula típica: SCALE = (raw - offset) / peso
    long raw    = scale.read_average(10);
    long offset = scale.get_offset();
    long diff   = raw - offset;

    if (diff == 0) return;

    float newCalib = (float)diff / knownKg;
    if (newCalib > 0.0f && newCalib < 1000000.0f) {
        _calibFactor = newCalib;
        scale.set_scale(_calibFactor);
    }
}

void ScaleManager::loadCalibrationFromEEPROM() {
#ifdef ESP32
    uint32_t magic = 0;
    EEPROM.get(EEPROM_ADDR_MAGIC, magic);
    if (magic == EEPROM_MAGIC) {
        float f;
        EEPROM.get(EEPROM_ADDR_CALIB, f);
        if (f > 0.0f && f < 1000000.0f) {
            _calibFactor = f;
        }
    }
#endif
}

void ScaleManager::saveCalibrationToEEPROM() {
#ifdef ESP32
    EEPROM.put(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
    EEPROM.put(EEPROM_ADDR_CALIB, _calibFactor);
    EEPROM.commit();
#endif
}
