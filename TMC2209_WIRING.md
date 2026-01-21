# 🔌 Diagrama de Conexões TMC2209 StallGuard

## ESP32 ↔ TMC2209 - Conexões Completas

```
┌────────────────────────────────────────────────────────────┐
│                    ESP32-WROOM                             │
│                                                            │
│  GPIO 25 (STEP)  ─────────────────────────► STEP          │
│  GPIO 26 (DIR)   ─────────────────────────► DIR           │
│  GPIO 27 (EN)    ─────────────────────────► EN            │
│                                                            │
│  GPIO 22 (TX)    ─────────────────────────► PDN_UART      │
│  GPIO 35 (RX)    ◄───────────────────────── PDN_UART      │
│  GPIO 32 (DIAG)  ◄───────────────────────── DIAG          │
│                                                            │
│  GPIO 33 (END)   ◄──── [Micro Switch] ──── GND            │
│                                                            │
└────────────────────────────────────────────────────────────┘
                            │
                            │ 12V Power
                            ▼
        ┌─────────────────────────────────────┐
        │         TMC2209 Driver              │
        │                                     │
        │  VIO ◄──── 3.3V (do ESP32 ou LDO)  │
        │  VM  ◄──── 12V (Fonte externa)     │
        │  GND ◄──── GND comum               │
        │                                     │
        │  1A  ────► Motor Coil A+           │
        │  1B  ────► Motor Coil A-           │
        │  2A  ────► Motor Coil B+           │
        │  2B  ────► Motor Coil B-           │
        │                                     │
        │  PDN_UART: TX/RX combinado (UART)  │
        │  DIAG: Saída StallGuard            │
        │                                     │
        │  MS1, MS2: Deixar aberto (16µstep) │
        │  INDEX: Não conectar               │
        └─────────────────────────────────────┘
                            │
                            ▼
                ┌──────────────────────┐
                │   NEMA11 Motor       │
                │   (200 steps/rev)    │
                │   ~0.8A RMS          │
                └──────────────────────┘
```

---

## 📋 Lista de Conexões (Passo a Passo)

### 1. Alimentação
```
ESP32:
  - USB 5V → alimenta ESP32
  - 3.3V pin → VIO do TMC2209

TMC2209:
  - VM (12V) ← Fonte externa 12V/1A
  - GND ← GND comum (ESP32 + Fonte 12V)
```

### 2. Sinais de Controle (STEP/DIR/EN)
```
ESP32 GPIO 25 → TMC2209 STEP (pulsos de passo)
ESP32 GPIO 26 → TMC2209 DIR (direção)
ESP32 GPIO 27 → TMC2209 EN (enable, LOW = ativo)
```

### 3. Comunicação UART (StallGuard)
```
ESP32 GPIO 22 (TX) → TMC2209 PDN_UART (transmite comandos)
ESP32 GPIO 35 (RX) ← TMC2209 PDN_UART (recebe status)
ESP32 GPIO 32      ← TMC2209 DIAG (sinal de stall, HIGH quando detecta)
```

### 4. Endstop (Proteção Adicional)
```
ESP32 GPIO 33 ← Micro Switch (NC ou NO)
  - Configurado como INPUT_PULLUP
  - LOW quando pressionado
  - Proteção no fundo do trilho (home)
```

### 5. Motor NEMA11
```
TMC2209:
  1A, 1B → Coil A (vermelho/azul típico)
  2A, 2B → Coil B (verde/preto típico)

⚠️ Polaridade: Se motor gira invertido, trocar 1A↔1B ou 2A↔2B
```

---

## ⚙️ Configuração de Jumpers/DIP Switches

### TMC2209 - Modo UART
```
MS1: Deixar ABERTO (não conectar)
MS2: Deixar ABERTO (não conectar)
CLK: Não conectar
```

### Endereço UART (padrão: 0b00)
```
MS1_AD0: LOW ou aberto
MS2_AD1: LOW ou aberto
→ Address = 0b00 (único driver no barramento)
```

---

## 🔍 Verificação de Conexões

### Checklist Pré-Ligação
- [ ] GND comum entre ESP32, TMC2209 e Fonte 12V
- [ ] VIO do TMC2209 conectado a 3.3V
- [ ] VM do TMC2209 conectado a 12V
- [ ] Polaridade da fonte 12V correta (+ e -)
- [ ] Motor conectado aos pinos corretos (1A/1B, 2A/2B)
- [ ] GPIO 22, 35, 32 conectados ao TMC2209
- [ ] Endstop conectado a GPIO 33

### Teste de Continuidade (com multímetro, fonte DESLIGADA)
```
ESP32 GND ↔ TMC2209 GND: Deve ter continuidade
ESP32 3.3V ↔ TMC2209 VIO: Deve ter ~3.3V quando ligado
TMC2209 VM: Deve ter 12V quando fonte ligada
```

---

## 📊 Pinagem TMC2209 (SilentStepStick)

```
        ┌─────────────────┐
     EN │1              16│ VM (12V)
   MS1 │2              15│ GND
   MS2 │3              14│ 1B (Motor)
    NC │4              13│ 1A (Motor)
   VIO │5              12│ 2A (Motor)
 INDEX │6              11│ 2B (Motor)
  DIAG │7  TMC2209     10│ GND
  STEP │8    Top       9 │ DIR
        │    View         │
     GND└─────────────────┘ PDN_UART
         
PDN_UART: Pino único para TX/RX (multiplexado)
DIAG: Saída de StallGuard (vai HIGH quando stall)
```

---

## 🛠️ Configuração via Software

### Parâmetros em config.h
```cpp
// Pinos
TMC_TX_PIN   = 22   // UART TX
TMC_RX_PIN   = 35   // UART RX  
TMC_DIAG_PIN = 32   // StallGuard

// Corrente
TMC_CURRENT_RMS  = 800   // 800mA (NEMA11)
TMC_CURRENT_HOLD = 400   // 50% em idle

// StallGuard
TMC_STALLGUARD_THRESHOLD = 10  // 0-255 (maior = menos sensível)
TMC_STALL_RETRACT_MM = 10.0    // Recuo após stall

// Resistor de sense
TMC_R_SENSE = 110  // 110mΩ (típico TMC2209)
```

### Comandos Serial (Debug)
```cpp
tmc2209Manager.getDiagnostics()  // Status completo
tmc2209Manager.getStallGuardValue()  // Valor SG_RESULT atual
tmc2209Manager.isCommunicationOK()   // Verifica UART
```

---

## 🚨 Troubleshooting Visual

### LED do TMC2209 (se tiver)
```
Piscando: ✅ Funcionando corretamente
Apagado: ❌ Sem alimentação VM ou VIO
Sempre ligado: ⚠️ Shutdown térmico ou curto-circuito
```

### Teste de Movimento Básico
```cpp
// No Serial Monitor, deve aparecer:
[TMC2209] Communication OK!
[TMC2209] Initialized successfully!
[TMC2209] RMS Current: 800 mA
[STEPPER] TMC2209 initialized successfully!

// Se aparecer:
[TMC2209] ERROR: Communication failed!
→ Verificar GPIO 22 e 35, e alimentação
```

### Teste de StallGuard
```
1. Ligar sistema
2. Mover motor para meio do curso
3. Bloquear trilho manualmente (segurar com a mão)
4. Deve aparecer no Serial:
   [TMC2209] ⚠️ STALL DETECTED! Motor blocked
   [STEPPER] ⚠️ STALLGUARD ATIVO! Motor travado
   [STEPPER] Retracted 10.0 mm after stall detection
5. LCD deve mostrar alerta vermelho
```

---

## 📐 Dimensões e Montagem

### Espaçamento de Pinos TMC2209
- Pitch: 2.54mm (0.1")
- Layout: 2x8 pinos (16 total)
- Compatible com: Protoboard, PCB perfurada

### Orientação do Motor
```
        ┌─────┐
        │     │  Shaft (eixo)
        ├─────┤
        │ □ □ │  Pinos (1A, 1B, 2A, 2B)
        │ □ □ │
        └─────┘
       NEMA11
```

---

## 📚 Documentos Relacionados

- **HARDWARE_REFERENCE.md**: Especificações completas
- **GPIO_MAPPING.md**: Tabela de todos os GPIOs
- **STALLGUARD_IMPLEMENTATION.md**: Detalhes da implementação
- **config.h**: Parâmetros configuráveis

---

**Versão**: 2.1.0  
**Data**: 18 de janeiro de 2026  
**Status**: ✅ Pronto para montagem
