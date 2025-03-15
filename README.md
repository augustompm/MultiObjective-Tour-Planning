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
- `/utils` - Ferramentas auxiliares, incluindo implementação do algoritmo HSO para cálculo de hipervolume

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

- Python 3.8+
- Bibliotecas Python listadas em `requirements.txt`

## Uso do Algoritmo

Para executar o algoritmo NSGA-II e gerar novas soluções:

```bash
cd src
python main.py
```

Os parâmetros de configuração podem ser ajustados no arquivo `config.py`.

## Avaliação de Hipervolume

Para calcular o hipervolume das soluções geradas:

```bash
cd utils
python hypervolume.py --input ../results/nsga2-resultados.csv
```

## Citação

Se você utilizar este código em sua pesquisa, por favor cite nosso trabalho:

```
@inproceedings{mendonca2025otimizacao,
  title={Otimiza{\c{c}}{\~a}o Multiobjetivo para Planejamento de Rotas Tur{\'\i}sticas: Um Dia no Rio de Janeiro},
  author={Mendon{\c{c}}a, Augusto Magalh{\~a}es Pinto de and Sousa, Filipe Pessoa and Coelho, Igor Machado},
  booktitle={Proceedings of the CNMAC 2025},
  year={2025}
}
```

## Licença

Este projeto está licenciado sob a licença MIT.

## Contato

- Augusto Magalhães Pinto de Mendonça - [augustompm@id.uff.br](mailto:augustompm@id.uff.br)
- Filipe Pessoa Sousa - [filipe.sousa@pos.ime.uerj.br](mailto:filipe.sousa@pos.ime.uerj.br)
- Igor Machado Coelho - [imcoelho@ic.uff.br](mailto:imcoelho@ic.uff.br)