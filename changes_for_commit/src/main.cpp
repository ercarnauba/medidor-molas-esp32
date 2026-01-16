#include <Arduino.h>

#include "config.h"
#include "scale_manager.h"
#include "stepper_manager.h"
#include "encoder_manager.h"
#include "ui_manager.h"

// ---- ESTADOS ----

enum AppState {
    APP_STATE_MENU = 0,
    APP_STATE_IDLE
};

static AppState appState = APP_STATE_MENU;

// Menu simples
static const char* MENU_ITEMS[] = {
    "Teste mola (k)",
    "Calibrar balanca"
};
static const int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
static int menuIndex = 0;

// Para navegação do menu
static long lastEncPosRaw = 0;

// ---- Prototipos ----
void runSpringTestWithGraph();
void runLoadcellCalibration();

// Botão frontal removido: retorno ao menu será pelo botão do encoder

// ---- SETUP ----
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("=== Medidor de mola - Inicializando ===");

    scaleManager.begin();
    stepperManager.begin();
    encoderManager.begin();
    uiManager.begin();

    stepperManager.enable(true);

    // Posição inicial do encoder
    lastEncPosRaw = encoderManager.getPosition();

    // Mostra menu inicial
    uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);

    appState = APP_STATE_MENU;

    Serial.print("Fator de calib inicial: ");
    Serial.println(scaleManager.getCalibFactor(), 4);
}

// ---- LOOP PRINCIPAL ----
void loop() {
    encoderManager.update();

    // Leitura periódica da célula de carga (útil para calib)
    scaleManager.update();

    // Leitura do encoder (para menu)
    long encPosRaw = encoderManager.getPosition();
    long deltaEnc  = encPosRaw - lastEncPosRaw;
    if (deltaEnc != 0) {
        lastEncPosRaw = encPosRaw;
    }

    bool encClick = encoderManager.wasButtonClicked();

    switch (appState) {
    case APP_STATE_MENU: {
        // Navegação de menu via encoder
        if (deltaEnc > 0) {
            menuIndex++;
            if (menuIndex >= MENU_COUNT) menuIndex = 0;
            uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
        } else if (deltaEnc < 0) {
            menuIndex--;
            if (menuIndex < 0) menuIndex = MENU_COUNT - 1;
            uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
        }

        // Seleção via click
        if (encClick) {
            if (menuIndex == 0) {
                // Teste da mola com grafico
                runSpringTestWithGraph();
                // Ao terminar, volta ao menu
                appState = APP_STATE_MENU;
                uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
            } else if (menuIndex == 1) {
                // Calibrar balanca
                runLoadcellCalibration();
                // Ao terminar, volta ao menu
                appState = APP_STATE_MENU;
                uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
            }
        }

        break;
    }

    case APP_STATE_IDLE:
    default:
        // Reservado para futuros modos; por enquanto, mantemos menu como padrão
        appState = APP_STATE_MENU;
        uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
        break;
    }

    // Pequeno delay para aliviar a CPU
    delay(10);
}

// ========================================================
//  TESTE DA MOLA COM GRAFICO
// ========================================================

void runSpringTestWithGraph() {
    Serial.println("[TESTE] Iniciando teste de mola...");

    const float compressionMm = DEFAULT_TEST_COMPRESSION_MM;
    const int   maxSamples    = MAX_GRAPH_SAMPLES;

    // Prepara tela
    uiManager.drawTestStatus(0.0f, compressionMm, 0.0f, 0.0f, true, false);
    uiManager.clearGraphArea();

    // HOMING
    Serial.println("[TESTE] Homing...");
    stepperManager.homeToEndstop((long)(STEPPER_MAX_TRAVEL_MM * stepperManager.getStepsPerMm()));
    scaleManager.tare();
    Serial.println("[TESTE] Home concluido, tara feita.");

    // Prepara amostragem
    // Vamos colher "n" pontos igualmente espaçados entre 0 e compressãoMm
    int numSamples = maxSamples;
    if (numSamples < 2) numSamples = 2;
    if (compressionMm <= 0.0f) return;

    float stepMm = compressionMm / (float)(numSamples - 1);

    float lastK_kgf_mm = 0.0f;
    float lastK_N_mm    = 0.0f;
    float lastForceKg   = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        encoderManager.update();
        float targetPos = stepMm * i;

        // Move eixo para a posição atual
        stepperManager.moveToPositionMm(targetPos);

        // Faz media da leitura em cada ponto
        const int N = 5;
        float soma = 0.0f;
        for (int j = 0; j < N; ++j) {
            scaleManager.update();
            soma += scaleManager.getWeightKg();
            delay(20);
        }
        float avgKg = soma / (float)N;

        lastForceKg = avgKg;

        // Calcula K com base no ponto atual (apenas se deslocamento > 0)
        float currentK_kgf_mm = 0.0f;
        float currentK_N_mm    = 0.0f;
        if (targetPos > 0.1f) {
            currentK_kgf_mm = avgKg / targetPos;
            currentK_N_mm   = currentK_kgf_mm * 9.80665f;
            lastK_kgf_mm    = currentK_kgf_mm;
            lastK_N_mm      = currentK_N_mm;
        }

        // Atualiza texto da tela
        uiManager.drawTestStatus(avgKg,
                                 compressionMm,
                                 lastK_kgf_mm,
                                 lastK_N_mm,
                                 true,
                                 false);

        // Normaliza para o grafico (0..1)
        float xNorm = targetPos / compressionMm;
        float yNorm = avgKg / GRAPH_MAX_FORCE_KG;
        if (yNorm > 1.0f) yNorm = 1.0f;

        // Plota ponto/segmento
        uiManager.plotGraphPoint(xNorm, yNorm, (i == 0));

        // Cancelar pelo botão do encoder (clique ou longo)
        if (encoderManager.wasButtonLongPressed() || encoderManager.wasButtonClicked()) {
            Serial.println("[TESTE] Cancelado pelo encoder.");
            break;
        }
    }

    // Solta a mola retornando ao zero
    Serial.println("[TESTE] Retornando para 0 mm...");
    stepperManager.moveToPositionMm(0.0f);

    // Tela final
    uiManager.drawTestStatus(lastForceKg,
                             compressionMm,
                             lastK_kgf_mm,
                             lastK_N_mm,
                             false,
                             true);

    Serial.println("[TESTE] Concluido.");
    Serial.print("K (kgf/mm): ");
    Serial.println(lastK_kgf_mm);
    Serial.print("K (N/mm): ");
    Serial.println(lastK_N_mm);

    // Aguarda um pouco antes de voltar ao menu (ou usuário aperta BTN)
    delay(500);
}
