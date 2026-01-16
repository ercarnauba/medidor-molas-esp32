# Guia de Testes - Medidor_Molas_RC

## 1. Teste Básico de Componentes (Sem montagem mecânica)

### 1.1 Verificar Serial Console
```bash
# Abrir terminal serial com:
# - Porta: COM? (confira qual é no Gerenciador de Dispositivos)
# - Baud rate: 115200
# - Data bits: 8, Stop bits: 1, Parity: None

# Resultado esperado:
# "=== Medidor de mola - Inicializando ==="
# "Fator de calib inicial: 1000.0000"
```

### 1.2 Testar Encoder
**Ação**: Girar o encoder
```
Resultado: Posição muda em console (se adicionado log)
```

**Ação**: Pressionar botão do encoder
```
Resultado: Menu muda de seleção
```

### 1.3 Testar Motor (SEM CARGA)
**Ação**: Entrar no menu "Teste mola (k)"
```
Resultado esperado:
- "[TESTE] Iniciando teste de mola..."
- "[TESTE] Homing..."
- "[STEPPER] Endstop found!" ou "[STEPPER] ERROR: Homing timeout!"
```

**Se falhar**: 
- Verificar conexão do endstop
- Mudar `STEPPER_HOME_DIR_INT` (0 ou 1)
- Adicionar log: `Serial.println(digitalRead(ENDSTOP_PIN))` em `isEndstopPressed()`

### 1.4 Testar Célula de Carga (HX711)
**Ação**: Entrar em "Calibrar balanca"
```
Resultado esperado:
- Tela mostra peso atual ≈ 0.0 kg
- Ao selecionar peso (ex: 5kg), tela solicita peso conhecido
```

**Colocar peso físico (5kg)** na célula
```
Resultado esperado:
- Peso aumenta na tela após clicar
- "[CALIB] Calibration saved to EEPROM successfully."
```

### 1.5 Testar Display TFT
**Ação**: Ao inicializar ou clicar no menu
```
Resultado esperado:
- Tela mostra "Menu principal"
- "Teste mola (k)"
- "Calibrar balanca"
- Opção selecionada em branco (invertida)
```

---

## 2. Teste Integrado Com Montagem Mecânica

### 2.1 Setup Mecânico
1. **Desabilitar motor**: `enable(false)`
2. **Posicionar mecanismo**: Mola em posição neutra, célula sem pressão
3. **Conectar endstop**: Motor deve encontrá-lo ao mover em certa direção
4. **Verificar curso**: Motor deve mover ~40mm máx (conforme `STEPPER_MAX_TRAVEL_MM`)

### 2.2 Teste de Homing
**Ação**: Entrar em "Calibrar balanca" ou "Teste mola"
```
Console esperado:
[TESTE] Iniciando teste de mola...
[TESTE] Homing...
[STEPPER] Starting homing...
[STEPPER] Endstop found!
[STEPPER] Homing successful, position reset to 0.
[TESTE] Home concluido, tara feita.
```

**Se falhar com timeout**:
```
[STEPPER] ERROR: Homing timeout! Endstop not found or stuck.
[TESTE] ERRO: Homing falhou! Verifique endstop e configuracao STEPPER_HOME_DIR_INT.
```

→ Solução: Mudar `STEPPER_HOME_DIR_INT` em `config.h` (0 ↔ 1)

### 2.3 Teste Completo de Mola
**Ação**: Selecionar "Teste mola (k)"

**Fluxo esperado**:
1. Tela mostra "Teste mola (k)" com gráfico vazio
2. Motor se move gradualmente
3. A cada posição, célula registra força
4. Gráfico desenha curva Força × Deslocamento
5. Após 40 amostras, mostra resultado: `K (kgf/mm): X.XX`

**Se cancelar**: Pressionar botão do encoder (long press)
```
[TESTE] Cancelado pelo encoder.
[TESTE] Retornando para 0 mm...
```

### 2.4 Teste de Calibração Final
**Ação**: "Calibrar balanca" com mola montada

**Processo**:
1. Selecionar peso de referência (ex: 5kg)
2. Posicionar mola sem carga → clicar "tara"
3. Colocar peso conhecido na mola
4. Clicar para confirmar calibração
5. Resultado salvo em EEPROM

---

## 3. Verificações Pós-Teste

### 3.1 Leitura de Serial para Diagnóstico
Abra o terminal serial e execute cada teste:

```bash
# Verificar inicialização
# → Deve aparecer: "Fator de calib inicial: XXXX"

# Testar encoder durante navegação
# → Posição deve mudar

# Testar homing
# → Deve aparecer uma das mensagens de sucesso/falha

# Testar calibração com peso
# → Deve aparecer: "Calibration successful: XXXX.XXXX"
```

### 3.2 Validar Dados Salvos
Após calibração, a EEPROM foi atualizada. **Reset do ESP32**:
```bash
# Desconectar e reconectar USB

# Console deve mostrar:
# "=== Medidor de mola - Inicializando ==="
# "Fator de calib inicial: XXXX.XXXX"  ← deve ser o novo valor, não 1000
```

Se mantém 1000, calibração não foi salva → verificar EEPROM.

---

## 4. Troubleshooting Rápido

| Problema | Verificar | Ação |
|----------|-----------|------|
| Motor não se move | Endstop conectado? | Adicionar `Serial.println(digitalRead(ENDSTOP_PIN))` |
| Homing falha | STEPPER_HOME_DIR_INT correto? | Trocar 0 ↔ 1 em `config.h` |
| Célula lê zero | HX711 respondendo? | `scale.is_ready()` retorna true? |
| Display em branco | TFT_eSPI configurado? | Verificar `User_Setup.h` |
| Encoder não responde | ISRs habilitadas? | Verificar `attachInterrupt()` em encoder_manager.cpp |
| Gráfico travado | Comunicação SPI lenta? | Testar com menos pontos (`MAX_GRAPH_SAMPLES = 20`) |

---

## 5. Validação de Precisão

### 5.1 Verificar Linearidade da Mola
Após homing bem-sucedido:
1. Colocar pesos conhecidos (0, 1kg, 2kg, 3kg)
2. Anotar leituras da célula
3. Plotar manualmente: Força × Deslocamento
4. Verificar se é uma reta (comportamento Hooke's Law linear)

### 5.2 Verificar Repetibilidade
Executar mesmo teste 3 vezes:
1. Teste 1: K = X.XX
2. Teste 2: K = X.XX
3. Teste 3: K = X.XX

Se valores diferem > 5%, verificar:
- Calibração da célula
- Rigidez da montagem
- Alinhamento da mola

---

## 6. Performance e Timing

### 6.1 Tempos Esperados
- **Homing**: 5-20 segundos (conforme STEPPER_HOME_TIMEOUT_MS)
- **Teste completo**: 60-120 segundos (40 pontos × ~2s por ponto)
- **Calibração**: 10-15 segundos

### 6.2 Otimizações
Se muito lento:
- Diminuir `MAX_GRAPH_SAMPLES` (menos pontos)
- Aumentar velocidade do motor: diminuir `STEPPER_DEFAULT_DELAY_US`
- Reduzir `delay(20)` em médias de leitura

Se muito rápido ou impreciso:
- Aumentar médias: de 5 para 10 leituras
- Aumentar delay entre amostras: de 20ms para 50ms

---

## 7. Logs Esperados Durante Execução Normal

```
=== Medidor de mola - Inicializando ===
Fator de calib inicial: 1000.0000
[TESTE] Iniciando teste de mola...
[TESTE] Homing...
[STEPPER] Starting homing...
[STEPPER] Endstop found!
[STEPPER] Homing successful, position reset to 0.
[TESTE] Home concluido, tara feita.
[TESTE] Cancelado pelo encoder.
[TESTE] Retornando para 0 mm...
[TESTE] Concluido.
K (kgf/mm): 0.50
K (N/mm): 4.90
```

---

## 8. Próximos Passos

✅ Todos os testes passando?
- Documentar os valores de K obtidos
- Criar base de dados de molas testadas
- Considerar adicionar ecrã tátil ou WebUI

❌ Algum teste falhando?
- Verificar logs em `CODE_REVIEW.md`
- Comparar com `HARDWARE_SETUP.md`
- Abrir issue no repositório com console output completo
