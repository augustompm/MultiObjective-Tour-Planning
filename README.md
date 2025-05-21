# MultiObjective-Tour-Planning

Planejamento de rotas turísticas de um dia no Rio de Janeiro utilizando otimização multiobjetivo.

## Sobre o Projeto

Este repositório contém a implementação de um sistema de otimização multiobjetivo para planejamento de rotas turísticas no Rio de Janeiro, desenvolvido como parte de uma pesquisa acadêmica. O sistema utiliza o algoritmo NSGA-II para gerar roteiros otimizados considerando quatro objetivos simultâneos:

1. Minimização de custos
2. Minimização de tempo de deslocamento
3. Maximização de atrações visitadas
4. Maximização da diversidade de bairros

## Estrutura do Repositório

- `/src` - Código-fonte do algoritmo NSGA-II adaptado para o problema de roteirização turística
- `/data` - Arquivos de dados das atrações e matrizes de distância/tempo
- `/visualization` - Aplicativo web "Um Dia no Rio" para visualização e navegação das soluções
- `/results` - Resultados experimentais e métricas de avaliação
- `/metrics` - Ferramentas auxiliares, incluindo implementação do algoritmo HSO para cálculo de hipervolume

## Aplicativo "Um Dia no Rio"

O diretório `/visualization` contém uma aplicação web desenvolvida com Flask e Dash que permite aos usuários:

- Visualizar o conjunto de soluções não-dominadas através de múltiplas técnicas de visualização
- Explorar interativamente os roteiros gerados
- Selecionar a solução que melhor atenda às suas preferências entre os quatro objetivos
- Ver detalhes de cada roteiro, incluindo sequência de atrações, horários e meios de transporte

### Executando o Aplicativo

Para rodar o aplicativo localmente:

```bash
cd visualization
pip install -r requirements.txt
python app.py
```

O aplicativo estará disponível em `http://localhost:8050`

## Requisitos

- C++17+ (compilador compatível)
- CMake 3.15+
- Python 3.8+ (para visualização)
- Bibliotecas Python listadas em `visualization/requirements.txt`

## Uso do Algoritmo

Para compilar e executar o algoritmo NSGA-II:

```bash
./run.sh
```

Ou manualmente:

```bash
mkdir build && cd build
cmake ..
cmake --build .
./bin/tourist_route
```

## Avaliação de Hipervolume

Para calcular o hipervolume das soluções geradas:

```bash
cd metrics
python calculate_hypervolume.py
```
## Licença

Este projeto está licenciado sob a licença MIT.

## Contato

- Augusto Magalhães Pinto de Mendonça - [augustompm@id.uff.br](mailto:augustompm@id.uff.br)
- Filipe Pessoa Sousa - [filipe.sousa@pos.ime.uerj.br](mailto:filipe.sousa@pos.ime.uerj.br)
- Igor Machado Coelho - [imcoelho@ic.uff.br](mailto:imcoelho@ic.uff.br)
