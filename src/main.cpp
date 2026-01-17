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
    "Calibrar balanca",
    "Teste hardware"
};
static const int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
static int menuIndex = 0;

// Para navegação do menu
static long lastEncPosRaw = 0;

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
    const float initialPositionMm = 30.0f;  // Posição REAL inicial para colocar a mola
    const float forceThresholdKg = 0.1f;    // Força mínima para detectar contato da mola
    const float detectionSpeedUsMicros = 1000;  // Velocidade lenta para detecção
    const int   maxSamples    = MAX_GRAPH_SAMPLES;

    // Variáveis críticas de posição
    float motorRealPositionMm = 0.0f;        // Posição REAL do motor no espaço físico
    float springContactMotorPosRealMm = 0.0f; // Posição REAL do motor quando detecta contato da mola
    float moldReadingPositionMm = 0.0f;      // Posição de LEITURA da mola (0mm = contato)

    // Prepara tela
    uiManager.drawTestStatus(0.0f, compressionMm, 0.0f, 0.0f, true, false);
    uiManager.clearGraphArea();

    // ETAPA 1: HOMING - Busca lentamente o micro switch de home, decrementando a posição
    Serial.println("[TESTE] Etapa 1: Homing lento...");
    stepperManager.homeToEndstop((long)(STEPPER_MAX_TRAVEL_MM * stepperManager.getStepsPerMm()));
    
    // Verifica se homing foi bem-sucedido
    if (!stepperManager.wasLastHomingSuccessful()) {
        Serial.println("[TESTE] ERRO: Homing falhou! Verifique endstop e configuracao STEPPER_HOME_DIR_INT.");
        uiManager.drawTestStatus(0.0f, compressionMm, 0.0f, 0.0f, false, false);
        delay(2000);
        return;
    }
    
    motorRealPositionMm = stepperManager.getPositionMm();
    Serial.print("[TESTE] Home concluido. Posição REAL do motor: ");
    Serial.print(motorRealPositionMm);
    Serial.println(" mm");

    // ETAPA 2: Posiciona em velocidade média para 30mm inicialmente
    Serial.println("[TESTE] Etapa 2: Posicionando para 30 mm (posição REAL)...");
    stepperManager.moveToPositionMm(initialPositionMm, 600);  // Velocidade média (600 µs)
    motorRealPositionMm = stepperManager.getPositionMm();
    Serial.print("[TESTE] Motor posicionado em posição REAL: ");
    Serial.print(motorRealPositionMm);
    Serial.println(" mm");

    // ETAPA 3: Mensagem no LCD para o usuário posicionar a mola
    Serial.println("[TESTE] Etapa 3: Aguardando posicionamento da mola pelo usuário...");
    uiManager.clearScreen();
    uiManager.drawText("=== Posicionar Mola ===", 10, 10, TFT_YELLOW, 2);
    uiManager.drawText("Motor em 30 mm", 10, 50, TFT_WHITE, 1);
    uiManager.drawText("Coloque a mola", 10, 70, TFT_WHITE, 1);
    uiManager.drawText("entre as plataformas", 10, 85, TFT_WHITE, 1);
    uiManager.drawText("", 10, 110, TFT_WHITE, 1);
    uiManager.drawText("Click encoder", 10, 130, TFT_CYAN, 2);
    uiManager.drawText("para confirmar", 10, 150, TFT_CYAN, 2);

    // ETAPA 4: Aguarda confirmação do usuário apertando o botão do encoder
    bool springReadyConfirmed = false;
    unsigned long timeout = millis() + 120000;  // Timeout de 2 minutos
    
    while (!springReadyConfirmed && millis() < timeout) {
        encoderManager.update();
        
        // Verifica se usuário clicou no botão do encoder
        if (encoderManager.wasButtonClicked()) {
            Serial.println("[TESTE] Mola posicionada confirmada pelo usuário.");
            springReadyConfirmed = true;
            break;
        }
        
        // Verifica se usuário fez clique longo (cancelar)
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[TESTE] Teste cancelado pelo usuário.");
            return;
        }
        
        delay(50);
    }
    
    if (!springReadyConfirmed) {
        Serial.println("[TESTE] ERRO: Timeout aguardando confirmação da mola.");
        uiManager.clearScreen();
        uiManager.drawText("Timeout!", 10, 10, TFT_RED, 2);
        delay(2000);
        return;
    }
    
    // ETAPA 5: Tara a balança COM a mola já posicionada
    scaleManager.tare();
    Serial.println("[TESTE] Tara feita com mola posicionada.");
    
    // ETAPA 6: DETECÇÃO AUTOMÁTICA DO PONTO DE CONTATO DA MOLA
    Serial.println("[TESTE] Etapa 6: Buscando ponto de contato da mola...");
    uiManager.clearScreen();
    uiManager.drawText("Buscando mola...", 10, 10, TFT_CYAN, 2);
    
    bool springContactDetected = false;
    float maxMotorSearchPosRealMm = initialPositionMm - compressionMm;  // Limite mínimo seguro (20mm)
    
    // Desce o motor lentamente observando a força
    while (motorRealPositionMm > maxMotorSearchPosRealMm && !springContactDetected) {
        encoderManager.update();
        scaleManager.update();
        
        // Desce 0.5mm por iteração
        motorRealPositionMm -= 0.5f;
        stepperManager.moveToPositionMm(motorRealPositionMm, (uint16_t)detectionSpeedUsMicros);
        
        float currentForceKg = scaleManager.getWeightKg();
        
        // Verifica se detectou a mola
        if (currentForceKg > forceThresholdKg) {
            Serial.print("[TESTE] MOLA DETECTADA! Força: ");
            Serial.print(currentForceKg, 3);
            Serial.print(" kg em posição REAL: ");
            Serial.print(motorRealPositionMm, 1);
            Serial.println(" mm");
            
            springContactDetected = true;
            springContactMotorPosRealMm = motorRealPositionMm;
            break;
        }
        
        delay(50);
    }
    
    if (!springContactDetected) {
        Serial.println("[TESTE] ERRO: Não foi possível detectar o ponto de contato da mola!");
        uiManager.clearScreen();
        uiManager.drawText("Mola nao", 10, 10, TFT_RED, 2);
        uiManager.drawText("detectada!", 10, 30, TFT_RED, 2);
        delay(2000);
        return;
    }
    
    // ETAPA 7: Retorna à posição de tara (célula = 0kg)
    Serial.println("[TESTE] Etapa 7: Retornando à posição de tara...");
    uiManager.clearScreen();
    uiManager.drawText("Retornando tara...", 10, 10, TFT_CYAN, 2);
    
    bool taraReached = false;
    float taraThresholdKg = 0.02f;  // Margem de tara aceitável
    
    // Sobe o motor lentamente observando a força
    while (motorRealPositionMm < initialPositionMm && !taraReached) {
        encoderManager.update();
        scaleManager.update();
        
        // Sobe 0.5mm por iteração
        motorRealPositionMm += 0.5f;
        if (motorRealPositionMm > initialPositionMm) {
            motorRealPositionMm = initialPositionMm;
        }
        stepperManager.moveToPositionMm(motorRealPositionMm, (uint16_t)detectionSpeedUsMicros);
        
        float currentForceKg = scaleManager.getWeightKg();
        
        // Verifica se voltou à tara
        if (currentForceKg < taraThresholdKg) {
            Serial.print("[TESTE] TARA ATINGIDA! Força: ");
            Serial.print(currentForceKg, 3);
            Serial.print(" kg em posição REAL: ");
            Serial.print(motorRealPositionMm, 1);
            Serial.println(" mm");
            
            taraReached = true;
            break;
        }
        
        delay(50);
    }
    
    if (!taraReached) {
        Serial.println("[TESTE] AVISO: Não retornou completamente à tara, continuando...");
    }
    
    // ETAPA 8: ZERA a referência de posição da mola
    Serial.println("[TESTE] Etapa 8: Zerando referência de posição da mola...");
    Serial.print("[TESTE] Offset de detecção (diferença posição REAL): ");
    Serial.print(springContactMotorPosRealMm, 1);
    Serial.println(" mm");
    
    // A partir daqui, a posição de LEITURA = posição REAL - springContactMotorPosRealMm
    // moldReadingPositionMm = 0mm quando o motor está em springContactMotorPosRealMm (contato com mola)
    
    Serial.println("[TESTE] Sistema pronto para iniciar compressão com detecção automática!");

    // ETAPA 9: Amostragem de compressão de 10mm com posições de LEITURA vs REAL
    Serial.println("[TESTE] Etapa 9: Iniciando amostragem de compressão (10mm)...");
    uiManager.drawTestStatus(0.0f, compressionMm, 0.0f, 0.0f, true, false);
    uiManager.clearGraphArea();

    // Amostragem de 10mm (1mm por passo)
    // IMPORTANTE: Usar posição REAL do motor, mas exibir posição de LEITURA (compressão)
    // posição de LEITURA = posição REAL - springContactMotorPosRealMm
    
    const int numSteps = (int)compressionMm;  // 10 passos de 1mm cada
    float maxMotorCompressionRealMm = springContactMotorPosRealMm - compressionMm;  // Limite mínimo seguro
    
    float lastK_kgf_mm = 0.0f;
    float lastK_N_mm    = 0.0f;
    float lastForceKg   = 0.0f;

    Serial.print("[TESTE] Comprimindo de leitura 0mm até leitura ");
    Serial.print(compressionMm);
    Serial.print(" mm");
    Serial.print(" | Posição REAL de ");
    Serial.print(springContactMotorPosRealMm, 1);
    Serial.print(" até ");
    Serial.print(maxMotorCompressionRealMm, 1);
    Serial.println(" mm");

    for (int i = 0; i <= numSteps; ++i) {
        encoderManager.update();
        
        // Compressão de LEITURA: 0mm, 1mm, 2mm, ..., 10mm
        float moldCompressionReadingMm = (float)i;
        
        // Posição REAL do motor correspondente
        float motorRealTargetMm = springContactMotorPosRealMm - moldCompressionReadingMm;
        
        // Proteção de segurança: não ultrapassar o limite
        if (motorRealTargetMm < maxMotorCompressionRealMm) {
            motorRealTargetMm = maxMotorCompressionRealMm;
            moldCompressionReadingMm = springContactMotorPosRealMm - motorRealTargetMm;
        }
        
        // Move eixo para a posição REAL calculada
        Serial.print("[TESTE] Passo ");
        Serial.print(i);
        Serial.print(": Leitura ");
        Serial.print(moldCompressionReadingMm, 1);
        Serial.print(" mm | Motor REAL ");
        Serial.print(motorRealTargetMm, 1);
        Serial.println(" mm");
        stepperManager.moveToPositionMm(motorRealTargetMm);
        
        // Aguarda estabilização
        delay(100);

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

        // Calcula K com base na compressão de LEITURA (apenas se > 0.5mm)
        float currentK_kgf_mm = 0.0f;
        float currentK_N_mm    = 0.0f;
        if (moldCompressionReadingMm > 0.5f) {
            currentK_kgf_mm = avgKg / moldCompressionReadingMm;
            currentK_N_mm   = currentK_kgf_mm * 9.80665f;
            lastK_kgf_mm    = currentK_kgf_mm;
            lastK_N_mm      = currentK_N_mm;
        }

        // Log no Serial (com ambas as posições para debug)
        Serial.print("[TESTE] Leitura: ");
        Serial.print(moldCompressionReadingMm, 1);
        Serial.print(" mm | Motor REAL: ");
        Serial.print(motorRealTargetMm, 1);
        Serial.print(" mm | Força: ");
        Serial.print(avgKg, 2);
        Serial.print(" kg | K: ");
        Serial.print(lastK_kgf_mm, 3);
        Serial.println(" kgf/mm");

        // Atualiza texto da tela com posição de LEITURA e K atual
        uiManager.drawTestStatus(avgKg,
                                 moldCompressionReadingMm,  // Mostra compressão de LEITURA
                                 lastK_kgf_mm,
                                 lastK_N_mm,
                                 true,
                                 false);

        // Normaliza para o grafico (0..1)
        float xNorm = moldCompressionReadingMm / compressionMm;
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

    // ETAPA 10: Retorna motor para posição inicial 30mm para o usuário retirar a mola
    Serial.println("[TESTE] Etapa 10: Retornando motor para posição inicial (30 mm) para remover mola...");
    uiManager.drawTestStatus(lastForceKg,
                             compressionMm,
                             lastK_kgf_mm,
                             lastK_N_mm,
                             false,
                             true);
    
    Serial.print("[TESTE] Motor movendo de ");
    Serial.print(springContactMotorPosRealMm, 1);
    Serial.print(" mm para 30 mm...");
    stepperManager.moveToPositionMm(initialPositionMm);
    Serial.println(" Pronto!");
    
    // Aguarda um pouco para o motor chegar na posição
    delay(500);
    
    // ETAPA 11: Exibe mensagem para retirar a mola
    Serial.println("[TESTE] Etapa 11: Aguardando retirada da mola pelo usuário...");
    
    // Desenha alerta na área inferior da tela, preservando gráfico e dados
    uiManager.drawText(">>> Retire a mola <<<", 20, 180, TFT_YELLOW, 1);
    uiManager.drawText("Click para continuar", 20, 200, TFT_CYAN, 1);
    
    // Aguarda clique do usuário ou timeout
    bool userConfirmedRemoval = false;
    unsigned long removalTimeout = millis() + 60000;  // 1 minuto de timeout
    
    while (!userConfirmedRemoval && millis() < removalTimeout) {
        encoderManager.update();
        
        // Verifica se usuário clicou
        if (encoderManager.wasButtonClicked()) {
            Serial.println("[TESTE] Usuário confirmou retirada da mola.");
            userConfirmedRemoval = true;
            break;
        }
        
        delay(50);
    }
    
    if (!userConfirmedRemoval) {
        Serial.println("[TESTE] Timeout esperando retirada da mola, continuando...");
    }

    // Tela final com resumo
    uiManager.clearScreen();
    uiManager.drawText("=== Teste Concluido ===", 10, 10, TFT_GREEN, 2);
    uiManager.drawText("", 10, 40, TFT_WHITE, 1);
    char kDisplay[64];
    snprintf(kDisplay, sizeof(kDisplay), "K: %.3f kgf/mm", lastK_kgf_mm);
    uiManager.drawText(kDisplay, 10, 60, TFT_CYAN, 2);
    
    char kNewtons[64];
    snprintf(kNewtons, sizeof(kNewtons), "K: %.3f N/mm", lastK_N_mm);
    uiManager.drawText(kNewtons, 10, 85, TFT_CYAN, 1);
    
    char forceDisplay[64];
    snprintf(forceDisplay, sizeof(forceDisplay), "Forca max: %.2f kg", lastForceKg);
    uiManager.drawText(forceDisplay, 10, 110, TFT_WHITE, 1);
    
    uiManager.drawText("", 10, 140, TFT_WHITE, 1);
    uiManager.drawText("Click encoder para menu", 10, 160, TFT_YELLOW, 1);

    Serial.println("[TESTE] Teste de mola concluido com sucesso!");
    Serial.print("Ponto de contato da mola (Motor REAL): ");
    Serial.print(springContactMotorPosRealMm, 1);
    Serial.println(" mm");
    Serial.print("K (kgf/mm): ");
    Serial.println(lastK_kgf_mm);
    Serial.print("K (N/mm): ");
    Serial.println(lastK_N_mm);

    // Aguarda um pouco antes de voltar ao menu (ou usuário aperta BTN)
    delay(500);
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
