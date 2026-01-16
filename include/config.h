#ifndef CONFIG_H
#define CONFIG_H

// ==== PINOS DO PROJETO ====

// Célula de carga (HX711)
constexpr int LOADCELL_DOUT_PIN = 32;
constexpr int LOADCELL_SCK_PIN  = 33;

// Motor de passo (driver tipo A4988/DRV8825)
constexpr int STEP_PIN   = 25;
constexpr int DIR_PIN    = 26;
constexpr int EN_PIN     = 27;

// Endstop de referência do eixo
constexpr int ENDSTOP_PIN = 34;

// Backlight do display TFT
constexpr int BL_PIN      = 21;

// Encoder KY-040
constexpr int ENC_CLK_PIN = 18;  // pino CLK
constexpr int ENC_DT_PIN  = 19;  // pino DT
constexpr int ENC_SW_PIN  = 23;  // botão do encoder

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
constexpr uint16_t STEPPER_DEFAULT_DELAY_US = 800;

// Máximo de tentativas de homing antes de falhar
constexpr int STEPPER_HOME_MAX_RETRIES = 1;

// Timeout para HX711 responder (ms)
constexpr unsigned long SCALE_READ_TIMEOUT_MS = 5000;

#endif
