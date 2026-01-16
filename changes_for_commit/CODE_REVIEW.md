# Revisão de Código - Medidor_Molas_RC

Resumo rápido

Este documento lista potenciais problemas, condições de corrida e sugestões de melhoria encontradas nos arquivos do projeto.

1) Encoder ISRs (`include/encoder_manager.h` / `src/encoder_manager.cpp`)

- Observação: as ISRs (`encoderISR` e `encoderButtonISR`) atualizam variáveis voláteis que o código principal lê com `noInterrupts()` — isso é correto em geral.

- Potencial problema: `encoderButtonISR()` usa `millis()` para debounce dentro da ISR. Em muitos ambientes (especialmente no ESP32/Arduino), chamar `millis()` dentro de uma ISR não é seguro porque `millis()` pode depender de interrupções ou APIs não reentrantes. Recomenda-se:
  - Fazer debounce no contexto da tarefa principal (ex.: ler `digitalRead(ENC_SW_PIN)` e aplicar debounce no `loop`), ou
  - Usar uma forma ISR-safe de marcar o tempo (por exemplo, `portENTER_CRITICAL_ISR`/`xTaskGetTickCountFromISR()` no ESP-IDF), ou
  - Simplesmente setar `_buttonClicked = true` na ISR e deixar o código principal ignorar bounces por lógica adicional (por exemplo, ignorar cliques por X ms após um clique registrado).

- Outra consideração: `digitalRead()` nas ISRs é relativamente lenta; embora geralmente funcione, para responsividade máxima pode-se ler diretamente os registradores ou reduzir lógica na ISR.

2) Concorrência / Atomicidade

- `EncoderManager::getPosition()` e `wasButtonClicked()` usam `noInterrupts()` para proteger leituras/escritas — isso é bom e evita condições de corrida.

- `ScaleManager::_currentKg` é atualizado em `ScaleManager::update()` (no loop principal) e lido por `getWeightKg()` sem proteção adicional. Como ambos correm no contexto principal (não nas ISRs), isso é seguro.

3) Uso de EEPROM (`src/scale_manager.cpp`)

- O código só executa `EEPROM` se `ESP32` estiver definido — confirmar que a placa alvo define `ESP32`.

- `EEPROM.begin(64)` usa 64 bytes; ok para armazenar um float e um magic number, mas documente o layout (já existe) e verifique compatibilidade entre builds.

4) Homing e movimento do motor (`src/stepper_manager.cpp`)

- `homeToEndstop(long maxSteps, ...)` e `moveSteps(...)` protegem contra curso máximo (usando `labs(_positionSteps) > maxStepsAbs`). Contudo:
  - Se a direção de homing estiver invertida (valor errado em `STEPPER_HOME_DIR_INT`), o homing poderá tentar mover além do curso e parar apenas por `maxSteps` — adicionar um timeout/segurança extra é recomendado.
  - Sugestão: adicionar timeout baseado em passos+tempo, e log/LED se home falhar.

- `enable(bool on)` assume `EN = LOW` habilita; documentar no README qual driver é esperado (A4988/DRV8825 etc.).

5) Estrutura do loop de teste (`src/main.cpp` => `runSpringTestWithGraph()`)

- Durante `stepperManager.moveToPositionMm()` o movimento é executado em blocking loops (delayMicroseconds). Isso é esperado, mas tenha em mente:
  - Não haverá atualização da UI ou leituras contínuas da célula enquanto o motor se move (o código faz leituras depois de mover e também faz médias com `delay(20)`), o que é normalmente aceitável para amostragem ponto-a-ponto.

- Cancelamento do teste por `BTN_PIN` usa `digitalRead(BTN_PIN)` dentro do laço de coleta — suficiente, mas se o botão for lido por ISR em outro lugar, sincronizar seria necessário.

6) Uso de `Serial.print` com floats

- Em alguns ambientes o `Serial.printf`/`Serial.print` com precisão exige certas flags; o código usa `Serial.println(value, 4)` e isso é compatível com Arduino.

7) Recomendações gerais de melhoria

- Evitar chamadas potencialmente não-ISR-safe dentro de ISRs (ex.: `millis()`), mover debounce para o loop principal.
- Adicionar timeouts e detecção de falha durante homing e movimentos longos.
- Validar pinos com o hardware real e documentar no `README` (especialmente `ENC_*`, `BTN_PIN`, `ENDSTOP_PIN`, `STEP/DIR/EN`).
- Considerar proteção contra perda de passos (monitorar corrente do driver ou adicionar sensores, se crítico).
- Adicionar logs mais detalhados via `Serial` em casos de erro (home não encontrado, peso fora do esperado etc.).

8) Pontos menores / pedacinhos para revisar

- `config.h`: revisar `STEPPER_STEPS_PER_MM` e `STEPPER_HOME_DIR_INT` antes do teste físico.
- `ui_manager.cpp`: `TFT_eSPI` pressupõe configurações corretas do `User_Setup.h` da biblioteca; confirmar configuração do driver/TFT.

Se quiser, eu posso:
- Substituir o debounce por uma solução safe (mover para polling no loop) e aplicar a correção no `src/encoder_manager.cpp` (mudança simples), ou
- Adicionar timeouts ao homing em `src/stepper_manager.cpp`.
