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
    
    // Desenha um ponto da curva amarela (K) no gráfico
    // xNorm: 0..1 (deslocamento)
    // yNorm: 0..1 (K normalizado)
    // firstPoint: true se for o primeiro ponto da curva
    void plotGraphPointYellow(float xNorm, float yNorm, bool firstPoint);

    // Desenha valor de K na lista lateral (incrementalmente)
    // step: número do passo (0, 1, 2...)
    // k_kgf_mm: valor da constante elástica em kgf/mm
    // k_N_mm: valor da constante elástica em N/mm
    void drawKValueAtStep(int step, float k_kgf_mm, float k_N_mm);

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

    // ---- FUNÇÕES AUXILIARES PARA TEXTO CUSTOMIZADO ----
    // Limpa toda a tela
    void clearScreen();
    
    // Desenha texto em posição específica
    // size: 1 (pequeno), 2 (médio), 3 (grande)
    void drawText(const char* text, int x, int y, uint16_t color, uint8_t size = 1);

    // Preenche retângulo com cor sólida
    void fillRect(int x, int y, int w, int h, uint16_t color);

    // ---- ALERTAS DE STALLGUARD ----
    // Exibe alerta de travamento mecânico detectado pelo StallGuard
    void showStallAlert();
    
    // Limpa alerta de stall após timeout
    void clearStallAlert();

private:
    UiMode _mode = UI_MODE_MENU;
    bool _stallAlertVisible = false;
    unsigned long _stallAlertTime = 0;
};

extern UiManager uiManager;

#endif
