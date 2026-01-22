#include "ui_manager.h"
#include "config.h"

#include <TFT_eSPI.h>
#include <SPI.h>

UiManager uiManager;
static TFT_eSPI tft = TFT_eSPI();

// Área do gráfico maximizada ao limite da tela
static const int GRAPH_X0 = 5;
static const int GRAPH_Y0 = 35;
static const int GRAPH_W  = 310;
static const int GRAPH_H  = 220;

static bool lastPointValid = false;
static int  lastPx = 0;
static int  lastPy = 0;

void UiManager::begin() {
    tft.init();
    tft.setRotation(1);

    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, HIGH);   // backlight ON

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("Medidor de mola");

    _mode = UI_MODE_MENU;
}

void UiManager::setMode(UiMode mode) {
    _mode = mode;
}

UiMode UiManager::getMode() const {
    return _mode;
}

// ---- MENU ----

void UiManager::drawMenu(const char* const* items, int itemCount, int selectedIndex) {
    _mode = UI_MODE_MENU;

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("Menu principal");

    tft.setTextSize(2);
    int y = 30;
    for (int i = 0; i < itemCount; ++i) {
        y += 25;
        if (i == selectedIndex) {
            // destaque simples: invertido
            tft.fillRect(0, y - 2, 240, 22, TFT_WHITE);
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.setCursor(5, y);
        tft.print(i);
        tft.print(") ");
        tft.print(items[i]);
    }

    // Rodapé
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.fillRect(0, 200, 240, 20, TFT_BLACK);
    tft.setCursor(0, 200);
    tft.print("Encoder: navegar | Click: selecionar");
}

// ---- TESTE DA MOLA ----

void UiManager::drawTestStatus(float forceKg,
                               float compressionMm,
                               float k_kgf_mm,
                               float k_N_mm,
                               bool running,
                               bool done)
{
    _mode = UI_MODE_TEST;

    // Cabeçalho com texto aumentado
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.fillRect(0, 0, 320, 32, TFT_BLACK);
    tft.setCursor(3, 2);
    tft.print("TESTE MOLA");
    tft.setCursor(150, 2);
    tft.printf("F:%.1f", forceKg);
    tft.setCursor(3, 16);
    tft.printf("C:%.1fmm", compressionMm);
    tft.setCursor(150, 16);
    tft.print("kg");

    // Linha divisória
    tft.drawFastHLine(0, 33, 320, TFT_DARKGREY);

    // Área do gráfico: y=35 até y=255 (altura 220px)
    // O gráfico será desenhado entre essas coordenadas

    // Rodapé compacto com resultados
    tft.fillRect(0, 258, 320, 22, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(3, 260);
    if (done) {
        tft.printf("K:%.3f kgf/mm", k_kgf_mm);
        tft.setCursor(3, 270);
        tft.printf("K:%.3f N/mm", k_N_mm);
    } else {
        tft.print("K:calc...");
    }

    // Status compacto
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(200, 265);
    if (running) {
        tft.print("RUN");
    } else if (done) {
        tft.print("OK");
    } else {
        tft.print("RDY");
    }

    // Instrução
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(130, 260);
    tft.print("Click:menu");
}

// Limpa área de gráfico maximizada ao limite
void UiManager::clearGraphArea() {
    // Área do gráfico: x=5, y=35, largura=310, altura=220
    const int gx = 5;
    const int gy = 35;
    const int gw = 310;
    const int gh = 220;
    
    tft.fillRect(gx - 1, gy - 1, gw + 2, gh + 2, TFT_BLACK);

    // Moldura do gráfico
    tft.drawRect(gx, gy, gw, gh, TFT_DARKGREY);

    // Labels dos eixos compactos
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(gx + 3, gy + 3);
    tft.print("F(kg)");
    tft.setCursor(gx + gw - 48, gy + gh - 11);
    tft.print("x(mm)");

    lastPointValid = false;
}

// Adiciona ponto no gráfico (desloc x força, normalizado)
void UiManager::plotGraphPoint(float xNorm, float yNorm, bool firstPoint) {
    if (xNorm < 0.0f) xNorm = 0.0f;
    if (xNorm > 1.0f) xNorm = 1.0f;
    if (yNorm < 0.0f) yNorm = 0.0f;
    if (yNorm > 1.0f) yNorm = 1.0f;

    int px = GRAPH_X0 + (int)(xNorm * (GRAPH_W - 1));
    int py = GRAPH_Y0 + GRAPH_H - 1 - (int)(yNorm * (GRAPH_H - 1));

    if (firstPoint || !lastPointValid) {
        tft.drawPixel(px, py, TFT_GREEN);
        lastPointValid = true;
        lastPx = px;
        lastPy = py;
    } else {
        tft.drawLine(lastPx, lastPy, px, py, TFT_GREEN);
        lastPx = px;
        lastPy = py;
    }
}

// ---- TELA DE CALIBRAÇÃO ----

void UiManager::drawCalibScreen(uint8_t stage,
                                float currentKg,
                                float calibFactor,
                                float referenceWeightKg)
{
    _mode = UI_MODE_CALIB;

    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Calibrar balanca");

    tft.setTextSize(2);
    tft.setCursor(0, 40);
    tft.printf("Leitura: %.2f kg", currentKg);

    tft.setTextSize(1);
    tft.setCursor(0, 80);
    tft.printf("Fator: %.1f", calibFactor);

    tft.setCursor(0, 100);
    if (stage == 0) {
        tft.println("1) Retire todos os pesos.");
        tft.println("2) Click encoder = TARA.");
        tft.println("Longo = sair.");
    } else if (stage == 1) {
        tft.println("Coloque peso de ref.");
        tft.printf("(%.1f kg)\n", referenceWeightKg);
        tft.println("Click encoder = CALIB.");
        tft.println("Longo = sair.");
    } else {
        tft.println("Calibracao concluida.");
        tft.println("Click encoder = voltar ao menu.");
        tft.println("Longo = voltar ao menu.");
    }
}

// ---- TELA DE SELEÇÃO DE PESO ----

void UiManager::drawWeightSelectionScreen(const float* weights, int count, int selectedIndex) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Escolher peso ref.");

    tft.setTextSize(2);
    int y = 30;
    for (int i = 0; i < count; ++i) {
        y += 25;
        if (i == selectedIndex) {
            // destaque: invertido
            tft.fillRect(0, y - 2, 240, 22, TFT_WHITE);
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.setCursor(5, y);
        tft.printf("%.1f kg", weights[i]);
    }

    // Rodapé
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.fillRect(0, 200, 240, 40, TFT_BLACK);
    tft.setCursor(0, 200);
    tft.print("Encoder: navegar");
    tft.setCursor(0, 212);
    tft.print("Click: selecionar");
    tft.setCursor(0, 224);
    tft.print("Longo: cancelar");
}

// ---- FUNÇÕES AUXILIARES ----

void UiManager::clearScreen() {
    tft.fillScreen(TFT_BLACK);
}

void UiManager::drawText(const char* text, int x, int y, uint16_t color, uint8_t size) {
    tft.setTextSize(size);
    tft.setTextColor(color, TFT_BLACK);
    tft.setCursor(x, y);
    tft.print(text);
}

void UiManager::showStallAlert() {
    // Desenha um alerta vermelho centralizado na tela
    int alertX = 10;
    int alertY = 100;
    int alertW = 300;
    int alertH = 80;
    
    // Fundo vermelho do alerta
    tft.fillRect(alertX, alertY, alertW, alertH, TFT_RED);
    tft.drawRect(alertX, alertY, alertW, alertH, TFT_WHITE);
    tft.drawRect(alertX+1, alertY+1, alertW-2, alertH-2, TFT_WHITE);
    
    // Texto do alerta
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    
    tft.setCursor(alertX + 15, alertY + 10);
    tft.println("ALERTA STALLGUARD!");
    
    tft.setTextSize(1);
    tft.setCursor(alertX + 10, alertY + 35);
    tft.println("Travamento mecanico detectado");
    
    tft.setCursor(alertX + 10, alertY + 50);
    tft.println("Motor recuado 10mm");
    
    tft.setCursor(alertX + 10, alertY + 65);
    tft.println("Verifique obstrucao no trilho");
    
    _stallAlertVisible = true;
    _stallAlertTime = millis();
    
    Serial.println("[UI] Stall alert displayed on LCD");
}

void UiManager::clearStallAlert() {
    if (_stallAlertVisible) {
        // Limpa área do alerta (redesenha tela dependendo do modo)
        // Por simplicidade, redesenha toda a tela
        clearScreen();
        _stallAlertVisible = false;
        
        Serial.println("[UI] Stall alert cleared");
    }
}
