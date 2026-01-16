#include "ui_manager.h"
#include "config.h"

#include <TFT_eSPI.h>
#include <SPI.h>

UiManager uiManager;
static TFT_eSPI tft = TFT_eSPI();

// Área do gráfico (ajuste se quiser)
static const int GRAPH_X0 = 20;
static const int GRAPH_Y0 = 70;
static const int GRAPH_W  = 200;
static const int GRAPH_H  = 120;

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

    // Cabeçalho
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillRect(0, 0, 240, 30, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.print("Teste mola (k)");

    // Forca
    tft.fillRect(0, 30, 240, 20, TFT_BLACK);
    tft.setCursor(0, 30);
    tft.printf("F: %.2f kg", forceKg);

    // Compressao alvo
    tft.fillRect(0, 50, 240, 20, TFT_BLACK);
    tft.setCursor(0, 50);
    tft.printf("Comp: %.1f mm", compressionMm);

    // K (parte inferior direita)
    tft.setTextSize(1);
    tft.fillRect(0, 200, 240, 40, TFT_BLACK);
    tft.setCursor(0, 200);
    if (done) {
        tft.printf("K: %.3f kgf/mm", k_kgf_mm);
        tft.setCursor(0, 212);
        tft.printf("K: %.3f N/mm",   k_N_mm);
    } else {
        tft.print("K: ---");
    }

    // Status
    tft.setCursor(140, 200);
    if (running) {
        tft.print("Rodando...");
    } else if (done) {
        tft.print("Concluido");
    } else {
        tft.print("Pronto");
    }

    // Mensagem
    tft.setCursor(0, 228);
    tft.print("Click encoder: voltar ao menu");
}

// Limpa área de gráfico
void UiManager::clearGraphArea() {
    tft.fillRect(GRAPH_X0 - 2, GRAPH_Y0 - 2,
                 GRAPH_W + 4, GRAPH_H + 4, TFT_BLACK);

    // Desenha moldura
    tft.drawRect(GRAPH_X0, GRAPH_Y0, GRAPH_W, GRAPH_H, TFT_DARKGREY);

    // Eixos simples
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(GRAPH_X0, GRAPH_Y0 - 10);
    tft.print("Forca");
    tft.setCursor(GRAPH_X0 + GRAPH_W - 40, GRAPH_Y0 + GRAPH_H + 2);
    tft.print("Desloc.");

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
                                float calibFactor)
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
        tft.printf("(%.1f kg)\n", SCALE_CALIB_REF_KG);
        tft.println("Click encoder = CALIB.");
        tft.println("Longo = sair.");
    } else {
        tft.println("Calibracao concluida.");
        tft.println("Click encoder = voltar ao menu.");
        tft.println("Longo = voltar ao menu.");
    }
}
