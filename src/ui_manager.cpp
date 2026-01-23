#include "ui_manager.h"
#include "config.h"

#include <TFT_eSPI.h>
#include <SPI.h>

UiManager uiManager;
static TFT_eSPI tft = TFT_eSPI();

// Área do gráfico maximizada ao limite da tela (480px horizontal)
static const int GRAPH_X0 = 5;
static const int GRAPH_Y0 = 35;
static const int GRAPH_W  = 310;
static const int GRAPH_H  = 220;
static const int K_VALUES_X = 320;  // Posição X para lista de K (fora do gráfico, à direita)
static const int K_VALUES_Y0 = 40;  // Posição Y inicial para lista de K

// Variáveis para rastrear pontos das duas curvas
static bool lastPointValid = false;  // Curva verde (força)
static int  lastPx = 0;
static int  lastPy = 0;
static bool lastPointValidYellow = false;  // Curva amarela (K)
static int  lastPxYellow = 0;
static int  lastPyYellow = 0;

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
    tft.setTextSize(3);
    tft.setCursor(10, 10);
    tft.println("Menu Principal");

    tft.setTextSize(3);
    int y = 70;
    for (int i = 0; i < itemCount; ++i) {
        y += 35;
        if (i == selectedIndex) {
            // destaque: invertido
            tft.fillRect(5, y - 3, 470, 26, TFT_WHITE);
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.setCursor(10, y);
        tft.print(i);
        tft.print(") ");
        tft.print(items[i]);
    }

    // Rodapé
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.fillRect(0, 290, 480, 30, TFT_BLACK);
    tft.setCursor(10, 295);
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

    // Cabeçalho em linha única
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.fillRect(0, 0, 320, 32, TFT_BLACK);
    tft.setCursor(3, 6);
    char headerBuf[64];
    snprintf(headerBuf, sizeof(headerBuf), "TESTE MOLA  C:%.1fmm  F:%.1fkg", compressionMm, forceKg);
    tft.print(headerBuf);

    // Linha divisória
    tft.drawFastHLine(0, 33, 320, TFT_DARKGREY);

    // Área do gráfico: y=35 até y=255 (altura 220px)
    // O gráfico será desenhado entre essas coordenadas

    // Rodapé com K em fonte maior
    tft.fillRect(0, 255, 320, 40, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);  // K em N/mm em amarelo
    tft.setCursor(3, 255);
    if (done) {
        tft.printf("K:%.3f N/mm", k_N_mm);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);  // K em kgf/mm em verde
        tft.setCursor(3, 273);
        tft.printf("K:%.3f kgf/mm", k_kgf_mm);
    } else {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.print("K:calc...");
    }

    // Status compacto
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(230, 260);
    if (running) {
        tft.print("RUN");
    } else if (done) {
        tft.print("OK");
    } else {
        tft.print("RDY");
    }

    // Instrução
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(200, 295);
    tft.print("Click:menu");
}

// Limpa área de gráfico maximizada ao limite
void UiManager::clearGraphArea() {
    // Área do gráfico usando toda largura disponível (480px)
    const int gx = 5;
    const int gy = 35;
    const int gw = 310;
    const int gh = 220;
    
    tft.fillRect(gx - 1, gy - 1, gw + 2, gh + 2, TFT_BLACK);
    
    // Limpa também a área de valores de K à direita
    tft.fillRect(K_VALUES_X, K_VALUES_Y0 - 5, 110, 215, TFT_BLACK);

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
    lastPointValidYellow = false;
}

void UiManager::drawKValueAtStep(int step, float k_kgf_mm, float k_N_mm) {
    // Desenha o valor de K para o passo atual na lista lateral
    int yPos = K_VALUES_Y0 + (step * 18);  // 18 pixels de espaçamento vertical
    
    // Limita para não sair da tela (máximo ~12 linhas)
    if (yPos > (K_VALUES_Y0 + 200)) return;
    
    tft.setTextSize(1);
    int x = K_VALUES_X;

    char bufN[26];
    snprintf(bufN, sizeof(bufN), "%dmm: %.2fN/mm", step, k_N_mm);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);  // N/mm em amarelo
    tft.setCursor(x, yPos);
    tft.print(bufN);

    int advance = (int)strlen(bufN) * 6;  // fonte size=1 => 6px por char
    tft.setTextColor(TFT_GREEN, TFT_BLACK);  // kgf/mm em verde
    tft.setCursor(x + advance + 6, yPos);
    char bufKg[20];
    snprintf(bufKg, sizeof(bufKg), "%.2fKgf/mm", k_kgf_mm);
    tft.print(bufKg);
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
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Calibrar Balanca");

    tft.setTextSize(3);
    tft.setCursor(10, 70);
    tft.printf("Leitura: %.2f kg", currentKg);

    tft.setTextSize(2);
    tft.setCursor(10, 120);
    tft.printf("Fator: %.1f", calibFactor);

    tft.setTextSize(2);
    tft.setCursor(10, 160);
    if (stage == 0) {
        tft.println("1) Retire todos os pesos.");
        tft.setCursor(10, 190);
        tft.println("2) Click encoder = TARA.");
        tft.setCursor(10, 220);
        tft.println("Longo = sair.");
    } else if (stage == 1) {
        tft.println("Coloque peso de ref.");
        tft.setCursor(10, 190);
        tft.printf("(%.1f kg)", referenceWeightKg);
        tft.setCursor(10, 220);
        tft.println("Click encoder = CALIB.");
        tft.setCursor(10, 250);
        tft.println("Longo = sair.");
    } else {
        tft.println("Calibracao concluida.");
        tft.setCursor(10, 190);
        tft.println("Click = voltar ao menu.");
        tft.setCursor(10, 220);
        tft.println("Longo = voltar ao menu.");
    }
}

// ---- TELA DE SELEÇÃO DE PESO ----

void UiManager::drawWeightSelectionScreen(const float* weights, int count, int selectedIndex) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Escolher Peso Ref.");

    tft.setTextSize(2);
    int y = 50;
    for (int i = 0; i < count; ++i) {
        y += 35;
        if (i == selectedIndex) {
            // destaque: invertido
            tft.fillRect(5, y - 3, 470, 26, TFT_WHITE);
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.setCursor(10, y);
        tft.printf("%.1f kg", weights[i]);
    }

    // Rodapé
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.fillRect(0, 270, 480, 50, TFT_BLACK);
    
    // Centralizar "Encoder: navegar" (16 chars * 12px = 192px)
    int xEncoder = (480 - 192) / 2;
    tft.setCursor(xEncoder, 275);
    tft.print("Encoder: navegar");
    
    tft.setCursor(10, 295);
    tft.print("Click: selecionar | Longo: cancelar");
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

void UiManager::drawCenteredText(const char* text, uint16_t color, uint8_t size) {
    if (size < 1) size = 1;
    // Largura/altura por caractere para fonte padrão do TFT_eSPI
    int charW = 6 * (int)size;
    int charH = 8 * (int)size;
    int len = (int)strlen(text);
    int textW = charW * len;
    int textH = charH;
    // Dimensões da tela com rotation=1 (480x320)
    const int screenW = 480;
    const int screenH = 320;
    int x = (screenW - textW) / 2;
    int y = (screenH - textH) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    tft.setTextSize(size);
    tft.setTextColor(color, TFT_BLACK);
    tft.setCursor(x, y);
    tft.print(text);
}

// Adiciona ponto amarelo no gráfico (K em N/mm)
void UiManager::plotGraphPointYellow(float xNorm, float yNorm, bool firstPoint) {
    if (xNorm < 0.0f) xNorm = 0.0f;
    if (xNorm > 1.0f) xNorm = 1.0f;
    if (yNorm < 0.0f) yNorm = 0.0f;
    if (yNorm > 1.0f) yNorm = 1.0f;

    int px = GRAPH_X0 + (int)(xNorm * (GRAPH_W - 1));
    int py = GRAPH_Y0 + GRAPH_H - 1 - (int)(yNorm * (GRAPH_H - 1));

    if (firstPoint || !lastPointValidYellow) {
        tft.drawPixel(px, py, TFT_YELLOW);
        lastPointValidYellow = true;
        lastPxYellow = px;
        lastPyYellow = py;
    } else {
        tft.drawLine(lastPxYellow, lastPyYellow, px, py, TFT_YELLOW);
        lastPxYellow = px;
        lastPyYellow = py;
    }
}

void UiManager::fillRect(int x, int y, int w, int h, uint16_t color) {
    tft.fillRect(x, y, w, h, color);
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
}

void UiManager::clearStallAlert() {
    if (_stallAlertVisible) {
        // Limpa área do alerta (redesenha tela dependendo do modo)
        // Por simplicidade, redesenha toda a tela
        clearScreen();
        _stallAlertVisible = false;
    }
}



