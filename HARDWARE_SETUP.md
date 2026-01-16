# Configuração de Hardware - Medidor_Molas_RC

## Componentes Necessários

### 1. **Microcontrolador**
- **ESP32 DevKit v1** (placa de desenvolvimento)
- Tensão do uC: **3.3V** (interno, alimentado via USB 5V)
- USB para programação e debug (Serial @ 115200 baud)

### 2. **Célula de Carga + Amplificador HX711**
- Célula de carga: tipicamente 5kg ou 10kg
- Amplificador HX711 com conector JST ou soldado
- Pinos:
  - `LOADCELL_DOUT_PIN = 32` (dados do HX711)
  - `LOADCELL_SCK_PIN = 33` (clock do HX711)

**Teste**: Com nada na célula, o Serial deve mostrar `Fator de calib inicial: 1000.0` (valor padrão)

### 3. **Motor de Passo + Driver**
- Motor de passo: **NEMA11** (200 passos/volta)
  - Tamanho: 28mm × 28mm (compacto)
  - Corrente típica: 0.8-1.0A
  - Torque: ~0.2-0.3 Nm (adequado para molas leves)
- Driver: **TMC2209** (SilentStepStick)
  - Suporta microsteps: 1, 2, 4, 8, 16, 32, 64, 128, 256x (configurável via pinos)
  - Muito silencioso e eficiente
  - Proteção contra corrente excessiva
- Microsteps padrão: 16x (para bom equilíbrio entre suavidade e velocidade)
- Pinos:
  - `STEP_PIN = 25` (pulso de passo)
  - `DIR_PIN = 26` (direção: HIGH=forward, LOW=backward)
  - `EN_PIN = 27` (ativo em LOW - habilita motor)
- Alimentação do motor: **12V** (conforme especificação)

**Importante**: Após configurar os microsteps no driver TMC2209, calcular:
```
STEPPER_STEPS_PER_MM = (passos/volta × microsteps) / deslocamento_mecânico_por_volta

Sistema atual (CONFIRMADO):
- Motor NEMA11: 200 passos/volta
- TMC2209: 16x microsteps
- Trilho deslizante: pitch 1mm/volta

STEPPER_STEPS_PER_MM = (200 × 16) / 1.0 = 3200 ✅ CORRETO
(Valor já configurado em config.h)

Configurações comuns (pinos M0/M1 do TMC2209):
- M0=LOW, M1=LOW   → 1x    (5 passos/rev)
- M0=HIGH, M1=LOW  → 2x    (10 passos/rev)
- M0=LOW, M1=HIGH  → 4x    (20 passos/rev)
- M0=HIGH, M1=HIGH → 16x   (80 passos/rev) [PADRÃO RECOMENDADO]
```

### 4. **Endstop (Fim de Curso)**
- Switch mecânico ou sensor indutivo
- Conexão: normalmente aberto (NO), ligado a GND quando pressionado
- Pino:
  - `ENDSTOP_PIN = 34` (INPUT_PULLUP)

**Ajuste necessário**: Se o motor não se move para o endstop, mudar `STEPPER_HOME_DIR_INT`:
- `0` = movimento FORWARD em direção ao endstop
- `1` = movimento BACKWARD em direção ao endstop

### 5. **Display TFT**
- Display TFT de 240x320 pixels (comumente SPI ILI9341 ou similar)
- Interface SPI (CLK, MOSI, MISO, CS, DC)
- Backlight: `BL_PIN = 21` (PWM)

**Configuração**: Editar `User_Setup.h` da biblioteca TFT_eSPI:
```cpp
#define TFT_CS   5     // chip select
#define TFT_DC   2     // data/command
#define TFT_MOSI 23    // MOSI
#define TFT_SCLK 18    // CLK
// etc - confira sua conexão específica
```

### 6. **Encoder Rotativo KY-040**
- Encoder com botão integrado
- Pinos:
  - `ENC_CLK_PIN = 18` (CLK - mudança de fase)
  - `ENC_DT_PIN = 19` (DT - dado)
  - `ENC_SW_PIN = 23` (botão - pressionado = LOW)
- Todos com INPUT_PULLUP internas no ESP32

### 7. **Montagem Mecânica**
- **Trilho deslizante**: Pitch 1mm/volta ✅ CONFIRMADO
- **Motor NEMA11**: Montado no trilho, torque limitado (~0.2-0.3 Nm)
- **Aplicação**: Adequado para molas leves até ~2 kgf
- **Mola a testar**: Colocada entre a célula de carga e o pistão móvel
- **Estrutura**: Rígida e nivelada para evitar erros de medição
- **Nota**: TMC2209 é silencioso; verificar vibração da estrutura em altas velocidades
- **⚠️ IMPORTANTE**: Se precisar de mais torque para molas rígidas, considerar upgrade para NEMA17

---

## Checklist Pré-Teste

- [ ] **Serial Console**: Conectar USB e abrir serial @ 115200 para debug
- [ ] **Tensão ESP32**: Verificar se está recebendo 5V USB (internamente 3.3V)
- [ ] **Motor**: Rodar manualmente; não deve travar
- [ ] **Endstop**: Pressionar manualmente; LED ou Serial deve detectar
- [ ] **HX711**: Sem peso, Serial mostra ~0g; com peso conhecido, aumenta proporcional
- [ ] **Encoder**: Girar e pressionar; Serial ou Serial mostra mudanças
- [ ] **Display**: Mostra menu inicial "Menu principal"
- [ ] **Calibração da mola**: Antes de testar, calibrar célula com peso conhecido

---

## Ajustes Críticos em `config.h`

### 1. **STEPPER_STEPS_PER_MM**
Calcular conforme seu motor NEMA11 + redução mecânica:
```
passos_por_volta = 200 (NEMA11)
microsteps = 16 (configurado no TMC2209: M0=HIGH, M1=HIGH)
deslocamento_por_volta_mm = 1.0 (trilho deslizante pitch 1mm) ✅ CONFIRMADO

STEPPER_STEPS_PER_MM = (200 × 16) / 1.0 = 3200 ✅ VALOR CORRETO

O valor em config.h já está correto (3200.0f).
NÃO é necessário alterar!
```

**IMPORTANTE para NEMA11**: Motor pequeno, torque limitado. Ideal para molas leves (~0-2 kgf).

### 2. **STEPPER_HOME_DIR_INT**
- `0` = tenta mover na direção FORWARD
- `1` = tenta mover na direção BACKWARD

**Teste**: Com console aberto, rode a calibração (segunda opção do menu). Se o motor não se mover ou se mover na direção errada, trocar este valor.

**Nota para TMC2209**: Verificar polaridade do motor. TMC2209 detecta automaticamente se os fios estão invertidos, mas ainda assim respeita a direção definida por DIR_PIN.

### 3. **STEPPER_MAX_TRAVEL_MM**
Distância máxima que o motor pode se mover (segurança). Deve ser menor que o comprimento físico disponível.

### 4. **SCALE_CALIB_DEFAULT** e **SCALE_CALIB_REF_KG**
- Primeiro, testar com um peso conhecido (ex: 5kg)
- Salvar calibração via menu "Calibrar balanca"
- Depois, usar sempre pesos dentro da faixa da célula

---

## Troubleshooting

| Sintoma | Possível Causa | Solução |
|---------|---|---|
| Serial não conecta | Driver USB não instalado | Baixar driver CH340 ou FTDI |
| Motor não se move | EN_PIN ou DIR_PIN invertido | Trocar `STEPPER_HOME_DIR_INT` |
| Homing falha (timeout) | Endstop não conectado ou invertido | Verificar conexão; adicionar Serial.println em `isEndstopPressed()` para debug |
| Célula lê zero sempre | HX711 não conectado | Verificar pinos DOUT/SCK |
| Display em branco | TFT não inicializado | Verificar `User_Setup.h` e pinos SPI |
| Encoder não responde | Pinos ou ISR não habilitada | Verificar `ENC_CLK_PIN`, `ENC_DT_PIN` |
| Debounce fraco no encoder | Debounce muito curto | Aumentar `DEBOUNCE_MS` em `encoder_manager.h` |

---

## Notas de Segurança

- ⚠️ **Sempre desabilitar motor** (`enable(false)`) ao manipular mecanicamente
- ⚠️ **Não forçar endstop** manualmente; deixar o motor acioná-lo
- ⚠️ **Verificar polaridade de motores e sensores** antes de ligar**0.8-1.0A para NEMA11**, não exceder!)
- ⚠️ **Tensão 12V para o motor**: Não usar 5V ou valores aleatórios
- ⚠️ **NEMA11 tem torque limitado**: Testar apenas com molas leves (~0-2 kgf)river (não deve exceder 1.5A para NEMA28)
- ⚠️ **Tensão 12V para o motor**: Não usar 5V ou valores aleatórios
- ⚠️ **Testar com mola fraca primeiro** antes de testar com molas rígidas
- ⚠️ **TMC2209 é silencioso**: Não significa que não está funcionando; verificar movimento visual

## Configuração TMC2209 - Pinos de Microstep

| M0  | M1  | Microsteps |
|-----|-----|------------|
| LOW | LOW | 1x         |
| HIGH| LOW | 2x         |
| LOW | HIGH| 4x         |
| HIGH| HIGH| **16x** ← RECOMENDADO|

**Nota**: Se M0/M1 não estão conectados, TMC2209 default para 16x microsteps.
