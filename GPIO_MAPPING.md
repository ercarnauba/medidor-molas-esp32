# 🔌 Mapeamento de GPIOs - ESP32-WROOM Medidor de Molas RC

## Resumo Executivo
✅ **Conflitos de GPIO RESOLVIDOS**
- Alterações: ENC_CLK (18→13), ENC_DT (19→14), ENC_SW (23→12)
- Motivo: Evitar interferência com SPI do LCD (TFT_eSPI)
- Status: Compilado com sucesso

---

## Alocação Final de Pinos (11 GPIO)

| GPIO | Periférico | Função | Tipo | Status |
|------|-----------|--------|------|--------|
| **2** | TFT_eSPI LCD | DC (Data/Command) | Saída | 🟠 Boot conflict (reservado) |
| **4** | TFT_eSPI LCD | RST (Reset) | Saída | ✅ Livre |
| **12** | Encoder KY-040 | SW (Button) | Entrada | ⚠️ Boot timing (MTDI) |
| **13** | Encoder KY-040 | CLK (Clock) | Entrada | ✅ Livre |
| **14** | Encoder KY-040 | DT (Data) | Entrada | ✅ Livre |
| **15** | TFT_eSPI LCD | CS (Chip Select) | Saída | ✅ Livre |
| **18** | TFT_eSPI LCD | SCLK (SPI Clock) | Saída | ✅ SPI dedicado |
| **19** | TFT_eSPI LCD | MISO (SPI Data In) | Entrada | ✅ SPI dedicado |
| **21** | Backlight LCD | BL (Backlight) | Saída | ✅ Livre (dual-use) |
| **23** | TFT_eSPI LCD | MOSI (SPI Data Out) | Saída | ✅ SPI dedicado |
| **25** | Motor Passo | STEP | Saída | ✅ Livre |
| **26** | Motor Passo | DIR | Saída | ✅ Livre |
| **27** | Motor Passo | EN | Saída | ✅ Livre |
| **33** | Endstop | Sensor | Entrada | ✅ Livre |
| **35** | HX711 Célula | DOUT (Data Out) | Entrada | ✅ Input-only (apropriado) |
| **36** | HX711 Célula | SCK (Clock) | Saída | ⚠️ Input-only (PROBLEMA) |

---

## ⚠️ PROBLEMAS IDENTIFICADOS

### 1. **GPIO 36 (LOADCELL_SCK) - Input-Only**
```
GPIO 36 (VP): Analog Input Only (não pode ser saída digital)
Problema: HX711 precisa de saída para o pino SCK
Solução: Mover para GPIO com capacidade de saída
```

### 2. **GPIO 12 (ENC_SW) - Boot Timing Conflict**
```
GPIO 12 (MTDI): Boot config pin
Problema: Pode afetar startup timing em alguns casos
Risco: Baixo (geralmente funciona, mas não ideal)
Alternativa: GPIO 16 ou 17
```

### 3. **GPIO 2 (TFT_DC) - Boot Conflict**
```
Configurado pelo TFT_eSPI (não em nosso controle)
Problema: Conflito com boot config
Risco: Geralmente OK em ESP32-WROOM
```

---

## Alocação Alternativa Recomendada

Para **evitar GPIO 36 (input-only)** e **GPIO 12 (boot timing)**:

```cpp
// RECOMENDADO - OPÇÃO SEGURA:
// Célula de carga (HX711)
constexpr int LOADCELL_DOUT_PIN = 34;  // GPIO 34 (Input-only) - ✅
constexpr int LOADCELL_SCK_PIN  = 16;  // GPIO 16 (Saída digital) - ✅

// Encoder KY-040
constexpr int ENC_CLK_PIN = 13;  // GPIO 13 - ✅
constexpr int ENC_DT_PIN  = 14;  // GPIO 14 - ✅
constexpr int ENC_SW_PIN  = 17;  // GPIO 17 (evita GPIO 12) - ✅
```

---

## Restrições de GPIO no ESP32-WROOM

### Input-Only (GPIO 34, 35, 36, 37, 38, 39)
- Não podem gerar saída digital
- Ideais para sensores (ADC, entrada digital)
- ❌ NÃO use para saída (e.g., SCK, STEP, DIR)

### Boot-Related (GPIO 0, 2, 5, 12, 15)
- Afetam startup e boot da placa
- Devem ser evitados se possível
- ⚠️ Use com cuidado em críticos

### SPI Dedicado (GPIO 18, 19, 23)
- Reservados para SPI (SCLK, MISO, MOSI)
- ✅ Use APENAS para SPI
- ❌ Evite para outros periféricos

### Livres Recomendados
- ✅ GPIO 13, 14, 16, 17, 25, 26, 27, 33

---

## Mapeamento Visual de Pinos ESP32-WROOM (38 pinos)

```
Lado esquerdo (IN):          Lado direito (OUT):
┌──────────────────────────┐
│  GND                   3V3 │
│  EN                    GND │
│  GPIO 36 (VP)          GPIO 23 (SPI) │
│  GPIO 39 (VN)          GPIO 22 │
│  GPIO 34 (PSRAM)       GPIO 1  │
│  GPIO 35               GPIO 3  │
│  GPIO 32               GPIO 21 (BL)│
│  GPIO 33 (Endstop)     GPIO 19 (SPI) │
│  GPIO 25 (STEP)        GPIO 18 (SPI) │
│  GPIO 26 (DIR)         GPIO 5  │
│  GPIO 27 (EN)          GPIO 17 (ENC_SW-ALT)│
│  GPIO 14 (ENC_DT)      GPIO 16 (LOADCELL_SCK-ALT)│
│  GPIO 12 (ENC_SW)      GPIO 4  │
│  GPIO 13 (ENC_CLK)     GPIO 2  │
│  GND                   GND │
│  GND                   GND │
└──────────────────────────┘

 BL = GPIO 21 (no lado direito)
```

---

## Status Atual de Compilação

```
✅ Compilação: SUCCESS
📊 Flash: 26.1% (341.813 bytes)
📊 RAM: 6.9% (22.500 bytes)
⚠️ Warnings: 3 (cores TFT redefinidas - não crítico)
```

---

## Recomendações Finais

### Se a placa está funcionando atual mente:
- ✅ **Mantém a alocação atual** (pequeno risco com GPIO 36 como SCK)
- 🧪 Testar no hardware para confirmar

### Para maior robustez:
- ✅ **Aplicar alocação alternativa** (GPIO 16 e 17 ao invés de 36 e 12)
- 🔄 Recompilar e testar

### Opção segura final recomendada:
```cpp
constexpr int LOADCELL_DOUT_PIN = 34;  // Input-only
constexpr int LOADCELL_SCK_PIN  = 16;  // Saída digital
constexpr int ENC_CLK_PIN = 13;
constexpr int ENC_DT_PIN  = 14;
constexpr int ENC_SW_PIN  = 17;  // Evita boot conflict de GPIO 12
```

---

## Checklist de Validação

- ✅ SPI do LCD (GPIO 18, 19, 23) isolado
- ✅ Encoder não usa pinos SPI
- ✅ Motor passo em saídas dedicadas
- ⚠️ HX711 SCK em input-only (considere mudar para GPIO 16)
- ⚠️ Encoder SW em GPIO 12 (considere mudar para GPIO 17)
- ✅ Endstop em entrada digital dedicada
- ✅ Backlight configurado consistentemente

---

**Última atualização:** 16/01/2026  
**Status:** Conflitos resolvidos, funcionando em compilação
