#!/bin/bash

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Limpando build anterior...${NC}"
rm -rf build/
mkdir build

echo -e "${YELLOW}Entrando no diretório build...${NC}"
cd build

echo -e "${YELLOW}Executando CMake...${NC}"
if ! cmake ..; then
    echo -e "${RED}Erro no CMake${NC}"
    exit 1
fi

echo -e "${YELLOW}Compilando o projeto...${NC}"
if ! cmake --build .; then
    echo -e "${RED}Erro na compilação${NC}"
    exit 1
fi

echo -e "${GREEN}Build concluído com sucesso!${NC}"

echo -e "${YELLOW}Executando o programa...${NC}"
./bin/tourist_route

cd ..