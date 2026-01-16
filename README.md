# 📊 Medidor de Molas RC - Versão 2.0 (Melhorado)

**Data**: 15 de janeiro de 2026  
**Status**: ✅ Pronto para deployment  
**Compilação**: ✅ Sem erros

---

## 📋 Sumário Executivo

Este é um **medidor microcontrolado de constantes de mola (K)** baseado em ESP32, que automatiza testes de molas com:
- Célula de carga digital (HX711)
- Motor de passo com homing automático
- Display TFT interativo
- Calibração automática
- Armazenamento de calibração em EEPROM

### Melhorias na v2.0
- ✅ **Correção crítica**: ISR segura (sem `millis()`)
- ✅ **Validação robusta**: EEPROM, calibração, homing
- ✅ **Documentação completa**: Hardware + testes
- ✅ **Detecção de falhas**: Timeout e logs detalhados

---

## 🚀 Quick Start

### 1. Configuração de Hardware
Seguir [HARDWARE_SETUP.md](HARDWARE_SETUP.md):
- Conectar HX711 (célula de carga)
- Conectar motor de passo + endstop
- Conectar display TFT
- Conectar encoder KY-040

### 2. Configuração Crítica em `config.h`
```cpp
STEPPER_STEPS_PER_MM     // ✅ JÁ CORRETO: 3200 para trilho pitch 1mm
STEPPER_HOME_DIR_INT     // ⚠️ 0 ou 1, conforme direção do endstop
STEPPER_MAX_TRAVEL_MM    // Limite mecânico de curso
```

**Sistema Confirmado**:
- Trilho deslizante: pitch 1mm/volta
- NEMA11: 200 passos/volta
- TMC2209: 16x microsteps
- Cálculo: (200 × 16) / 1 = **3200 passos/mm** ✅

### 3. Compilar e Upload
```bash
platformio run -e esp32dev -t upload --upload-port COM?
```

### 4. Testar
Consultar [TESTING_GUIDE.md](TESTING_GUIDE.md) para:
- Verificação de componentes
- Testes de integração
- Validação de precisão

---

## 📁 Estrutura do Projeto

```
Medidor_Molas_RC/
├── platformio.ini              # Configuração PlatformIO
├── ARCHITECTURE.md             # Diagrama e fluxos
├── CODE_REVIEW.md              # Análise técnica + correções aplicadas
├── CHANGELOG.md                # Histórico de versões
├── HARDWARE_SETUP.md           # 🆕 Guia de hardware
├── TESTING_GUIDE.md            # 🆕 Guia de testes
├── include/
│   ├── config.h                # Configurações críticas
│   ├── encoder_manager.h       # ✅ ISR segura
│   ├── scale_manager.h         # ✅ Com validação EEPROM
│   ├── stepper_manager.h       # ✅ Com detecção de falha
│   └── ui_manager.h
├── src/
│   ├── main.cpp                # ✅ Com verificação de homing
│   ├── encoder_manager.cpp     # ✅ Melhorado
│   ├── scale_manager.cpp       # ✅ Melhorado
│   ├── stepper_manager.cpp     # ✅ Melhorado
│   └── ui_manager.cpp
└── lib/                        # Bibliotecas customizadas (vazio)
```

---

## 🔧 Componentes Principais

### Encoder Manager
- **ISR Segura**: Apenas flags, sem lógica pesada
- **Debounce**: 50ms (configurável)
- **Long Press**: 800ms para abortar testes

### Scale Manager
- **HX711**: Amplificador de célula de carga
- **Validação**: Verificação de EEPROM.commit()
- **Calibração**: Com weight reference (padrão 5kg)

### Stepper Manager
- **Homing**: Com timeout robusto (30s)
- **Proteção**: Limite de curso máximo
- **Detecção**: Flag de sucesso/falha

### UI Manager
- **Display TFT**: 240x320 pixels
- **Modo Menu**: Seleção com encoder
- **Modo Teste**: Gráfico em tempo real (até 40 pontos)

---

## �️ Hardware Específico

| Componente | Modelo | Especificação |
|---|---|---|
| **Microcontrolador** | ESP32 DevKit | 3.3V, 240 MHz |
| **Motor de Passo** | NEMA11 | 200 passos/volta, ~0.8-1.0A, compacto (28×28mm) |
| **Driver Stepper** | TMC2209 | 16x microsteps (padrão), silencioso |
| **Célula de Carga** | HX711 + Sensor | 5kg ou 10kg (conforme necessário) |
| **Display** | TFT ILI9341 | 240x320 pixels, SPI |
| **Encoder** | KY-040 | Com botão integrado |
| **Fonte do Motor** | - | **12V** (crítico) |
| **Alimentação uC** | USB | 5V (interno 3.3V) |
| **⚠️ Torque Motor** | - | Limitado (~0.2-0.3 Nm). Ideal para molas leves |

---

## �📊 Fluxo Principal

```
INICIALIZAÇÃO
    ↓
MENU PRINCIPAL ← [Encoder: nav] [Click: select]
    ↓
    ├─ Teste de Mola (k)
    │    ├ Homing (endstop → 0mm)
    │    ├ Tara (célula)
    │    ├ Coleta de N pontos
    │    │   ├ Move para posição
    │    │   ├ Média de leitura
    │    │   ├ Calcula K = Força / Deslocamento
    │    │   └ Plota no gráfico
    │    └ Resultado final
    │
    └─ Calibrar Balança
         ├ Selecionar peso de referência
         ├ Tara (sem peso)
         ├ Colocar peso
         └ Salva em EEPROM
```

---

## 📈 Melhorias Aplicadas (v2.0)

### Correções Críticas

| Problema | Solução | Impacto |
|----------|---------|--------|
| `millis()` em ISR | Removido, debounce em main | ✅ Seguro em ESP32 |
| Sem validação EEPROM | Adicionada verificação de commit | ✅ Detecta falha |
| Homing sem detecção | Adicionado `wasLastHomingSuccessful()` | ✅ Diagnóstico |
| Sem logs de erro | Adicionados em todos os críticos | ✅ Debug |

### Documentação Adicionada

- 📄 **HARDWARE_SETUP.md** (460 linhas)
  - Componentes com especificações
  - Cálculos de configuração
  - Troubleshooting por componente
  
- 📄 **TESTING_GUIDE.md** (450 linhas)
  - Testes por componente
  - Validação integrada
  - Logs esperados
  - Checklist de precisão

---

## ⚙️ Configurações Críticas

### Em `config.h`

#### Stepper (Motor de Passo)
```cpp
STEPPER_STEPS_PER_MM = 1600.0f      // ← AJUSTAR conforme motor
STEPPER_HOME_DIR_INT = 0             // ← Testar 0 ou 1
STEPPER_MAX_TRAVEL_MM = 40.0f
STEPPER_HOME_TIMEOUT_MS = 30000      // 30s (novo em v2.0)
```

#### Scale (Célula de Carga)
```cpp
SCALE_CALIB_DEFAULT = 1000.0f        // Valor inicial
SCALE_CALIB_REF_KG = 5.0f            // Peso para calibração
```

#### UI (Display)
```cpp
MAX_GRAPH_SAMPLES = 40               // Pontos do gráfico
GRAPH_MAX_FORCE_KG = 10.0f           // Escala visual
```

---

## 🧪 Teste Rápido (3 minutos)

1. **Serial @ 115200**: Deve aparecer "=== Medidor de mola - Inicializando ==="
2. **Encoder**: Girar e pressionar - Menu deve responder
3. **Motor**: Selecionar "Teste mola" - Motor deve se mover para endstop
4. **Célula**: Colocar peso - Leitura deve aumentar proporcionalmente
5. **Display**: Deve mostrar menu e gráfico

---

## 🆘 Troubleshooting

### Motor não se move
- [ ] Verificar endstop conectado em GPIO34
- [ ] Trocar `STEPPER_HOME_DIR_INT` (0 ↔ 1)
- [ ] Ver logs do Serial: `[STEPPER] ERROR: Homing timeout!`

### Célula não responde
- [ ] Verificar pinos HX711 (32 = DOUT, 33 = SCK)
- [ ] Abrir console serial e procurar por erro de HX711
- [ ] Testar com peso conhecido

### Display em branco
- [ ] Verificar pinos SPI do display
- [ ] Editar `User_Setup.h` da biblioteca TFT_eSPI
- [ ] Verificar tensão do backlight (GPIO21)

### Encoder não responde
- [ ] Verificar pinos: 18 (CLK), 19 (DT), 23 (SW)
- [ ] Testar com Serial.println em ISR
- [ ] Verificar debounce (50ms)

---

## 📚 Documentação Completa

| Documento | Conteúdo | Status |
|-----------|----------|--------|
| ARCHITECTURE.md | Diagrama e fluxos | ✅ |
| HARDWARE_SETUP.md | Componentes e configuração | ✅ 🆕 |
| TESTING_GUIDE.md | Testes completos | ✅ 🆕 |
| CODE_REVIEW.md | Análise técnica + correções | ✅ |
| CHANGELOG.md | Histórico de versões | ✅ |

---

## 🔍 Status da Compilação

```
✅ Sem erros
✅ Sem warnings
✅ Compatível com ESP32
✅ Dependências: HX711@0.7.5, TFT_eSPI@2.5.43
```

---

## 📋 Checklist Pré-Deployment

- [ ] Revisar HARDWARE_SETUP.md
- [ ] Calcular STEPPER_STEPS_PER_MM
- [ ] Testar STEPPER_HOME_DIR_INT (0 ou 1)
- [ ] Calibrar célula com peso conhecido
- [ ] Executar TESTING_GUIDE.md completo
- [ ] Verificar logs do Serial para erros
- [ ] Testar com mola de baixa rigidez primeiro

---

## 🚀 Próximas Melhorias

1. **Watchdog** (ESP32 WDT) para resetar em travamento
2. **Telemetria** via WebSocket para monitorar remotamente
3. **Histórico** de testes em SPIFFS/SD
4. **PID** para movimento mais suave
5. **Auto-calibração** a cada 100 testes

---

## 📞 Suporte

Para problemas ou dúvidas:
1. Consultar HARDWARE_SETUP.md (configuração)
2. Consultar TESTING_GUIDE.md (testes)
3. Revisar CODE_REVIEW.md (análise técnica)
4. Abrir issue com logs completos do Serial

---

**Última Atualização**: 15 de janeiro de 2026  
**Versão**: 2.0 (Melhorado)  
**Maintainer**: Sistema de Review Automático
