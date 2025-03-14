#!/bin/bash

# run-movns.sh - Script para executar o algoritmo MOVNS e salvar resultados no diretório adequado

# Cores para saída
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Multi-Objective Variable Neighborhood Search Runner ===${NC}"
echo ""

# Verificar se o diretório build existe
if [ ! -d "build" ]; then
    echo -e "${RED}Diretório build não encontrado. Executando cmake...${NC}"
    mkdir -p build
    cd build
    cmake ..
    cd ..
fi

# Verificar se diretório results existe
if [ ! -d "results" ]; then
    echo -e "${BLUE}Criando diretório results...${NC}"
    mkdir -p results
fi

# Compilar o MOVNS
echo -e "${BLUE}Compilando MOVNS...${NC}"
cd build
make movns_main
if [ $? -ne 0 ]; then
    echo -e "${RED}Erro na compilação. Abortando.${NC}"
    exit 1
fi
cd ..

# Executar o MOVNS
echo -e "${BLUE}Executando MOVNS...${NC}"
cd build
./bin/movns_main

# Copiar os resultados para a pasta results na raiz
echo -e "${BLUE}Copiando resultados para o diretório results...${NC}"
if [ -f "resultados_movns.csv" ]; then
    cp resultados_movns.csv ../results/movns-resultados.csv
    echo -e "${GREEN}Arquivo movns-resultados.csv salvo em results/${NC}"
else
    echo -e "${RED}Arquivo resultados_movns.csv não encontrado!${NC}"
fi

if [ -f "movns-geracoes.csv" ]; then
    cp movns-geracoes.csv ../results/movns-geracoes.csv
    echo -e "${GREEN}Arquivo movns-geracoes.csv salvo em results/${NC}"
else
    echo -e "${RED}Arquivo movns-geracoes.csv não encontrado!${NC}"
fi

cd ..
echo -e "${GREEN}Execução do MOVNS concluída com sucesso!${NC}"