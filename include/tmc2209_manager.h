#ifndef TMC2209_MANAGER_H
#define TMC2209_MANAGER_H

#include <Arduino.h>
#include <TMCStepper.h>

/**
 * @brief Gerenciador do driver TMC2209 com suporte a StallGuard
 * 
 * Esta classe implementa comunicação UART com o TMC2209 e funções avançadas:
 * - Configuração de corrente (RMS, hold)
 * - Configuração de microsteps
 * - StallGuard para detecção de colisão/travamento
 * - Recuo automático quando detectar stall
 * - Monitoramento contínuo de stall durante movimentos
 */
class TMC2209Manager {
public:
    /**
     * @brief Inicializa comunicação UART e configura driver TMC2209
     * @return true se inicialização bem sucedida, false caso contrário
     */
    bool begin();

    /**
     * @brief Configura corrente do motor (RMS e hold)
     * @param currentRMS Corrente RMS em mA (ex: 800 para NEMA11)
     * @param currentHold Corrente em hold/idle em mA (ex: 400, 50% da RMS)
     */
    void setCurrent(uint16_t currentRMS, uint16_t currentHold);

    /**
     * @brief Configura microsteps do driver
     * @param microsteps Valor de microsteps (1, 2, 4, 8, 16, 32, 64, 128, 256)
     */
    void setMicrosteps(uint16_t microsteps);

    /**
     * @brief Configura threshold do StallGuard
     * @param threshold Valor 0-255 (quanto MAIOR, MENOS sensível)
     * Valores recomendados: 5-20 para detecção de colisão
     */
    void setStallGuardThreshold(uint8_t threshold);

    /**
     * @brief Verifica se houve detecção de stall (motor travado)
     * @return true se stall detectado, false caso contrário
     */
    bool isStallDetected();

    /**
     * @brief Limpa flag de stall após tratamento
     */
    void clearStall();

    /**
     * @brief Verifica se último stall ainda precisa ser tratado
     * @return true se há stall pendente (aguardando recuo)
     */
    bool hasUntreatedStall() const;

    /**
     * @brief Marca que o stall foi tratado (recuo executado)
     */
    void markStallTreated();

    /**
     * @brief Retorna leitura do valor StallGuard atual (para debug)
     * Valores típicos: 0-511 (quanto menor, mais próximo do stall)
     */
    int getStallGuardValue();

    /**
     * @brief Verifica se comunicação UART está funcionando
     * @return true se driver responde corretamente
     */
    bool isCommunicationOK();

    /**
     * @brief Habilita/desabilita modo StealthChop (silencioso)
     * @param enable true para StealthChop, false para SpreadCycle (mais torque)
     */
    void enableStealthChop(bool enable);

    /**
     * @brief Obtém status de diagnóstico do driver
     * @return String com informações de status (corrente, temperatura, erros)
     */
    String getDiagnostics();

private:
    TMC2209Stepper* _driver = nullptr;
    bool _stallDetectedFlag = false;
    bool _stallUntreated = false;
    unsigned long _lastStallTime = 0;
    
    // Hardware Serial para comunicação UART
    HardwareSerial* _serial = nullptr;
};

extern TMC2209Manager tmc2209Manager;

#endif
