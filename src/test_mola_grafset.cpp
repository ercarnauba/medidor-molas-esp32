#include "test_mola_grafset.h"
#include "scale_manager.h"
#include "stepper_manager.h"
#include "encoder_manager.h"
#include "ui_manager.h"
#include "config.h"

// Baseline global ao arquivo para monitorar homing
static float g_homingBaselineKg = 0.0f;
static unsigned long g_lastHomingMonitorMs = 0;

// Callback de monitoramento para abortar homing quando houver alteração na balança
static bool homingWeightMonitor(void* ctx) {
    // Throttle: só checa em intervalos para evitar jitter
    unsigned long now = millis();
    if ((now - g_lastHomingMonitorMs) < HOMING_MONITOR_INTERVAL_MS) {
        return false;
    }
    g_lastHomingMonitorMs = now;

    // Leitura rápida, não bloqueante (se não pronto, mantém último)
    float kg = scaleManager.peekWeightKgFast();
    float delta = kg - g_homingBaselineKg;
    if (delta < 0.0f) delta = -delta;
    if (delta >= HOMING_WEIGHT_DELTA_KG) {
        // Alteração significativa detectada durante homing: abortar
        return true;
    }
    return false;
}

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
      compressionSamplingDone(false),
      screenShownReady(false),
      homingExecuted(false),
      moveExecuted(false)
{
}

void TestMolaGrafset::start() {
    Serial.println("[TESTE] Teste de mola selecionado. Selecionando curso...");
    currentState = STATE_SELECT_COURSE;
    finished = false;
    
    // Reset de variáveis
    motorRealPositionMm = 0.0f;
    springContactMotorPosRealMm = 0.0f;
    moldReadingPositionMm = 0.0f;
    lastK_kgf_mm = 0.0f;
    lastK_N_mm = 0.0f;
    lastForceKg = 0.0f;
    compressionStepCounter = 0;
    sampleCount = 0;  // Reinicia buffer de amostras para novo teste
    selectedCourseMm = DEFAULT_TEST_COMPRESSION_MM;
    
    // Reset de flags
    screenShownAwaitSpringPlacement = false;
    screenShownFindSpringContact = false;
    screenShownReturnToTare = false;
    screenShownZeroReference = false;
    screenShownCompressionSampling = false;
    screenShownReturnInitial = false;
    screenShownShowResults = false;
    
    screenShownReady = false;
    homingExecuted = false;
    moveExecuted = false;
    readyEntryTimeMs = 0;
    
    springReadyConfirmed = false;
    springContactDetected = false;
    taraReached = false;
    userConfirmedRemoval = false;
    compressionSamplingDone = false;
    
    stateStartTime = millis();
    
    // NÃO desenhar tela aqui - será desenhada no executeStateReady()
}

void TestMolaGrafset::tick() {
    if (finished) return;
    
    // Atualizar sensores (NÃO chamar encoderManager.update aqui - é no loop principal)
    scaleManager.update();
    
    switch (currentState) {
        case STATE_SELECT_COURSE:
            executeStateSelectCourse();
            break;
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

// ============== ETAPA SELEÇÃO DE CURSO ==============
void TestMolaGrafset::executeStateSelectCourse() {
    static int courseIndex = 0;  // Índice da opção selecionada
    static bool screenShown = false;
    static long lastEncPos = 0;
    static unsigned long entryTimeMs = 0;

    // Desenha tela uma única vez
    if (!screenShown) {
        Serial.println("[TESTE] Selecionando curso de leitura...");
        
        uiManager.clearScreen();
        uiManager.drawText("=== Teste de Mola ===", 50, 20, TFT_YELLOW, 3);
        uiManager.drawText("Curso de leitura:", 90, 80, TFT_WHITE, 3);
        
        // Mostra opções disponíveis
        for (int i = 0; i < SPRING_TEST_COURSES_COUNT; i++) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0f mm", SPRING_TEST_COURSES[i]);
            uint16_t color = (i == courseIndex) ? TFT_CYAN : TFT_WHITE;
            uiManager.drawText(buf, 180, 140 + i * 50, color, 2);
            
            // Desenha símbolo para todos os itens
            if (i == courseIndex) {
                uiManager.drawText(">", 150, 140 + i * 50, TFT_CYAN, 2);
            } else {
                uiManager.drawText(" ", 150, 140 + i * 50, TFT_BLACK, 2);
            }
        }
        
        uiManager.drawText("Click = confirmar", 120, 280, TFT_GREEN, 2);
        uiManager.drawText("Long press = cancelar", 80, 305, TFT_RED, 2);
        
        lastEncPos = encoderManager.getPosition();
        encoderManager.wasButtonClicked();  // Consome cliques pendentes
        encoderManager.wasButtonLongPressed();
        
        screenShown = true;
        entryTimeMs = millis();
        return;
    }

    // Gating: aguarda período após mostrar tela
    if (millis() - entryTimeMs < 500) {
        return;
    }

    // Navegação com encoder
    long encPos = encoderManager.getPosition();
    long delta = encPos - lastEncPos;
    
    if (delta != 0) {
        lastEncPos = encPos;
        
        if (delta > 0) {
            courseIndex++;
            if (courseIndex >= SPRING_TEST_COURSES_COUNT) courseIndex = 0;
        } else {
            courseIndex--;
            if (courseIndex < 0) courseIndex = SPRING_TEST_COURSES_COUNT - 1;
        }
        
        // Redesenha todas as opções e símbolos
        for (int i = 0; i < SPRING_TEST_COURSES_COUNT; i++) {
            // Limpa completamente a área do símbolo (20x16 pixels para tamanho 2)
            uiManager.fillRect(150, 140 + i * 50, 20, 16, TFT_BLACK);
            
            // Desenha o texto da opção
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0f mm     ", SPRING_TEST_COURSES[i]);
            uint16_t color = (i == courseIndex) ? TFT_CYAN : TFT_WHITE;
            uiManager.drawText(buf, 180, 140 + i * 50, color, 2);
            
            // Desenha símbolo apenas no item selecionado
            if (i == courseIndex) {
                uiManager.drawText(">", 150, 140 + i * 50, TFT_CYAN, 2);
            }
        }
    }

    // Confirma seleção
    if (encoderManager.wasButtonClicked()) {
        selectedCourseMm = SPRING_TEST_COURSES[courseIndex];
        Serial.print("[TESTE] Curso selecionado: ");
        Serial.print(selectedCourseMm);
        Serial.println(" mm");
        
        screenShown = false;
        currentState = STATE_READY;
        return;
    }

    // Cancela teste
    if (encoderManager.wasButtonLongPressed()) {
        Serial.println("[TESTE] Seleção de curso cancelada.");
        finished = true;
        screenShown = false;
        return;
    }
}

// ============== ETAPA PRONTO (READY) ==============
void TestMolaGrafset::executeStateReady() {
    if (!screenShownReady) {
        Serial.println("[TESTE] Aguardando confirmação do usuário para iniciar...");
        
        // Desenha tela de confirmação
        uiManager.clearScreen();
        uiManager.drawText("=== Teste de Mola ===", 50, 40, TFT_YELLOW, 3);
        
        char buf[32];
        snprintf(buf, sizeof(buf), "Curso: %.0f mm", selectedCourseMm);
        uiManager.drawText(buf, 90, 120, TFT_CYAN, 3);
        
        uiManager.drawText("Click para iniciar", 80, 190, TFT_GREEN, 3);
        uiManager.drawText("", 10, 230, TFT_WHITE, 2);
        uiManager.drawText("Long press para cancelar", 60, 280, TFT_RED, 2);
        
        // IMPORTANTE: Limpar qualquer click pendente do teste anterior
        // Isso evita que um click "guardado" inicie o novo teste automaticamente
        encoderManager.wasButtonClicked();  // Consome qualquer click pendente
        encoderManager.wasButtonLongPressed(); // Consome long press pendente também
        
        screenShownReady = true;
        readyEntryTimeMs = millis();
        return;  // Aguarda próximo tick para processar entrada
    }
    
    // Gating: aguarda um período após mostrar a tela para aceitar cliques (800ms para segurança extra)
    if (millis() - readyEntryTimeMs < 800) {
        return;
    }

    // Verifica clique para iniciar
    if (encoderManager.wasButtonClicked()) {
        Serial.println("[TESTE] Usuário confirmou. Iniciando teste...");
        currentState = STATE_INITIAL;
        screenShownReady = false;
        return;
    }
    
    // Verifica long press para cancelar
    if (encoderManager.wasButtonLongPressed()) {
        Serial.println("[TESTE] Teste cancelado pelo usuário.");
        finished = true;
        screenShownReady = false;
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
    if (!homingExecuted) {
        // Limpa tela e mostra mensagem de homing
        uiManager.clearScreen();
        uiManager.drawText("=== Homing ===", 120, 80, TFT_CYAN, 3);
        uiManager.drawText("Buscando HOME...", 70, 150, TFT_YELLOW, 3);
        uiManager.drawText("Aguarde...", 170, 220, TFT_WHITE, 2);
        
        // Captura baseline da balança para detectar alterações durante homing
        scaleManager.update();
        homingBaselineKg = scaleManager.getWeightKg();
        g_homingBaselineKg = homingBaselineKg;

        // IMPORTANTE: Usar limite MUITO grande (250mm = 400000 passos a 1600 spm)
        // O motor será parado APENAS pelo micro switch, não por limite de movimento
        long maxSteps = (long)(250.0f * stepperManager.getStepsPerMm());
        
        // Homing com monitoramento de alteração de peso na balança
        stepperManager.homeToEndstopWithMonitor(maxSteps, 133, homingWeightMonitor, this);
        homingExecuted = true;
        
        delay(200);  // Aguardar estabilização
        
        bool homingSuccess = stepperManager.wasLastHomingSuccessful();
        
        if (!homingSuccess) {
            delay(100);
            scaleManager.update();
            float currentWeight = scaleManager.getWeightKg();
            float delta = currentWeight - homingBaselineKg;
            if (delta < 0.0f) delta = -delta;

            if (delta >= HOMING_WEIGHT_DELTA_KG || currentWeight >= SPRING_CONTACT_FORCE_KG) {
                // ALARME: Alteração de peso detectada durante homing → possível objeto/mola colocada
                Serial.println("[ALARME] *** ALARME DE HOMING: Objeto detectado durante homing ***");
                uiManager.clearScreen();
                uiManager.drawText("=== ALARME ===", 105, 40, TFT_RED, 3);
                uiManager.drawText("Objeto detectado", 60, 110, TFT_RED, 3);
                uiManager.drawText("durante HOMING!", 70, 155, TFT_RED, 3);
                uiManager.drawText("", 10, 200, TFT_WHITE, 2);
                uiManager.drawText("Remova a mola/objeto", 90, 220, TFT_YELLOW, 2);
                uiManager.drawText("e confirme para tentar", 80, 245, TFT_YELLOW, 2);
                uiManager.drawText("novamente.", 165, 270, TFT_YELLOW, 2);

                delay(3000);
            } else {
                // Falha sem alteração de peso – provável problema mecânico ou switch
                uiManager.drawTestStatus(0.0f, selectedCourseMm, 0.0f, 0.0f, false, false);
                delay(3000);
            }
            
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
        uiManager.drawText("=== Posicionar Mola ===", 15, 30, TFT_YELLOW, 3);
        uiManager.drawText("Motor em 30 mm", 130, 100, TFT_WHITE, 2);
        uiManager.drawText("Coloque a mola", 130, 130, TFT_WHITE, 2);
        uiManager.drawText("entre as plataformas", 85, 155, TFT_WHITE, 2);
        uiManager.drawText("", 10, 190, TFT_WHITE, 2);
        uiManager.drawText("Click encoder", 100, 220, TFT_CYAN, 3);
        uiManager.drawText("para confirmar", 90, 265, TFT_CYAN, 3);
        
        userInteractionTimeout = millis() + 120000;  // 2 minutos
        screenShownAwaitSpringPlacement = true;
        awaitSpringEntryTimeMs = millis();  // Marca entrada no estado
        
        // Consome cliques pendentes no primeiro frame
        encoderManager.wasButtonClicked();
        encoderManager.wasButtonLongPressed();
    }
    
    // Guard: ignora cliques nos primeiros 500ms após entrada no estado
    if ((millis() - awaitSpringEntryTimeMs) < 500) {
        return;
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
        
        // Limpa tela e mostra mensagem de busca
        uiManager.clearScreen();
        uiManager.drawText("=== Busca Mola ===", 75, 80, TFT_CYAN, 3);
        uiManager.drawText("Procurando...", 105, 150, TFT_YELLOW, 3);
        uiManager.drawText("Aguarde...", 170, 220, TFT_WHITE, 2);
        
        screenShownFindSpringContact = true;
    }
    
    static int chunkSteps = 0;
    static float maxMotorSearchPosRealMm = 5.0f;
    static bool searchStarted = false;
    static bool continuousMovementDone = false;
    
    if (!searchStarted) {
        chunkSteps = (int)(0.25f * stepperManager.getStepsPerMm());
        if (chunkSteps < 1) chunkSteps = 1;
        searchStarted = true;
        continuousMovementDone = false;
        
        // Fase 1: Movimento contínuo de 10mm (uma única vez)
        long continuousSteps = (int)(10.0f * stepperManager.getStepsPerMm());  // 10mm contínuo
        stepperManager.moveSteps(continuousSteps, STEPPER_DIR_BACKWARD, 130);  // 130us (mais seguro)
        motorRealPositionMm = stepperManager.getPositionMm();
        
        Serial.print("[TESTE] Movimento contínuo de 10mm concluído. Posição: ");
        Serial.print(motorRealPositionMm, 1);
        Serial.println(" mm");
        
        // Verifica se mola foi detectada durante o movimento contínuo
        float currentForceKg = scaleManager.getWeightKg();
        if (currentForceKg >= SPRING_CONTACT_FORCE_KG) {
            springContactDetected = true;
            springContactMotorPosRealMm = motorRealPositionMm;
            Serial.print("[TESTE] MOLA DETECTADA (durante contínuo)! Forca: ");
            Serial.print(currentForceKg, 3);
            Serial.print(" kg em posicao: ");
            Serial.print(springContactMotorPosRealMm, 1);
            Serial.println(" mm");
            
            currentState = STATE_RETURN_TO_TARE;
            searchStarted = false;
            screenShownFindSpringContact = false;
            continuousMovementDone = false;
            return;
        }
        
        continuousMovementDone = true;
    }
    
    if (motorRealPositionMm > maxMotorSearchPosRealMm) {
        // Fase 2: Movimento em pulsos (após 10mm contínuos)
        
        // Check for user cancellation
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[TESTE] Cancelado pelo usuario durante busca.");
            finished = true;
            searchStarted = false;
            screenShownFindSpringContact = false;
            continuousMovementDone = false;
            return;
        }
        
        // Move down em pulsos
        stepperManager.moveSteps(chunkSteps, STEPPER_DIR_BACKWARD, 100);
        motorRealPositionMm = stepperManager.getPositionMm();
        
        float currentForceKg = scaleManager.getWeightKg();
        
        if (currentForceKg >= SPRING_CONTACT_FORCE_KG) {
            springContactDetected = true;
            springContactMotorPosRealMm = motorRealPositionMm;
            Serial.print("[TESTE] MOLA DETECTADA (fase pulsos)! Forca: ");
            Serial.print(currentForceKg, 3);
            Serial.print(" kg em posicao: ");
            Serial.print(springContactMotorPosRealMm, 1);
            Serial.println(" mm");
            
            currentState = STATE_RETURN_TO_TARE;
            searchStarted = false;
            screenShownFindSpringContact = false;
            continuousMovementDone = false;
            return;
        }
    } else if (!springContactDetected) {
        Serial.println("[TESTE] ERRO: Não foi possível detectar o ponto de contato!");
        
        // Exibir alarme no TFT
        uiManager.clearScreen();
        uiManager.drawText("=== ALARME ===", 105, 40, TFT_RED, 3);
        uiManager.drawText("", 10, 100, TFT_WHITE, 2);
        uiManager.drawText("Mola NAO DETECTADA!", 30, 120, TFT_RED, 3);
        uiManager.drawText("", 10, 200, TFT_WHITE, 2);
        uiManager.drawText("Verificar:", 170, 200, TFT_YELLOW, 2);
        uiManager.drawText("- Mola posicionada?", 100, 225, TFT_YELLOW, 2);
        uiManager.drawText("- Balanca calibrada?", 90, 250, TFT_YELLOW, 2);
        uiManager.drawText("", 10, 280, TFT_WHITE, 2);
        uiManager.drawText("Click para tentar novamente", 45, 295, TFT_CYAN, 2);
        
        Serial.println("[TESTE] Aguardando confirmação para tentar novamente...");
        userInteractionTimeout = millis() + 60000;  // 1 minuto
        searchStarted = false;
        continuousMovementDone = false;
        
        // Aguardar clique do usuário
        while (millis() < userInteractionTimeout) {
            encoderManager.update();
            scaleManager.update();
            
            if (encoderManager.wasButtonClicked()) {
                Serial.println("[TESTE] Usuário optou por tentar novamente.");
                // Reseta e volta ao HOMING para garantir posição segura antes do novo ciclo
                currentState = STATE_HOMING;
                homingExecuted = false;
                moveExecuted = false;
                screenShownAwaitSpringPlacement = false;
                searchStarted = false;
                screenShownFindSpringContact = false;
                springContactDetected = false;
                return;
            }
        }
        
        // Timeout - voltar ao menu
        Serial.println("[TESTE] Timeout no alarme de mola não detectada.");
        finished = true;
        searchStarted = false;
        screenShownFindSpringContact = false;
    }
}

// ============== RETORNA À TARA ==============
void TestMolaGrafset::executeStateReturnToTare() {
    if (!screenShownReturnToTare) {
        Serial.println("[TESTE] Etapa 7: Recua ate nao tocar...");
        // Não limpa tela - mantém gráfico visível
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
        
        stepperManager.moveSteps(chunkSteps, STEPPER_DIR_FORWARD, 100);
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
        // Define a referência de zero na posição aliviada (pós-tara),
        // evitando comprimir ~2 mm antes da primeira leitura
        motorRealPositionMm = stepperManager.getPositionMm();
        springContactMotorPosRealMm = motorRealPositionMm;

        Serial.println("[TESTE] Etapa 8: Zerando referencia de posicao da mola...");
        Serial.print("[TESTE] Zero fixado em posicao aliviada: ");
        Serial.print(springContactMotorPosRealMm, 1);
        Serial.println(" mm");
        Serial.println("[TESTE] Sistema pronto para compressao automatica (0 mm = contato aliviado)!");
        screenShownZeroReference = true;
    }
    
    currentState = STATE_COMPRESSION_SAMPLING;
}

// ============== AMOSTRAGEM DE COMPRESSÃO ==============
void TestMolaGrafset::executeStateCompressionSampling() {
    if (!screenShownCompressionSampling) {
        Serial.println("[TESTE] Etapa 9: Iniciando amostragem de compressão (10mm)...");
        uiManager.clearScreen();
        uiManager.drawTestStatus(0.0f, selectedCourseMm, 0.0f, 0.0f, true, false);
        uiManager.clearGraphArea();
        
        compressionStepCounter = 0;
        screenShownCompressionSampling = true;
    }
    
    if (compressionStepCounter <= (int)selectedCourseMm) {
        // Check cancellation
        if (encoderManager.wasButtonLongPressed()) {
            Serial.println("[TESTE] Cancelado durante amostragem.");
            finished = true;
            screenShownCompressionSampling = false;
            return;
        }
        
        float moldCompressionReadingMm = (float)compressionStepCounter;
        float motorRealTargetMm = springContactMotorPosRealMm - moldCompressionReadingMm;
        
        float maxMotorCompressionRealMm = springContactMotorPosRealMm - selectedCourseMm;
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
        // Armazena amostra para regressão
        if (sampleCount < MAX_SAMPLES) {
            sampleX_mm[sampleCount] = moldCompressionReadingMm;
            sampleF_kg[sampleCount] = avgKg;
            sampleCount++;
        }
        
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
        
        float xNorm = moldCompressionReadingMm / selectedCourseMm;
        float yNorm = avgKg / GRAPH_MAX_FORCE_KG;
        if (yNorm > 1.0f) yNorm = 1.0f;
        
        uiManager.plotGraphPoint(xNorm, yNorm, (compressionStepCounter == 0));
        
            // Plota curva amarela (K em N/mm)
            float kNorm = lastK_N_mm / 20.0f;  // Normaliza assumindo K máximo ~20 N/mm
            uiManager.plotGraphPointYellow(xNorm, kNorm, (compressionStepCounter == 0));
        
        // Desenha valor de K na lista lateral
        if (compressionStepCounter > 0) {  // Pula o passo 0 (sem compressão)
            uiManager.drawKValueAtStep(compressionStepCounter, lastK_kgf_mm, lastK_N_mm);
        }
        
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
        // Ao finalizar a amostragem, calcula K por regressão linear na faixa útil (2 a 8mm)
        float r2 = 0.0f;
        float upper = selectedCourseMm < 8.0f ? selectedCourseMm : 8.0f;
        float k_kgf_mm = computeSpringRateRegression(2.0f, upper, &r2);
        if (k_kgf_mm > 0.0f) {
            lastK_kgf_mm = k_kgf_mm;
            lastK_N_mm = lastK_kgf_mm * 9.80665f;
            lastR2 = r2;
        }

        Serial.println("[TESTE] Etapa 10: Retornando motor para 30mm...");
        uiManager.drawTestStatus(lastForceKg,
                                 selectedCourseMm,
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

// Regressão linear F = a + k*x, usando amostras coletadas
float TestMolaGrafset::computeSpringRateRegression(float lowMm, float highMm, float* outR2) {
    // Acumula apenas pontos no intervalo [lowMm, highMm]
    float sumX = 0.0f, sumY = 0.0f, sumXX = 0.0f, sumXY = 0.0f;
    int n = 0;
    float minX = lowMm, maxX = highMm;
    for (int i = 0; i < sampleCount; ++i) {
        float x = sampleX_mm[i];
        float y = sampleF_kg[i];
        if (x >= minX && x <= maxX) {
            sumX += x;
            sumY += y;
            sumXX += x * x;
            sumXY += x * y;
            n++;
        }
    }
    if (n < 2) {
        if (outR2) *outR2 = 0.0f;
        return 0.0f;
    }
    float xMean = sumX / n;
    float yMean = sumY / n;
    float denom = (sumXX - n * xMean * xMean);
    if (denom == 0.0f) {
        if (outR2) *outR2 = 0.0f;
        return 0.0f;
    }
    float k = (sumXY - n * xMean * yMean) / denom; // kgf/mm
    // Intercept a = yMean - k * xMean (não usado)

    // R^2
    if (outR2) {
        float ssTot = 0.0f;
        float ssRes = 0.0f;
        for (int i = 0; i < sampleCount; ++i) {
            float x = sampleX_mm[i];
            float y = sampleF_kg[i];
            if (x >= minX && x <= maxX) {
                float yHat = (yMean - k * xMean) + k * x;
                ssTot += (y - yMean) * (y - yMean);
                ssRes += (y - yHat) * (y - yHat);
            }
        }
        *outR2 = (ssTot > 0.0f) ? (1.0f - ssRes / ssTot) : 0.0f;
    }
    return k;
}

// ============== EXIBE RESULTADOS ==============
void TestMolaGrafset::executeStateShowResults() {
    if (!screenShownShowResults) {
        Serial.println("[TESTE] Etapa 11: Teste concluido. Aguardando confirmação do usuário...");
        
        const char* removeMsg = ">>> Retire a mola <<<";
        int removeMsgWidth = (int)strlen(removeMsg) * 6 * 2;  // fonte size=2, 6px base
        int removeMsgX = 320 - removeMsgWidth;
        if (removeMsgX < 0) removeMsgX = 0;

        uiManager.drawText(removeMsg, removeMsgX, 285, TFT_WHITE, 2);
        uiManager.drawText("Click para continuar", 0, 305, TFT_CYAN, 2);
        
        userInteractionTimeout = millis() + 60000;  // 1 minuto
        screenShownShowResults = true;
    }
    
    // Pisca a mensagem "Retire a mola"
    static unsigned long lastBlinkToggle = 0;
    static bool blinkOn = true;
    static int cachedMsgX = -1;
    static int cachedMsgW = 0;
    if (cachedMsgX < 0) {
        const char* removeMsg = ">>> Retire a mola <<<";
        cachedMsgW = (int)strlen(removeMsg) * 6 * 2;
        cachedMsgX = 320 - cachedMsgW;
        if (cachedMsgX < 0) cachedMsgX = 0;
    }
    if (millis() - lastBlinkToggle > 500) {
        lastBlinkToggle = millis();
        blinkOn = !blinkOn;
        if (blinkOn) {
            uiManager.drawText(">>> Retire a mola <<<", cachedMsgX, 285, TFT_WHITE, 2);
        } else {
            // Limpa a área onde o texto aparece
            uiManager.fillRect(cachedMsgX, 282, cachedMsgW + 4, 28, TFT_BLACK);
        }
    }

    if (encoderManager.wasButtonClicked()) {
        Serial.println("[TESTE] Usuário confirmou.");
        userConfirmedRemoval = true;
        
        // Exibe resumo final
        uiManager.clearScreen();
        uiManager.drawText("=== Teste Concluido ===", 20, 30, TFT_GREEN, 3);
        uiManager.drawText("", 10, 80, TFT_WHITE, 2);
        
        char kDisplay[64];
        snprintf(kDisplay, sizeof(kDisplay), "K: %.3f kgf/mm", lastK_kgf_mm);
        uiManager.drawText(kDisplay, 60, 110, TFT_CYAN, 3);
        
        char kNewtons[64];
        snprintf(kNewtons, sizeof(kNewtons), "K: %.3f N/mm", lastK_N_mm);
        uiManager.drawText(kNewtons, 100, 160, TFT_CYAN, 2);

        // Mostra R^2 para avaliar linearidade da faixa usada
        char r2Display[64];
        snprintf(r2Display, sizeof(r2Display), "R^2: %.3f", lastR2);
        uiManager.drawText(r2Display, 100, 185, TFT_YELLOW, 2);
        
        char forceDisplay[64];
        snprintf(forceDisplay, sizeof(forceDisplay), "Forca max: %.2f kg", lastForceKg);
        uiManager.drawText(forceDisplay, 90, 200, TFT_WHITE, 2);
        
        uiManager.drawText("", 10, 200, TFT_WHITE, 2);
        uiManager.drawText("Click no botao para menu", 100, 285, TFT_YELLOW, 2);
        
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
















