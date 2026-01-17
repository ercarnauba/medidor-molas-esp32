#ifndef CONFIG_H
#define CONFIG_H

// ==== ALOCAÇÃO DE PINOS - ESP32-WROOM COM TFT_eSPI ====
// ⚠️ CONFLITOS RESOLVIDOS - Evita pinos reservados pelo TFT_eSPI (18, 19, 23, 15, 2, 4)
// Pinos reservados TFT (SPI): GPIO 18 (SCLK), 19 (MISO), 23 (MOSI), 15 (CS), 2 (DC), 4 (RST)
// Pinos recomendados ESP32-WROOM: 0, 1, 3, 5, 12, 13, 14, 16, 17, 21, 22, 25, 26, 27, 32, 33, 34, 35, 36, 37, 38, 39
// ⚠️ GPIO 34-39: APENAS ENTRADA (Analog Input Only) - NÃO USE PARA SAÍDA
// ⚠️ GPIO 0, 2: Boot conflict - evitar se possível
// ⚠️ GPIO 12: Boot conflict (MTDI) - evitar
// ⚠️ GPIO 5: Startup timing - evitar se possível

// ==== PINOS DO PROJETO ====

// Célula de carga (HX711) - Pinos de dados simples
constexpr int LOADCELL_DOUT_PIN = 34;  // GPIO 34 (Input only) - ✅ Apenas leitura
constexpr int LOADCELL_SCK_PIN  = 16;  // GPIO 16 (Saída digital) - ✅ Pode gerar saída para SCK

// Motor de passo (driver tipo A4988/DRV8825) - Pinos de saída digital
constexpr int STEP_PIN   = 25;  // GPIO 25 - ✅ Saída digital (livre)
constexpr int DIR_PIN    = 26;  // GPIO 26 - ✅ Saída digital (livre)
constexpr int EN_PIN     = 27;  // GPIO 27 - ✅ Saída digital (livre)

// Endstop de referência do eixo - Pino de entrada digital
constexpr int ENDSTOP_PIN = 33;  // GPIO 33 - ✅ Entrada digital (livre)

// Backlight do display TFT
constexpr int BL_PIN      = 21;  // GPIO 21 - ✅ Configurado tanto em config.h quanto em User_Setup.h

// Encoder KY-040 - Pinos de entrada digital (alternados para evitar SPI e boot conflicts)
constexpr int ENC_CLK_PIN = 13;  // GPIO 13 - ✅ Entrada digital (mudado de 18, que é SCLK do LCD)
constexpr int ENC_DT_PIN  = 14;  // GPIO 14 - ✅ Entrada digital (mudado de 19, que é MISO do LCD)
constexpr int ENC_SW_PIN  = 17;  // GPIO 17 - ✅ Entrada digital (mudado de 12, que tem boot conflict MTDI)

// ==== RESUMO DE ALOCAÇÃO (Total: 11 pinos) - VERSÃO OTIMIZADA ====
// GPIO 13: ENC_CLK (Encoder Clock) - ✅ Livre
// GPIO 14: ENC_DT (Encoder Data) - ✅ Livre
// GPIO 16: LOADCELL_SCK (HX711 Clock) - ✅ Saída digital (foi GPIO 36 input-only)
// GPIO 17: ENC_SW (Encoder Switch) - ✅ Livre (foi GPIO 12 boot conflict)
// GPIO 21: BL_PIN (Backlight) - ✅ Livre
// GPIO 25: STEP_PIN (Motor Step) - ✅ Livre
// GPIO 26: DIR_PIN (Motor Direction) - ✅ Livre
// GPIO 27: EN_PIN (Motor Enable) - ✅ Livre
// GPIO 33: ENDSTOP_PIN (Endstop sensor) - ✅ Livre
// GPIO 34: LOADCELL_DOUT (HX711 Data Out) - ✅ Input-only (apropriado)
// ✅ SPI (TFT): GPIO 18 (SCLK), 19 (MISO), 23 (MOSI), 15 (CS), 2 (DC), 4 (RST)
// ⚠️ EVITADOS: GPIO 0, 2 (Boot), 5 (Timing), 12 (Boot MTDI), 36 (Input-only)

// ==== CONSTANTES DE APLICAÇÃO ====

// Fator de calibração inicial da célula de carga (ajuste depois com peso de 5kg)
constexpr float SCALE_CALIB_DEFAULT = 1000.0f;

// Peso de referência para calibração automática
constexpr float SCALE_CALIB_REF_KG = 5.0f;

// Pesos disponíveis para calibração (em kg)
constexpr float SCALE_CALIB_WEIGHTS[] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
constexpr int SCALE_CALIB_WEIGHTS_COUNT = sizeof(SCALE_CALIB_WEIGHTS) / sizeof(SCALE_CALIB_WEIGHTS[0]);

// Comprimento livre da mola (informativo)
constexpr float SPRING_FREE_LENGTH_MM = 20.0f;

// ==== PARÂMETROS DO EIXO / MOTOR DE PASSO ====
// Motor NEMA11 com TMC2209 (16x microsteps padrão)
// Sistema: Trilho deslizante com pitch 1mm/volta
// Cálculo: 200 passos/volta * 16 microsteps / 1 mm = 3200 passos/mm ✅ CONFIRMADO
// ⚠️ NEMA11: Torque limitado (~0.2-0.3 Nm). Adequado para molas leves até ~2 kgf
constexpr float STEPPER_STEPS_PER_MM   = 3200.0f;   // ✅ VALOR CORRETO para trilho pitch 1mm
constexpr float STEPPER_MAX_TRAVEL_MM  = 40.0f;     // curso mecânico útil

// Direção do home (para o lado do fim de curso)
constexpr int STEPPER_HOME_DIR_INT = 0; // 0 = FORWARD, 1 = BACKWARD

// ==== TESTE PADRÃO DE MOLA ====

// Compressão padrão usada no teste (pode ajustar depois)
constexpr float DEFAULT_TEST_COMPRESSION_MM = 10.0f;

// Número máximo de pontos no gráfico durante o teste
constexpr int MAX_GRAPH_SAMPLES = 40;

// Estimativa de força máxima em kg para escalar o gráfico (só visual)
constexpr float GRAPH_MAX_FORCE_KG = 10.0f;

// ==== TIMEOUTS E SEGURANÇA ====
// Timeout para homing (ms)
constexpr unsigned long STEPPER_HOME_TIMEOUT_MS = 30000;  // 30 segundos

// Velocidade máxima do motor (microseconds entre passos)
constexpr int STEPPER_DEFAULT_DELAY_US = 800;

// Máximo de tentativas de homing antes de falhar
constexpr int STEPPER_HOME_MAX_RETRIES = 1;

// Timeout para HX711 responder (ms)
constexpr unsigned long SCALE_READ_TIMEOUT_MS = 5000;

// ==== CORES DO DISPLAY TFT (TFT_eSPI) ====
// Cores RGB565 para uso na interface
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_ORANGE      0xFD20
#define TFT_DARKGREY    0x7BEF
#define TFT_LIGHTGREY   0xC618

#endif
