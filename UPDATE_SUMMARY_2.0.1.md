# Resumo de Atualização v2.0.1 - Hardware Específico

**Data**: 15 de janeiro de 2026  
**Versão**: 2.0.1  
**Status**: ✅ Documentação Atualizada com Hardware Real

---

## 📋 O Que Foi Atualizado

### Hardware Confirmado
```
✅ Motor de Passo: NEMA11 (compacto 28×28mm)
✅ Corrente Motor: 0.8-1.0A (torque limitado)
✅ Driver Stepper: TMC2209 (não A4988/DRV8825)
✅ Tensão uC: 3.3V (ESP32)
✅ Tensão Motor: 12V (crítico!)
✅ Microsteps Padrão: 16x
⚠️ Torque: Adequado para molas leves (~0-2 kgf)
```

---

## 🔄 Arquivos Modificados

### 1. **HARDWARE_SETUP.md** ✅
- Substituído A4988/DRV8825 por TMC2209
- Adicionada tabela de configuração de microsteps (M0/M1)
- NEMA11 especificado (motor compacto 28×28mm)
- Limite de corrente 0.8-1.0A para NEMA11
- Aviso sobre torque limitado (ideal para molas leves)
- Fórmula de cálculo atualizada para 16x microsteps
- Notas sobre TMC2209 ser silencioso

### 2. **include/config.h** ✅
- STEPPER_STEPS_PER_MM: **1600 → 32011 com TMC2209 (16x microsteps padrão)"
- Adicionado aviso sobre torque limitado do NEMA11
- Comentário atualizado: "Motor NEMA28 com TMC2209 (16x microsteps padrão)"
- Fórmula de exemplo: (200 × 16) / 1 mm = 3200

### 3. **README.md** ✅
- Adicionada seção "🛠️ Hardware Específico"
- NEMA11 especificado (compacto, torque limitado)
- Driver TMC2209 com detalhe de 16x microsteps
- Tensão motor 12V destacada
- Aviso sobre torque limitado para molas leves de 16x microsteps
- Tensão motor 12V destacada

### 4. **CHANGELOG.md** ✅
- Nova versão v2.0.1 documentada
- Alterações de hardware listadas
- Ações recomendadas incluídas

### 5. **HARDWARE_REFERENCE.md** 🆕 (Novo!)
- Guia rápido de referência
- Pinos ESP32 organizados por componente
- Cálculo STEPPER_STEPS_PER_MM com exemplos
- Configuração TMC2209 (resistência, LED, M0/M1)
- Troubleshooting rápido
- Checklist de segurança

---

## 🧮 Cálculo Atualizado

### Fórmula
```
STEPPER_STEPS_PER_MM = (200 passos/volta × 16 microsteps) / deslocamento_por_volta_mm
```

### Exemplos Comuns
| Parafuso | Passo | Deslocamento | STEPPER_STEPS_PER_MM |
|---|---|---|---|
| M8 | 1.25mm | 1.25mm/volta | 2560 |
| M10 | 1.5mm | 1.5mm/volta | 2133 |
| M8 | 1.0mm | 1.0mm/volta | **3200** ← Padrão |

---

## ⚠️ Pontos Críticos

### 1. **Tensão Motor 12V**
- ❌ Não usar 5V ou 24V
- ✅ Fonte estabilizada de 12V / 2A mínimo
- ✅ Pólo negativo comum com ESP32

### 2. **TMC2209 vs A4988**
| Aspecto | A4988 | TMC2209 |
|---|---|---|
| Microsteps | 1-16x | 1-256x |
| Ruído | Alto | Muito baixo |
| Controle | Jumpers | Pinos M0/M1 |
| Proteção | Básica | Avançada (termostato) |
| Corrente | Manual | Potenciômetro |

### 3. **Limite de Corrente**
- Potenciômetro do TMC2209 ajustar para ~1.5A
- NEMA28 tem torque menor que NEMA17
- Fórmula: IMAX = VREF × 2 (A)

---

## 📊 Antes vs. Depois

| Aspecto | v2.0 | v2.0.1 |
|---|---|---|
| Motor | Genérico NEMA17 | NEMA28 especificado |
| Driver | A4988/DRV8825 sugerido | TMC2209 confirmado |
| Microsteps | 8x exemplo | 16x confirmado |
| STEPPER_STEPS_PER_MM | 1600 (padrão) | 3200 (padrão atualizado) |
| Tensão Motor | "12V ou 24V" | 12V (crítico!) |
| Documentação | Genérica | Hardware específico |

---

## ✅ Próximas Ações

### Antes de Compilar
1. ✅ Verificar pinos M0/M1 do TMC2209
2. ✅ Ajustar potenciômetro de corrente
3. ✅ Validar fonte 12V

### Antes de Testar
1. ✅ Medir deslocamento real do parafuso/correia
2. ✅ Calcular STEPPER_STEPS_PER_MM preciso
3. ✅ Testar STEPPER_HOME_DIR_INT (0 ou 1)
4. ✅ Consultar HARDWARE_REFERENCE.md para verificação

### Durante Teste
1. ✅ Monitorar série para logs
2. ✅ Verificar movimento suave (sem vibração)
3. ✅ Validar leitura da célula de carga

---

## 📚 Documentação Completa

| Arquivo | Propósito | Tamanho |
|---|---|---|
| README.md | Visão geral + quick start | 276 linhas |
| ARCHITECTURE.md | Diagrama e fluxos | 110 linhas |
| HARDWARE_SETUP.md | Guia completo de hardware | 470+ linhas |
| HARDWARE_REFERENCE.md | Referência rápida (NEW) | 200+ linhas |
| TESTING_GUIDE.md | Procedimentos de teste | 450+ linhas |
| CODE_REVIEW.md | Análise técnica | 180+ linhas |
| CHANGELOG.md | Histórico de versões | 100+ linhas |

---

## 🎯 Status Final

```
✅ Código compila sem erros
✅ Hardware documentado específicamente
✅ Cálculos atualizados (STEPPER_STEPS_PER_MM)
✅ TMC2209 configurado (16x microsteps)
✅ NEMA28 especificado
✅ Tensão 12V destacada como crítica
✅ Referência rápida disponível
✅ Pronto para montagem e teste
```

---

## 📞 Suporte

Para dúvidas sobre:
- **Hardware**: Consulte [HARDWARE_SETUP.md](HARDWARE_SETUP.md)
- **Referência rápida**: Consulte [HARDWARE_REFERENCE.md](HARDWARE_REFERENCE.md)
- **Testes**: Consulte [TESTING_GUIDE.md](TESTING_GUIDE.md)
- **Detalhes técnicos**: Consulte [CODE_REVIEW.md](CODE_REVIEW.md)

---

**Versão**: 2.0.1  
**Data**: 15 de janeiro de 2026  
**Status**: ✅ Pronto para uso
