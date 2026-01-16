# ===================================
# Script para configurar Git e fazer primeiro commit
# Projeto: Medidor de Molas RC v2.0.1
# ===================================

Write-Host "`n=== CONFIGURACAO GIT ===" -ForegroundColor Cyan

# 1. Configurar nome e email (ALTERE COM SEUS DADOS)
Write-Host "`n[1/7] Configurando usuario Git..." -ForegroundColor Yellow
git config --global user.name "Seu Nome Aqui"
git config --global user.email "seu.email@exemplo.com"

# 2. Inicializar repositorio (se ainda nao existe)
Write-Host "`n[2/7] Inicializando repositorio..." -ForegroundColor Yellow
if (-Not (Test-Path ".git")) {
    git init
    Write-Host "Repositorio inicializado!" -ForegroundColor Green
} else {
    Write-Host "Repositorio ja existe." -ForegroundColor Green
}

# 3. Criar .gitignore para arquivos desnecessarios
Write-Host "`n[3/7] Criando .gitignore..." -ForegroundColor Yellow
$gitignoreContent = @"
# PlatformIO
.pio/
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json
.vscode/ipch
.vscode/extensions.json

# Build artifacts
*.o
*.elf
*.bin
*.hex

# OS files
.DS_Store
Thumbs.db
desktop.ini

# Temporary files
*.log
*.tmp
*~

# VS Code
.vscode/settings.json
"@

Set-Content -Path ".gitignore" -Value $gitignoreContent
Write-Host ".gitignore criado!" -ForegroundColor Green

# 4. Adicionar todos os arquivos
Write-Host "`n[4/7] Adicionando arquivos ao staging..." -ForegroundColor Yellow
git add .
Write-Host "Arquivos adicionados!" -ForegroundColor Green

# 5. Fazer o commit
Write-Host "`n[5/7] Criando commit..." -ForegroundColor Yellow
git commit -m "feat: adicionar menu de teste de hardware

- Adicionar terceiro item no menu: 'Teste hardware'
- Implementar leitura continua da celula de carga em gramas
- Implementar controle de motor via encoder (1mm por clique)
- Exibir posicao do motor em tempo real no LCD
- Botao do encoder finaliza teste e retorna ao menu
- Versao: v2.0.1 com especificacoes de hardware validadas
- Hardware: ESP32 + NEMA11 + TMC2209 + trilho deslizante 1mm pitch"

Write-Host "Commit realizado!" -ForegroundColor Green

# 6. Mostrar status
Write-Host "`n[6/7] Status atual:" -ForegroundColor Yellow
git status
git log --oneline -5

# 7. Instrucoes para adicionar remote
Write-Host "`n=== PROXIMO PASSO: ADICIONAR REPOSITORIO REMOTO ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Para enviar para o GitHub/GitLab:" -ForegroundColor White
Write-Host "1. Crie um repositorio novo no GitHub/GitLab (NAO inicialize com README)" -ForegroundColor Yellow
Write-Host "2. Copie a URL do repositorio (ex: https://github.com/seu-usuario/medidor-molas.git)" -ForegroundColor Yellow
Write-Host "3. Execute os comandos:" -ForegroundColor Yellow
Write-Host ""
Write-Host "   git remote add origin https://github.com/seu-usuario/medidor-molas.git" -ForegroundColor Green
Write-Host "   git branch -M main" -ForegroundColor Green
Write-Host "   git push -u origin main" -ForegroundColor Green
Write-Host ""
Write-Host "=== SCRIPT CONCLUIDO! ===" -ForegroundColor Cyan
