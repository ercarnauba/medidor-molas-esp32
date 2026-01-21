# Referência Rápida de Hardware - Medidor_Molas_RC

## ⚡ Dados Confirmados

```
┌─────────────────────────────────────────────┐
│       CONFIGURAÇÃO DE HARDWARE ATUAL        │
├─────────────────────────────────────────────┤
│ Microcontrolador:   ESP32 DevKit            │
│ Tensão uC:          3.3V (alimentado 5V)    │
│ Motor:              NEMA11 (200 passos/vol) │
│ Driver:             TMC2209 (SilentStepStick)│
│ Microsteps:         16x (padrão)            │
│ Tensão Motor:       12V (crítico!)          │
│ Fonte Recomendada:  12V / 1A min            │
└─────────────────────────────────────────────┘
```

---

## 📋 Pinos ESP32

### Motor de Passo (TMC2209)
| Pino ESP32 | Função | Descrição |
|---|---|---|
| **GPIO25** | STEP | Pulso de passo |
| **GPIO26** | DIR | Direção (HIGH=forward, LOW=backward) |
| **GPIO27** | EN | Enable (LOW=motor ligado) |
| **GPIO22** | UART TX | Comunicação UART (ESP32 -> TMC2209) |
| **GPIO35** | UART RX | Comunicação UART (TMC2209 -> ESP32) |
| **GPIO32** | DIAG | Sinal StallGuard (HIGH quando stall detectado) |

### Célula de Carga (HX711)
| Pino ESP32 | Função |
|---|---|
| **GPIO34** | DOUT (dados do HX711, input-only) |
| **GPIO16** | SCK (clock do HX711) |

### Fim de Curso (Endstop)
| Pino ESP32 | Função |
|---|---|
| **GPIO33** | ENDSTOP (INPUT_PULLUP, LOW quando pressionado) |

### Encoder KY-040
| Pino ESP32 | Função |
|---|---|
| **GPIO13** | CLK (rotação) |
| **GPIO14** | DT (rotação) |
| **GPIO17** | SW (botão) |

### Display TFT
| Pino ESP32 | Função |
|---|---|
| **GPIO21** | BL (backlight, PWM) |
| SPI (CLK, MOSI, MISO, CS, DC) | Conforme User_Setup.h |

---

## 🔌 Conexões TMC2209

### Comunicação UART (StallGuard Habilitado)
```
ESP32 GPIO22 (TX) -> TMC2209 PDN_UART
ESP32 GPIO35 (RX) <- TMC2209 PDN_UART
ESP32 GPIO32     <- TMC2209 DIAG (pull-up interno no ESP32)

Baud Rate: 115200
Address: 0b00 (padrão, único driver no barramento)
```

### Pinos de Microstep (M0, M1)
```
M0 e M1: deixar em aberto (flutuantes)
Padrão: 16x microsteps

Configurável via UART: 1, 2, 4, 8, 16, 32, 64, 128, 256 microsteps
```

### StallGuard (Detecção de Travamento)
```
Pino DIAG conectado a GPIO32
Quando stall detectado: DIAG vai HIGH

Configuração:
- SGTHRS: 10 (threshold padrão, ajustável 0-255)
- Quanto MAIOR o threshold, MENOS sensível
- Valores recomendados: 5-20 para colisão
- Motor recua automaticamente 10mm após stall

⚠️ StallGuard NÃO afeta leitura da mola durante teste
Proteção ativa principalmente no topo do trilho
```

### Potenciômetro de Corrente
```
Regulador VREF no TMC2209 (IGNORADO se usar UART)
Corrente configurada via software: 800mA (NEMA11)
Fórmula manual: IMAX = VREF × 2 (A)

⚠️ Com comunicação UART ativa, corrente é controlada via código
```

---

## 🧮 Cálculo STEPPER_STEPS_PER_MM

### Fórmula
```
STEPPER_STEPS_PER_MM = (passos/volta × microsteps) / deslocamento_por_volta_mm

Motor NEMA11: 200 passos/volta
TMC2209: 16x microsteps (padrão)
Trilho deslizante: pitch 1mm/volta ✅ CONFIRMADO
```

### Cálculo para Seu Sistema (Trilho Pitch 1mm)
```
STEPPER_STEPS_PER_MM = (200 × 16) / 1.0 = 3200 ✅ VALOR CORRETO

Este é exatamente o valor padrão em config.h!
Não é necessário alterar STEPPER_STEPS_PER_MM.
```

### Outros Exemplos (Referência)

**Parafuso M8 (passo 1.25mm):**
```
STEPPER_STEPS_PER_MM = (200 × 16) / 1.25 = 2560
```

**Parafuso M10 (passo 1.5mm):**
```
STEPPER_STEPS_PER_MM = (200 × 16) / 1.5 = 2133
```

**Nota NEMA11**: Motor menor, torque limitado (~0.2-0.3 Nm). Ideal para molas leves.

---

## ⚙️ Configuração TMC2209

### Corrente do Motor (via UART)
```
NEMA11 recomendado: 600-1000mA
Configuração atual: 800mA RMS, 400mA hold

⚠️ Não exceder corrente nominal do motor!
Verificar datasheet do motor NEMA11 específico

Ajuste via código em config.h:
TMC_CURRENT_RMS = 800
TMC_CURRENT_HOLD = 400
```

### Modo de Operação
```
StealthChop (padrão): Silencioso, menos torque
SpreadCycle: Mais torque, mais barulho

Configurado via software (tmc2209Manager.enableStealthChop)
```

### Calibração StallGuard
```
1. Iniciar com threshold = 10
2. Fazer teste de movimento completo
3. Se falsos positivos: aumentar threshold (menos sensível)
4. Se não detectar colisão: diminuir threshold (mais sensível)
5. Valor ideal: detecta colisão mas não mola dura

Ajuste em config.h: TMC_STALLGUARD_THRESHOLD
```

---

## 🔍 Verificação Rápida

### ✅ Pré-teste
```bash
1. Fonte 12V conectada? Tensão estável?
2. ESP32 alimentado por USB (5V)?
3. Pinos do motor conectados?
4. Endstop pressionável mecanicamente?
5. Célula de carga sem peso lê ~0g?
6. Display mostra "Menu principal"?
```

### ✅ Durante Teste
```bash
1. Motor se move na direção correta?
2. Célula de carga responde a peso?
3. Encoder navega no menu?
4. Display não pisca ou trava?
```

### ✅ Pós-teste
```bash
1. Calibração salva em EEPROM?
2. Constante K calculada corretamente?
3. Sem travamentos ou resets?
```

---

## 🚨 Troubleshooting Rápido

| Sintoma | Causa Provável | Solução |
|---|---|---|
| Motor não se move | Polaridade invertida | Trocar fios do motor |
| Motor zumbindo, não se move | TMC2209 não alimentado | Verificar 12V |
| Homing falha | Endstop não toca | Puxar botão do endstop manualmente |
| Célula lê zero sempre | HX711 sem CLK | Verificar GPIO33 |
| Display branco | TFT não inicializado | Verificar User_Setup.h |
| Encoder não responde | ISR não habilitada | Verificar pinos 18/19 |

---

## 📊 Checklist de Segurança

- [ ] Fonte 12V estabilizada
- [ ] Pólo negativo 12V conectado a GND do ESP32
- [ ] Limite de corrente TMC2209 ajustado (~1.5A)
- [ ] Endstop instalado e testado manualmente
- [ ] Estrutura mecânica rígida (sem vibração)
- [ ] Fios de motor blindados se possível
- [ ] Testado com mola fraca primeiro
- [ ] Série monitorada durante teste (logs)

---

## 📚 Referências

- **TMC2209 Datasheet**: [trinamic.com](https://trinamic.com)
- **HARDWARE_SETUP.md**: Guia completo
- **TESTING_GUIDE.md**: Procedimentos de teste
- **CODE_REVIEW.md**: Detalhes técnicos

---

**Versão**: 2.0.1  
**Data**: 15 de janeiro de 2026  
**Status**: ✅ Documentado
