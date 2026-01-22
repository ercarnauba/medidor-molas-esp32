#ifndef TEST_MOLA_GRAFSET_H
#define TEST_MOLA_GRAFSET_H

#include "grafset.h"
#include <cstdint>

/**
 * @brief Grafset para teste de mola com medição de constante elástica
 * 
 * Máquina de estado com 11 etapas:
 * 0: Inicial
 * 10: Homing (busca home)
 * 20: Retorna 30mm
 * 30: Aguarda usuário posicionar mola
 * 40: Tara
 * 50: Busca ponto de contato
 * 60: Retorna à tara
 * 70: Zera referência
 * 80: Amostragem de compressão
 * 90: Retorna à posição inicial
 * 100: Exibe resultados
 */
class TestMolaGrafset : public Grafset {
public:
    enum TestState {
        STATE_READY = -1,        // Aguardando confirmação para iniciar
        STATE_INITIAL = 0,
        STATE_HOMING = 10,
        STATE_RETURN_30MM = 20,
        STATE_AWAIT_SPRING_PLACEMENT = 30,
        STATE_TARE = 40,
        STATE_FIND_SPRING_CONTACT = 50,
        STATE_RETURN_TO_TARE = 60,
        STATE_ZERO_REFERENCE = 70,
        STATE_COMPRESSION_SAMPLING = 80,
        STATE_RETURN_INITIAL = 90,
        STATE_SHOW_RESULTS = 100
    };

    TestMolaGrafset();
    virtual ~TestMolaGrafset() {}
    
    void start() override;
    void tick() override;
    void reset() override;

private:
    TestState currentState;
    
    // Variáveis de progresso entre etapas
    float motorRealPositionMm;
    float springContactMotorPosRealMm;
    float moldReadingPositionMm;
    
    float lastK_kgf_mm;
    float lastK_N_mm;
    float lastForceKg;
    
    int compressionStepCounter;
    const int MAX_COMPRESSION_STEPS = 11;  // 0 a 10mm
    
    unsigned long stateStartTime;
    unsigned long userInteractionTimeout;
    
    // Flags de estado para cada etapa
    bool screenShownAwaitSpringPlacement;
    bool screenShownFindSpringContact;
    bool screenShownReturnToTare;
    bool screenShownZeroReference;
    bool screenShownCompressionSampling;
    bool screenShownReturnInitial;
    bool screenShownShowResults;
    
    bool springReadyConfirmed;
    bool springContactDetected;
    bool taraReached;
    bool userConfirmedRemoval;
    bool compressionSamplingDone;
    
    // Flags de execução para estados (não usar static nos métodos)
    bool screenShownReady;
    bool homingExecuted;
    bool moveExecuted;

    // Baseline da balança para detectar alteração durante homing
    float homingBaselineKg = 0.0f;

    // Tempo de entrada no estado READY para gating de clique
    unsigned long readyEntryTimeMs = 0;

    // Tempo de entrada no estado AWAIT_SPRING_PLACEMENT para gating de clique
    unsigned long awaitSpringEntryTimeMs = 0;
    
    // Métodos internos para cada etapa
    void executeStateReady();
    void executeStateInitial();
    void executeStateHoming();
    void executeStateReturn30mm();
    void executeStateAwaitSpringPlacement();
    void executeStateTare();
    void executeStateFindSpringContact();
    void executeStateReturnToTare();
    void executeStateZeroReference();
    void executeStateCompressionSampling();
    void executeStateReturnInitial();
    void executeStateShowResults();
    
    // Auxiliares
    bool checkUserInteractionTimeout(unsigned long timeout);
};

#endif // TEST_MOLA_GRAFSET_H
