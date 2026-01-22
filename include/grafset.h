#ifndef GRAFSET_H
#define GRAFSET_H

/**
 * @brief Classe base abstrata para implementar máquinas de estado (Grafsets)
 * 
 * Cada Grafset representa um modo/teste/operação do sistema.
 * - possui um conjunto de estados internos
 * - implementa tick() que avança a máquina de estado
 * - isFinished() indica quando a operação terminou
 * - reset() prepara para a próxima execução
 */
class Grafset {
public:
    Grafset() : finished(false) {}
    virtual ~Grafset() {}
    
    /**
     * @brief Avança a máquina de estado por um ciclo
     * Chamado repetidamente no loop principal
     */
    virtual void tick() = 0;
    
    /**
     * @brief Inicia/reinicia o Grafset
     * Chamado quando o usuário seleciona esta operação
     */
    virtual void start() = 0;
    
    /**
     * @brief Indica se a operação foi concluída
     */
    bool isFinished() const { return finished; }
    
    /**
     * @brief Reseta o estado interno para pronto executar novamente
     */
    virtual void reset() {
        finished = false;
    }

protected:
    bool finished;  // Flag indicando conclusão da operação
};

#endif // GRAFSET_H
