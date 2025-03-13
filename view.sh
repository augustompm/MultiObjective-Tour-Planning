#!/bin/bash

# Script para iniciar a visualização dos resultados

# Caminho do diretório base do projeto
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VISUALIZATION_DIR="$BASE_DIR/visualization"

echo "==== Iniciando Visualizador de Resultados NSGA-II ===="
echo "Diretório de visualização: $VISUALIZATION_DIR"

# Verifica se o diretório de visualização existe
if [ ! -d "$VISUALIZATION_DIR" ]; then
    echo "ERRO: Diretório de visualização não encontrado."
    echo "Verifique se a pasta 'visualization' existe na raiz do projeto."
    exit 1
fi

# Entra na pasta de visualização
cd "$VISUALIZATION_DIR"

# Verifica se as dependências estão instaladas
if ! python3 -c "import dash, plotly, pandas" 2>/dev/null; then
    echo "AVISO: Algumas dependências parecem estar faltando."
    echo "Instalando dependências necessárias..."
    python3 -m pip install -r requirements.txt
fi

# Verifica se o script de visualização existe
if [ ! -f "results_visualizer.py" ]; then
    echo "ERRO: Script de visualização 'results_visualizer.py' não encontrado."
    exit 1
fi

# Inicia o servidor da visualização
echo ""
echo "Iniciando o servidor de visualização..."
echo "Acesse em seu navegador: http://127.0.0.1:8050/"
echo "Pressione Ctrl+C para encerrar o servidor."
echo ""

python3 app.py