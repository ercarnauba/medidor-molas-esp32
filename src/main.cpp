#include <Arduino.h>

#include "config.h"
#include "scale_manager.h"
#include "stepper_manager.h"
#include "encoder_manager.h"
#include "ui_manager.h"
#include "test_mola_grafset.h"

// ---- ESTADOS ----

enum AppState {
    APP_STATE_MENU = 0,
    APP_STATE_IDLE,
    APP_STATE_WAITING_TO_RETURN_MENU
};

static AppState appState = APP_STATE_MENU;

// Menu simples
static const char* MENU_ITEMS[] = {
    "Teste mola (k)",
    "Calibrar balanca",
    "Teste hardware"
};
static const int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
static int menuIndex = 0;

// Para navegação do menu
static long lastEncPosRaw = 0;

// Grafset para teste de mola
static TestMolaGrafset testMolaGrafset;

// ---- Prototipos ----
void runSpringTestWithGraph();
void runLoadcellCalibration();
void runHardwareTest();

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

    // Motor já está habilitado após stepperManager.begin()

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
    scaleManager.update();

    long encPosRaw = encoderManager.getPosition();
    long deltaEnc  = encPosRaw - lastEncPosRaw;
    if (deltaEnc != 0) {
        lastEncPosRaw = encPosRaw;
    }

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

        // Seleção via click - APENAS no menu
        if (encoderManager.wasButtonClicked()) {
            if (menuIndex == 0) {
                // Teste da mola com Grafset
                // Aguarda um momento para evitar que o mesmo clique inicie o teste
                delay(100);
                encoderManager.wasButtonClicked(); // Consome qualquer clique residual
                encoderManager.wasButtonLongPressed(); // Consome long press também
                testMolaGrafset.start();
                appState = APP_STATE_IDLE;
            } else if (menuIndex == 1) {
                // Calibrar balanca
                runLoadcellCalibration();
                // Ao terminar, volta ao menu
                appState = APP_STATE_MENU;
                uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
            } else if (menuIndex == 2) {
                // Teste de hardware
                runHardwareTest();
                // Ao terminar, volta ao menu
                appState = APP_STATE_MENU;
                uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
            }
        }

        break;
    }

    case APP_STATE_IDLE: {
        // Grafset rodando
        testMolaGrafset.tick();
        
        // Se terminou, aguarda clique para voltar ao menu
        if (testMolaGrafset.isFinished()) {
            appState = APP_STATE_WAITING_TO_RETURN_MENU;
        }
        break;
    }

    case APP_STATE_WAITING_TO_RETURN_MENU: {
        // Aguarda clique do usuário para retornar ao menu
        // NÃO chamar update() aqui - já é feito no loop principal!
        
        if (encoderManager.wasButtonClicked()) {
            Serial.println("[MENU] Retornando ao menu...");
            testMolaGrafset.reset();
            appState = APP_STATE_MENU;
            menuIndex = 0;
            uiManager.drawMenu(MENU_ITEMS, MENU_COUNT, menuIndex);
        }
        break;
    }

    default:
        appState = APP_STATE_MENU;
        break;
    }
}

// ========================================================
//  CALIBRAÇÃO DA BALANÇA (HX711)
// ========================================================

void runLoadcellCalibration() {
    Serial.println("[CALIB] Iniciando calibracao da balanca...");

    // ---- ETAPA 1: SELEÇÃO DO PESO ----
    int weightIndex = 2; // Padrão: 2.0 kg (índice 2)
    bool weightSelected = false;
    long lastEncPos = encoderManager.getPosition();

    uiManager.drawWeightSelectionScreen(SCALE_CALIB_WEIGHTS, SCALE_CALIB_WEIGHTS_COUNT, weightIndex);

    while (!weightSelected) {
        encoderManager.update();

        // Navegação com encoder
        long encPos = encoderManager.getPosition();
        long delta = encPos - lastEncPos;
        if (delta != 0) {
            lastEncPos = encPos;
            if (delta > 0) {
                weightIndex++;
                if (weightIndex >= SCALE_CALIB_WEIGHTS_COUNT) weightIndex = 0;
            } else {
                weightIndex--;
                if (weightIndex < 0) weightIndex = SCALE_CALIB_WEIGHTS_COUNT - 1;
            }
            uiManager.drawWeightSelectionScreen(SCALE_CALIB_WEIGHTS, SCALE_CALIB_WEIGHTS_COUNT, weightIndex);
        }

        // Click confirma seleção
        if (encoderManager.wasButtonClicked()) {
            weightSelected = true;
            Serial.print("[CALIB] Peso selecionado: ");
            Serial.print(SCALE_CALIB_WEIGHTS[weightIndex]);
            Serial.println(" kg");
        }

        // Long press cancela
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[CALIB] Cancelado pelo usuario.");
            return;
        }

        delay(50);
    }

    float selectedWeight = SCALE_CALIB_WEIGHTS[weightIndex];

    // ---- ETAPA 2: CALIBRAÇÃO COM O PESO ESCOLHIDO ----
    uint8_t stage = 0;
    bool done = false;

    while (!done) {
        encoderManager.update();
        // Atualiza leitura atual só para mostrar na tela
        scaleManager.update();
        float kg = scaleManager.getWeightKg();

        uiManager.drawCalibScreen(stage, kg, scaleManager.getCalibFactor(), selectedWeight);

        // Clique do encoder avanca etapas
        if (encoderManager.wasButtonClicked()) {
            if (stage == 0) {
                // Fazer tara sem peso
                Serial.println("[CALIB] TARA sem peso...");
                scaleManager.tare();
                stage = 1;
            } else if (stage == 1) {
                // Peso de referencia
                Serial.print("[CALIB] Lendo peso referencia (");
                Serial.print(selectedWeight);
                Serial.println(" kg)...");

                // Media antes, só por curiosidade
                const int N = 10;
                float soma = 0.0f;
                for (int i = 0; i < N; ++i) {
                    scaleManager.update();
                    soma += scaleManager.getWeightKg();
                    delay(50);
                }
                float antesKg = soma / (float)N;
                Serial.print("[CALIB] Antes calib: ");
                Serial.print(antesKg, 3);
                Serial.println(" kg");

                scaleManager.calibrateWithKnownWeight(selectedWeight);
                scaleManager.saveCalibrationToEEPROM();

                Serial.print("[CALIB] Novo fator: ");
                Serial.println(scaleManager.getCalibFactor(), 4);
                Serial.println("[CALIB] Concluido.");

                stage = 2;
            } else {
                // Stage 2: finalizado, qualquer clique apenas mantem
            }
        }

        // Long press sai em qualquer etapa
        if (encoderManager.wasButtonLongPressed()) {
            done = true;
        }

        // Encoder: se concluído (stage 2), clique retorna ao menu
        if (stage == 2 && encoderManager.wasButtonClicked()) {
            done = true;
        }

        delay(50);
    }

    Serial.println("[CALIB] Saindo da calibracao, voltando ao menu.");
}

// ============================
// runHardwareTest
// ============================
void runHardwareTest() {
    Serial.println("[HW_TEST] Iniciando teste de hardware...");
    
    // Limpa a tela e desenha o cabeçalho
    uiManager.clearScreen();
    uiManager.drawText("=== Teste Hardware ===", 10, 5, TFT_YELLOW, 2);
    uiManager.drawText("Encoder: +/- 1mm", 5, 35, TFT_WHITE, 1);
    uiManager.drawText("Botao: Sair teste", 5, 50, TFT_WHITE, 1);
    
    // Labels fixos na tela
    uiManager.drawText("Forca:", 5, 80, TFT_CYAN, 2);
    uiManager.drawText("Posicao:", 5, 110, TFT_GREEN, 2);
    
    // Instruções no rodapé
    uiManager.drawText("Click encoder = Sair", 5, 200, TFT_YELLOW, 1);
    
    // Posição inicial do motor (pode ser qualquer posição atual)
    float currentPositionMm = stepperManager.getPositionMm();
    long lastEncPosRaw = encoderManager.getPosition();
    
    bool done = false;
    unsigned long lastUpdate = 0;
    
    while (!done) {
        encoderManager.update();
        scaleManager.update();
        
        // Verifica clique do encoder para sair
        if (encoderManager.wasButtonClicked()) {
            done = true;
            break;
        }
        
        // Atualiza display a cada 100ms
        unsigned long now = millis();
        if (now - lastUpdate >= 100) {
            lastUpdate = now;
            
            // Lê a célula de carga em kg e converte para gramas
            float forceKg = scaleManager.getWeightKg();
            float forceG = forceKg * 1000.0f;
            
            // Mostra a força na tela LCD e Serial
            char bufForce[32];
            snprintf(bufForce, sizeof(bufForce), "%.2f g      ", forceG);
            uiManager.drawText(bufForce, 110, 80, TFT_CYAN, 2);
            
            Serial.print("[HW_TEST] Forca: ");
            Serial.print(forceG, 2);
            Serial.println(" g");
            
            // Mostra a posição atual do motor na tela LCD e Serial
            float displayPosition = stepperManager.getPositionMm();
            char bufPos[32];
            snprintf(bufPos, sizeof(bufPos), "%.2f mm      ", displayPosition);
            uiManager.drawText(bufPos, 110, 110, TFT_GREEN, 2);
            
            Serial.print("[HW_TEST] Posicao: ");
            Serial.print(displayPosition, 2);
            Serial.println(" mm");
        }
        
        // Verifica movimento do encoder
        long encPosRaw = encoderManager.getPosition();
        long deltaEnc = encPosRaw - lastEncPosRaw;
        
        if (deltaEnc != 0) {
            lastEncPosRaw = encPosRaw;
            
            // Cada click do encoder = 1mm de movimento
            // deltaEnc positivo = horário = incrementa posição
            // deltaEnc negativo = anti-horário = decrementa posição
            float movementMm = (float)deltaEnc * 1.0f;
            currentPositionMm += movementMm;
            
            Serial.print("[HW_TEST] Encoder delta: ");
            Serial.print(deltaEnc);
            Serial.print(", Movendo motor para: ");
            Serial.print(currentPositionMm);
            Serial.println(" mm");
            
            // Move o motor para a nova posição
            stepperManager.moveToPositionMm(currentPositionMm);
        }
        
        delay(50);
    }
    
    Serial.println("[HW_TEST] Teste de hardware finalizado.");
}
