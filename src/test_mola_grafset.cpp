#include "test_mola_grafset.h"
#include "scale_manager.h"
#include "stepper_manager.h"
#include "encoder_manager.h"
#include "ui_manager.h"
#include "config.h"

TestMolaGrafset::TestMolaGrafset()
    : currentState(STATE_INITIAL),
      motorRealPositionMm(0.0f),
      springContactMotorPosRealMm(0.0f),
      moldReadingPositionMm(0.0f),
      lastK_kgf_mm(0.0f),
      lastK_N_mm(0.0f),
      lastForceKg(0.0f),
      compressionStepCounter(0),
      stateStartTime(0),
      userInteractionTimeout(0),
      screenShownAwaitSpringPlacement(false),
      screenShownFindSpringContact(false),
      screenShownReturnToTare(false),
      screenShownZeroReference(false),
      screenShownCompressionSampling(false),
      screenShownReturnInitial(false),
      screenShownShowResults(false),
      springReadyConfirmed(false),
      springContactDetected(false),
      taraReached(false),
      userConfirmedRemoval(false),
      compressionSamplingDone(false)
{
}

void TestMolaGrafset::start() {
    Serial.println("[TESTE] Teste de mola selecionado. Aguardando confirmação do usuário...");
    currentState = STATE_READY;
    finished = false;
    
    // Reset de variáveis
    motorRealPositionMm = 0.0f;
    springContactMotorPosRealMm = 0.0f;
    moldReadingPositionMm = 0.0f;
    lastK_kgf_mm = 0.0f;
    lastK_N_mm = 0.0f;
    lastForceKg = 0.0f;
    compressionStepCounter = 0;
    
    // Reset de flags
    screenShownAwaitSpringPlacement = false;
    screenShownFindSpringContact = false;
    screenShownReturnToTare = false;
    screenShownZeroReference = false;
    screenShownCompressionSampling = false;
    screenShownReturnInitial = false;
    screenShownShowResults = false;
    
    springReadyConfirmed = false;
    springContactDetected = false;
    taraReached = false;
    userConfirmedRemoval = false;
    compressionSamplingDone = false;
    
    stateStartTime = millis();
    
    // Exibe tela de confirmação
    uiManager.clearScreen();
    uiManager.drawText("=== Teste de Mola ===", 10, 40, TFT_YELLOW, 2);
    uiManager.drawText("", 10, 80, TFT_WHITE, 1);
    uiManager.drawText("Click encoder", 10, 120, TFT_CYAN, 2);
    uiManager.drawText("para iniciar", 10, 145, TFT_CYAN, 2);
    uiManager.drawText("", 10, 180, TFT_WHITE, 1);
    uiManager.drawText("Long press para cancelar", 10, 220, TFT_RED, 1);
}

void TestMolaGrafset::tick() {
    if (finished) return;
    
    // Atualizar sensores (NÃO chamar encoderManager.update aqui - é no loop principal)
    scaleManager.update();
    
    switch (currentState) {
        case STATE_READY:
            executeStateReady();
            break;
        case STATE_INITIAL:
            executeStateInitial();
            break;
        case STATE_HOMING:
            executeStateHoming();
            break;
        case STATE_RETURN_30MM:
            executeStateReturn30mm();
            break;
        case STATE_AWAIT_SPRING_PLACEMENT:
            executeStateAwaitSpringPlacement();
            break;
        case STATE_TARE:
            executeStateTare();
            break;
        case STATE_FIND_SPRING_CONTACT:
            executeStateFindSpringContact();
            break;
        case STATE_RETURN_TO_TARE:
            executeStateReturnToTare();
            break;
        case STATE_ZERO_REFERENCE:
            executeStateZeroReference();
            break;
        case STATE_COMPRESSION_SAMPLING:
            executeStateCompressionSampling();
            break;
        case STATE_RETURN_INITIAL:
            executeStateReturnInitial();
            break;
        case STATE_SHOW_RESULTS:
            executeStateShowResults();
            break;
        default:
            currentState = STATE_INITIAL;
            break;
    }
}

void TestMolaGrafset::reset() {
    Grafset::reset();
    currentState = STATE_INITIAL;
}

// ============== ETAPA PRONTO (READY) ==============
void TestMolaGrafset::executeStateReady() {
    static bool screenShown = false;
    
    if (!screenShown) {
        Serial.println("[TESTE] Aguardando confirmação do usuário para iniciar...");
        screenShown = true;
    }
    
    // Verifica clique para iniciar
    if (encoderManager.wasButtonClicked()) {
        Serial.println("[TESTE] Usuário confirmou. Iniciando teste...");
        currentState = STATE_INITIAL;
        screenShown = false;
        return;
    }
    
    // Verifica long press para cancelar
    if (encoderManager.wasButtonLongPressed()) {
        Serial.println("[TESTE] Teste cancelado pelo usuário.");
        finished = true;
        screenShown = false;
        return;
    }
}

// ============== ETAPA INICIAL ==============
void TestMolaGrafset::executeStateInitial() {
    Serial.println("[TESTE] Iniciando teste de mola...");
    Serial.println("[TESTE] Etapa 1: Buscar HOME (descer ate micro switch)...");
    stateStartTime = millis();
    currentState = STATE_HOMING;
}

// ============== ETAPA HOMING ==============
void TestMolaGrafset::executeStateHoming() {
    // Executa homing apenas uma vez
    static bool homingExecuted = false;
    
    if (!homingExecuted) {
        stepperManager.homeToEndstop((long)(STEPPER_HOME_TRAVEL_MM * stepperManager.getStepsPerMm()), 133);
        homingExecuted = true;
        
        if (!stepperManager.wasLastHomingSuccessful()) {
            Serial.println("[TESTE] ERRO: Homing falhou!");
            uiManager.drawTestStatus(0.0f, DEFAULT_TEST_COMPRESSION_MM, 0.0f, 0.0f, false, false);
            delay(2000);
            finished = true;
            homingExecuted = false;
            return;
        }
        
        motorRealPositionMm = stepperManager.getPositionMm();
        Serial.print("[TESTE] Etapa 2: HOME = 0mm definido. Posição REAL atual: ");
        Serial.print(motorRealPositionMm);
        Serial.println(" mm");
    }
    
    // Transição automática
    currentState = STATE_RETURN_30MM;
    homingExecuted = false;
}

// ============== RETORNA 30MM ==============
void TestMolaGrafset::executeStateReturn30mm() {
    static bool moveExecuted = false;
    
    if (!moveExecuted) {
        Serial.println("[TESTE] Etapa 3: Retornando 30 mm...");
        stepperManager.moveToPositionMm(30.0f, 133);
        moveExecuted = true;
        
        motorRealPositionMm = stepperManager.getPositionMm();
        Serial.print("[TESTE] Motor posicionado em: ");
        Serial.print(motorRealPositionMm);
        Serial.println(" mm");
    }
    
    currentState = STATE_AWAIT_SPRING_PLACEMENT;
    moveExecuted = false;
}

// ============== AGUARDA POSICIONAMENTO DA MOLA ==============
void TestMolaGrafset::executeStateAwaitSpringPlacement() {
    if (!screenShownAwaitSpringPlacement) {
        Serial.println("[TESTE] Etapa 4: Aguardando usuario inserir a mola...");
        uiManager.clearScreen();
        uiManager.drawText("=== Posicionar Mola ===", 10, 10, TFT_YELLOW, 2);
        uiManager.drawText("Motor em 30 mm", 10, 50, TFT_WHITE, 1);
        uiManager.drawText("Coloque a mola", 10, 70, TFT_WHITE, 1);
        uiManager.drawText("entre as plataformas", 10, 85, TFT_WHITE, 1);
        uiManager.drawText("", 10, 110, TFT_WHITE, 1);
        uiManager.drawText("Click encoder", 10, 130, TFT_CYAN, 2);
        uiManager.drawText("para confirmar", 10, 150, TFT_CYAN, 2);
        
        userInteractionTimeout = millis() + 120000;  // 2 minutos
        screenShownAwaitSpringPlacement = true;
    }
    
    // Verifica clique
    if (encoderManager.wasButtonClicked()) {
        Serial.println("[TESTE] Mola posicionada confirmada pelo usuário.");
        springReadyConfirmed = true;
        currentState = STATE_TARE;
        screenShownAwaitSpringPlacement = false;
        return;
    }
    
    // Verifica long press (cancelar)
    if (encoderManager.wasButtonLongPressed()) {
        Serial.println("[TESTE] Teste cancelado pelo usuário.");
        finished = true;
        screenShownAwaitSpringPlacement = false;
        return;
    }
    
    // Timeout
    if (millis() > userInteractionTimeout) {
        Serial.println("[TESTE] ERRO: Timeout aguardando confirmação.");
        finished = true;
        screenShownAwaitSpringPlacement = false;
        return;
    }
}

// ============== TARA ==============
void TestMolaGrafset::executeStateTare() {
    static bool tareExecuted = false;
    
    if (!tareExecuted) {
        scaleManager.tare();
        Serial.println("[TESTE] Etapa 5: Tara feita com a mola posicionada.");
        tareExecuted = true;
    }
    
    currentState = STATE_FIND_SPRING_CONTACT;
    tareExecuted = false;
}

// ============== BUSCA PONTO DE CONTATO ==============
void TestMolaGrafset::executeStateFindSpringContact() {
    if (!screenShownFindSpringContact) {
        Serial.println("[TESTE] Etapa 6: Iniciando busca da mola...");
        uiManager.clearScreen();
        uiManager.drawText("Buscando mola...", 10, 10, TFT_CYAN, 2);
        screenShownFindSpringContact = true;
    }
    
    static int chunkSteps = 0;
    static float maxMotorSearchPosRealMm = 5.0f;
    static bool searchStarted = false;
    
    if (!searchStarted) {
        chunkSteps = (int)(0.25f * stepperManager.getStepsPerMm());
        if (chunkSteps < 1) chunkSteps = 1;
        searchStarted = true;
    }
    
    if (motorRealPositionMm > maxMotorSearchPosRealMm) {
        // Check for user cancellation
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[TESTE] Cancelado pelo usuario durante busca.");
            finished = true;
            searchStarted = false;
            screenShownFindSpringContact = false;
            return;
        }
        
        // Move down
        stepperManager.moveSteps(chunkSteps, STEPPER_DIR_BACKWARD, 300);
        motorRealPositionMm = stepperManager.getPositionMm();
        
        float currentForceKg = scaleManager.getWeightKg();
        
        if (currentForceKg >= SPRING_CONTACT_FORCE_KG) {
            springContactDetected = true;
            springContactMotorPosRealMm = motorRealPositionMm;
            Serial.print("[TESTE] MOLA DETECTADA! Forca: ");
            Serial.print(currentForceKg, 3);
            Serial.print(" kg em posicao REAL: ");
            Serial.print(springContactMotorPosRealMm, 1);
            Serial.println(" mm");
            
            currentState = STATE_RETURN_TO_TARE;
            searchStarted = false;
            screenShownFindSpringContact = false;
        }
    } else if (!springContactDetected) {
        Serial.println("[TESTE] ERRO: Não foi possível detectar o ponto de contato!");
        finished = true;
        searchStarted = false;
        screenShownFindSpringContact = false;
    }
}

// ============== RETORNA À TARA ==============
void TestMolaGrafset::executeStateReturnToTare() {
    if (!screenShownReturnToTare) {
        Serial.println("[TESTE] Etapa 7: Recua ate nao tocar...");
        uiManager.clearScreen();
        uiManager.drawText("Retornando tara...", 10, 10, TFT_CYAN, 2);
        screenShownReturnToTare = true;
    }
    
    static int chunkSteps = 0;
    static float initialPositionMm = 30.0f;
    static bool returnStarted = false;
    
    if (!returnStarted) {
        chunkSteps = (int)(0.25f * stepperManager.getStepsPerMm());
        if (chunkSteps < 1) chunkSteps = 1;
        returnStarted = true;
    }
    
    if (motorRealPositionMm < initialPositionMm) {
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[TESTE] Cancelado durante recuo.");
            finished = true;
            returnStarted = false;
            screenShownReturnToTare = false;
            return;
        }
        
        stepperManager.moveSteps(chunkSteps, STEPPER_DIR_FORWARD, 300);
        motorRealPositionMm = stepperManager.getPositionMm();
        
        float currentForceKg = scaleManager.getWeightKg();
        
        if (currentForceKg <= SPRING_TARA_THRESHOLD_KG) {
            Serial.print("[TESTE] TARA ATINGIDA! Força: ");
            Serial.print(currentForceKg, 3);
            Serial.print(" kg em posição REAL: ");
            Serial.print(motorRealPositionMm, 1);
            Serial.println(" mm");
            
            taraReached = true;
            currentState = STATE_ZERO_REFERENCE;
            returnStarted = false;
            screenShownReturnToTare = false;
        }
    } else if (!taraReached) {
        Serial.println("[TESTE] AVISO: Não retornou completamente à tara, continuando...");
        currentState = STATE_ZERO_REFERENCE;
        returnStarted = false;
        screenShownReturnToTare = false;
    }
}

// ============== ZERA REFERÊNCIA ==============
void TestMolaGrafset::executeStateZeroReference() {
    if (!screenShownZeroReference) {
        Serial.println("[TESTE] Etapa 8: Zerando referencia de posicao da mola...");
        Serial.print("[TESTE] Offset de detecção: ");
        Serial.print(springContactMotorPosRealMm, 1);
        Serial.println(" mm");
        Serial.println("[TESTE] Sistema pronto para compressão automática!");
        screenShownZeroReference = true;
    }
    
    currentState = STATE_COMPRESSION_SAMPLING;
}

// ============== AMOSTRAGEM DE COMPRESSÃO ==============
void TestMolaGrafset::executeStateCompressionSampling() {
    if (!screenShownCompressionSampling) {
        Serial.println("[TESTE] Etapa 9: Iniciando amostragem de compressão (10mm)...");
        uiManager.drawTestStatus(0.0f, DEFAULT_TEST_COMPRESSION_MM, 0.0f, 0.0f, true, false);
        uiManager.clearGraphArea();
        
        compressionStepCounter = 0;
        screenShownCompressionSampling = true;
    }
    
    if (compressionStepCounter <= 10) {
        // Check cancellation
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[TESTE] Cancelado durante amostragem.");
            finished = true;
            screenShownCompressionSampling = false;
            return;
        }
        
        float moldCompressionReadingMm = (float)compressionStepCounter;
        float motorRealTargetMm = springContactMotorPosRealMm - moldCompressionReadingMm;
        
        float maxMotorCompressionRealMm = springContactMotorPosRealMm - DEFAULT_TEST_COMPRESSION_MM;
        if (motorRealTargetMm < maxMotorCompressionRealMm) {
            motorRealTargetMm = maxMotorCompressionRealMm;
            moldCompressionReadingMm = springContactMotorPosRealMm - motorRealTargetMm;
        }
        
        Serial.print("[TESTE] Passo ");
        Serial.print(compressionStepCounter);
        Serial.print(": Leitura ");
        Serial.print(moldCompressionReadingMm, 1);
        Serial.print(" mm | Motor REAL ");
        Serial.print(motorRealTargetMm, 1);
        Serial.println(" mm");
        
        stepperManager.moveToPositionMm(motorRealTargetMm);
        delay(100);
        
        // Media de 5 leituras
        const int N = 5;
        float soma = 0.0f;
        for (int j = 0; j < N; ++j) {
            scaleManager.update();
            soma += scaleManager.getWeightKg();
            delay(20);
        }
        float avgKg = soma / (float)N;
        lastForceKg = avgKg;
        
        // Calcula K
        float currentK_kgf_mm = 0.0f;
        float currentK_N_mm = 0.0f;
        if (moldCompressionReadingMm > 0.5f) {
            currentK_kgf_mm = avgKg / moldCompressionReadingMm;
            currentK_N_mm = currentK_kgf_mm * 9.80665f;
            lastK_kgf_mm = currentK_kgf_mm;
            lastK_N_mm = currentK_N_mm;
        }
        
        Serial.print("[TESTE] Leitura: ");
        Serial.print(moldCompressionReadingMm, 1);
        Serial.print(" mm | Força: ");
        Serial.print(avgKg, 2);
        Serial.print(" kg | K: ");
        Serial.print(lastK_kgf_mm, 3);
        Serial.println(" kgf/mm");
        
        uiManager.drawTestStatus(avgKg,
                                 moldCompressionReadingMm,
                                 lastK_kgf_mm,
                                 lastK_N_mm,
                                 true,
                                 false);
        
        float xNorm = moldCompressionReadingMm / DEFAULT_TEST_COMPRESSION_MM;
        float yNorm = avgKg / GRAPH_MAX_FORCE_KG;
        if (yNorm > 1.0f) yNorm = 1.0f;
        
        uiManager.plotGraphPoint(xNorm, yNorm, (compressionStepCounter == 0));
        
        compressionStepCounter++;
    } else {
        compressionSamplingDone = true;
        currentState = STATE_RETURN_INITIAL;
        screenShownCompressionSampling = false;
    }
}

// ============== RETORNA POSIÇÃO INICIAL ==============
void TestMolaGrafset::executeStateReturnInitial() {
    if (!screenShownReturnInitial) {
        Serial.println("[TESTE] Etapa 10: Retornando motor para 30mm...");
        uiManager.drawTestStatus(lastForceKg,
                                 DEFAULT_TEST_COMPRESSION_MM,
                                 lastK_kgf_mm,
                                 lastK_N_mm,
                                 false,
                                 true);
        
        Serial.print("[TESTE] Motor movendo de ");
        Serial.print(springContactMotorPosRealMm, 1);
        Serial.print(" mm para 30 mm...");
        stepperManager.moveToPositionMm(30.0f, 133);
        Serial.println(" Pronto!");
        
        delay(500);
        screenShownReturnInitial = true;
    }
    
    currentState = STATE_SHOW_RESULTS;
}

// ============== EXIBE RESULTADOS ==============
void TestMolaGrafset::executeStateShowResults() {
    if (!screenShownShowResults) {
        Serial.println("[TESTE] Etapa 11: Teste concluido. Aguardando confirmação do usuário...");
        
        uiManager.drawText(">>> Retire a mola <<<", 20, 180, TFT_YELLOW, 1);
        uiManager.drawText("Click para continuar", 20, 200, TFT_CYAN, 1);
        
        userInteractionTimeout = millis() + 60000;  // 1 minuto
        screenShownShowResults = true;
    }
    
    if (encoderManager.wasButtonClicked()) {
        Serial.println("[TESTE] Usuário confirmou.");
        userConfirmedRemoval = true;
        
        // Exibe resumo final
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
        Serial.print("Ponto de contato (Motor REAL): ");
        Serial.print(springContactMotorPosRealMm, 1);
        Serial.println(" mm");
        Serial.print("K (kgf/mm): ");
        Serial.println(lastK_kgf_mm);
        Serial.print("K (N/mm): ");
        Serial.println(lastK_N_mm);
        
        delay(500);
        finished = true;
        screenShownShowResults = false;
    }
    
    if (millis() > userInteractionTimeout) {
        Serial.println("[TESTE] Timeout, retornando ao menu.");
        finished = true;
        screenShownShowResults = false;
    }
}

bool TestMolaGrafset::checkUserInteractionTimeout(unsigned long timeout) {
    return millis() > timeout;
}
