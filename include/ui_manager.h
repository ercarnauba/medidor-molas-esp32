#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>

// Modo geral da tela
enum UiMode {
    UI_MODE_MENU = 0,
    UI_MODE_TEST,
    UI_MODE_CALIB
};

class UiManager {
public:
    void begin();

    void setMode(UiMode mode);
    UiMode getMode() const;

    // ---- TELA DE MENU ----
    // items: array de strings (const char*)
    void drawMenu(const char* const* items, int itemCount, int selectedIndex);

    // ---- TELA DE TESTE DA MOLA ----
    // Mostra cabeçalho (força, compressão, K, status)
    void drawTestStatus(float forceKg,
                        float compressionMm,
                        float k_kgf_mm,
                        float k_N_mm,
                        bool running,
                        bool done);

    // Desenha um ponto no gráfico (normalizado 0..1)
    // xNorm: 0..1 (deslocamento)
    // yNorm: 0..1 (força)
    // firstPoint: true se for o primeiro ponto da curva
    void plotGraphPoint(float xNorm, float yNorm, bool firstPoint);

    // Limpa a área do gráfico
    void clearGraphArea();

    // ---- TELA DE CALIBRAÇÃO DA BALANÇA ----
    // stage: 0 = aguardando início, 1 = após tara, aguardando peso, 2 = concluído
    void drawCalibScreen(uint8_t stage,
                         float currentKg,
                         float calibFactor,
                         float referenceWeightKg = 5.0f);

    // ---- TELA DE SELEÇÃO DE PESO PARA CALIBRAÇÃO ----
    // Mostra lista de pesos disponíveis para o usuário escolher
    void drawWeightSelectionScreen(const float* weights, int count, int selectedIndex);

private:
    UiMode _mode = UI_MODE_MENU;
};

extern UiManager uiManager;

#endif
