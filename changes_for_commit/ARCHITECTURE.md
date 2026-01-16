# Arquitetura - Medidor_Molas_RC

Diagrama de interação (visão simplificada):

[User] 
   | (encoder click / rotate, BTN)
   v
[Encoder (ISR)] ---> updates position / button flag
   |
   v
[Main loop] <-----------------------------------------------+
   |                                                      |
   | calls                                                 |
   |                                                      |
   v                                                      |
[UiManager]  <--- drawMenu / drawTestStatus / plotGraph ---+
   ^                                                      |
   |                                                      |
   |                                                      v
[ScaleManager] --reads--> [HX711] --measures--> [Loadcell] 
   |
   | (tare / calibrate / save EEPROM)                      
   v
[EEPROM] (store calibration factor)

[StepperManager] --drive--> [Step motor driver (STEP/DIR/EN)]
       |                                |
       | homing / moveToPositionMm       | moves carriage compressing spring
       v                                v
   [ENDSTOP switch]                   [Mechanical assembly / Mola]

Fluxos principais

- Teste da mola (`Teste mola (k)`):
  1. `stepperManager.homeToEndstop()` (homing até `ENDSTOP_PIN`)
  2. `scaleManager.tare()`
  3. Para N amostras: `stepperManager.moveToPositionMm(pos)` -> `scaleManager.update()` (média) -> `uiManager.plotGraphPoint()` -> calcula K
  4. Retorna a 0 mm e exibe K final

- Calibração (`Calibrar balanca`):
  1. Etapa 0: `scaleManager.tare()` sem peso
  2. Etapa 1: com peso conhecido (`SCALE_CALIB_REF_KG`) chama `scaleManager.calibrateWithKnownWeight()` e `saveCalibrationToEEPROM()`

Arquivos/classe principais

- `src/main.cpp` - fluxo da aplicação e menu
- `src/encoder_manager.cpp`, `include/encoder_manager.h` - encoder + botão (ISRs)
- `src/scale_manager.cpp`, `include/scale_manager.h` - HX711, tare, calibração, EEPROM
- `src/stepper_manager.cpp`, `include/stepper_manager.h` - movimentos e homing do motor de passo
- `src/ui_manager.cpp`, `include/ui_manager.h` - TFT (TFT_eSPI) desenho de telas e gráfico

Observações de deployment

- Código condicionado a ESP32 (uso de `EEPROM.begin()` e macros `ESP32` em `scale_manager`). Confirmar placa alvo.
- Verificar definições em `include/config.h` (`STEPPER_STEPS_PER_MM`, pinos, `STEPPER_HOME_DIR_INT`) antes do primeiro teste físico.
