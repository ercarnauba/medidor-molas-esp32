# Changelog - Medidor_Molas_RC

## [2.0.1] - 2026-01-15 (Hardware Atualizado)

### 🔧 **Atualização de Hardware Específico**

#### Informações Confirmadas
- **Motor de Passo**: NEMA11 (200 passos/volta, compacto 28×28mm) ✅
- **Corrente Motor**: 0.8-1.0A (torque limitado ~0.2-0.3 Nm) ✅
- **Driver Stepper**: TMC2209 (SilentStepStick) ✅
- **Microsteps padrão**: 16x (configurável M0/M1) ✅
- **Sistema Mecânico**: Trilho deslizante pitch 1mm/volta ✅
- **STEPPER_STEPS_PER_MM**: 3200 (valor correto confirmado) ✅
- **Tensão uC**: 3.3V (ESP32) ✅
- **Tensão Motor**: 12V ✅

#### Atualizações em config.h
- STEPPER_STEPS_PER_MM = 3200.0f ✅ CONFIRMADO para trilho pitch 1mm
- Adicionados comentários sobre TMC2209
- Adicionada documentação sobre trilho deslizante

#### Documentação Atualizada
- HARDWARE_SETUP.md: Substituído A4988/DRV8825 por TMC2209
- HARDWARE_SETUP.md: NEMA11 especificado (motor compacto)
- HARDWARE_SETUP.md: Trilho deslizante pitch 1mm documentado ✅
- HARDWARE_SETUP.md: Tabela de configuração de microsteps
- HARDWARE_SETUP.md: Limite de corrente 0.8-1.0A para NEMA11
- HARDWARE_SETUP.md: Aviso sobre torque limitado (ideal para molas leves ~0-2 kgf)
- README.md: Hardware específico com tabela detalhada
- HARDWARE_REFERENCE.md: Cálculo confirmado para trilho pitch 1mm ✅
~~Calcular STEPPER_STEPS_PER_MM~~** ✅ JÁ CONFIRMADO:
   - Trilho deslizante: pitch 1mm/volta
   - Fórmula: (200 × 16) / 1 = 3200
   - Valor em config.h está correto!
   - Medir deslocamento real do parafuso/correia por volta
   - Fórmula: (200 × microsteps) / deslocamento_mm
   - Exemplo: (200 × 16) / 1mm = 3200

2. **Configurar TMC2209 M0/M1**:
   - HIGH + HIGH = 16x microsteps (recomendado)
   - Verificar se os pinos estão conectados ou flutuantes

3. **Validar Tensão 12V**:
   - Usar fonte de 12V estabilizada
   - Verificar pólo negativo comum com ESP32

4. **Limitar Corrente TMC2209**0.8-1.0A para NEMA11 (não exceder!)
   - Evitar sobreaquecimento do motor
   
5. **Validar Torque Adequado**:
   - NEMA11 tem torque limitado (~0.2-0.3 Nm)
   - Adequado para molas leves até ~2 kgf
   - Para molas mais rígidas, considerar upgrade para NEMA17ara NEMA28
   - Evitar sobreaquecimento do motor

---

## [2.0] - 2026-01-15 (Melhorias Críticas)

### 🔧 **Correções Críticas**

#### Encoder Manager
- **CRÍTICO**: Removido `millis()` de ISR (não-seguro no ESP32)
- ✅ Debounce movido para `update()` (contexto principal)
- ✅ ISRs agora apenas definem flags, sem lógica pesada

#### Scale Manager
- **CRÍTICO**: Adicionada validação de `EEPROM.commit()`
- ✅ Logs de erro quando falha em salvar calibração
- ✅ Validação de bounds para fator de calibração
- ✅ Logs informativos em calibração bem-sucedida
- ✅ Tratamento de erro quando HX711 não responde

#### Stepper Manager
- **CRÍTICO**: Adicionada detecção de falha em homing
- ✅ Nova função `wasLastHomingSuccessful()` para verificar status
- ✅ Timeout mais robusto (30s, configurável)
- ✅ Logs detalhados de erro/sucesso
- ✅ Proteção contra movimento além do curso máximo
- ✅ Proteção melhorada em `moveSteps()`

#### Main Loop
- ✅ Adicionada verificação de sucesso de homing antes de teste
- ✅ Aborta teste com mensagem de erro se homing falhar

### 📚 **Documentação Adicionada**

#### `HARDWARE_SETUP.md` (NOVO)
- Componentes necessários com especificações
- Esquema de pinos e conexões
- Cálculo de `STEPPER_STEPS_PER_MM`
- Ajustes críticos pré-teste
- Guia de troubleshooting por componente
- Notas de segurança

#### `TESTING_GUIDE.md` (NOVO)
- Testes básicos de componentes
- Validação integrada com montagem mecânica
- Logs esperados para cada teste
- Checklist de diagnóstico
- Verificações de precisão e linearidade
- Performance e timing

#### `CODE_REVIEW.md` (ATUALIZADO)
- Adicionada seção "Correções Aplicadas (v2.0)"
- Resumo de melhorias antes/depois
- Próximas melhorias sugeridas

### ⚙️ **Melhorias em config.h**

Adicionadas constantes de segurança:
```cpp
constexpr unsigned long STEPPER_HOME_TIMEOUT_MS = 30000;  // 30s (era hardcoded 20s)
constexpr uint16_t STEPPER_DEFAULT_DELAY_US = 800;
constexpr int STEPPER_HOME_MAX_RETRIES = 1;
constexpr unsigned long SCALE_READ_TIMEOUT_MS = 5000;
```

### 📊 **Matriz de Melhorias**

| Componente | Antes | Depois | Status |
|---|---|---|---|
| ISR Encoder | `millis()` em ISR | Flag-only ISR + debounce em main | ✅ Seguro |
| EEPROM | Sem validação | Validação + logs | ✅ Robusto |
| Homing | Timeout estático 20s | Timeout 30s + detecção falha | ✅ Diagnóstico |
| Movimento | Sem logs | Logs de limite/endstop | ✅ Debug |
| Calibração | Retorna silenciosamente | Validação + logs de erro | ✅ Seguro |
| Hardware Doc | Nenhuma | Completa | ✅ Novo |
| Testing | Nenhuma | Completa | ✅ Novo |

### 🧪 **Validação**

- ✅ Sem erros de compilação
- ✅ Compatível com ESP32
- ✅ Pronto para deployment

### 📝 **Notas**

- STEPPER_HOME_DIR_INT continua como configuração crítica (0 ou 1)
- Revisar STEPPER_STEPS_PER_MM antes de teste físico
- Consultar HARDWARE_SETUP.md para montagem correta
- Usar TESTING_GUIDE.md para validação completa

---

## [1.0] - 2025-12-XX (Inicial)

### ✨ **Recursos**
- Menu interativo com encoder
- Teste de mola com gráfico em tempo real
- Calibração automática da célula de carga (HX711)
- Homing automático com motor de passo
- Display TFT
- Armazenamento em EEPROM

### ⚠️ **Conhecidos**
- ISR com `millis()` (não-seguro)
- Sem validação de EEPROM.commit()
- Homing sem detecção de falha
- Documentação limitada
