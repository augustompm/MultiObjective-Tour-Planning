# src/paste.txt

import requests
import pandas as pd
import numpy as np
import time
from datetime import datetime
import locale
import os

# Configurar o locale para o padrão brasileiro
try:
    locale.setlocale(locale.LC_NUMERIC, 'pt_BR.UTF-8')
except:
    try:
        locale.setlocale(locale.LC_NUMERIC, 'pt_BR')
    except:
        pass  # Se falhar, manterá o locale padrão

# Ler a chave do Mapbox do arquivo .key
def ler_mapbox_token():
    try:
        with open(os.path.join(os.path.dirname(__file__), '.key'), 'r') as f:
            for linha in f:
                if linha.startswith('MAPBOX_ACCESS_TOKEN'):
                    return linha.split('=')[1].strip().strip('"\'')
        
        # Caso não encontre a chave no arquivo
        raise ValueError("Token do Mapbox não encontrado no arquivo .key")
    except Exception as e:
        print(f"Erro ao ler token do Mapbox: {e}")
        exit(1)

# Configurações das APIs
MAPBOX_ACCESS_TOKEN = ler_mapbox_token()
MAPBOX_DIRECTIONS_URL = "https://api.mapbox.com/directions/v5/mapbox"
OSRM_URL = "http://router.project-osrm.org/route/v1"

# Dados das atrações
atracoes_texto = """
Cristo Redentor;-22.95112457318234,-43.21057352457689;120;241.54;480;1140
Jardim Botanico;-22.96352231913769,-43.22267669743484;180;15;480;1020
Praia de Ipanema;-22.98648190982438,-43.20668238449197;180;0;0;1439
Parque Lage;-22.957975657009477,-43.21163217653976;120;0;540;1020
Praia do Arpoador;-22.988371516628888,-43.19400661701697;180;0;0;1439
Maracana;-22.911925545881697,-43.23019601022307;240;116;540;1020
Praia da Barra da Tijuca;-23.01102228463985,-43.366251547561305;180;0;0;1439
Centro Cultural Banco do Brasil;-22.9008215829648,-43.17658567839236;180;0;540;1260
Museu do Amanha;-22.894391989927207,-43.179644120721214;180;30;600;1080
AquaRio;-22.893263266025134,-43.19215237748509;120;75;540;1020
Escadaria Selaron;-22.915144047171196,-43.17917141918494;60;0;540;1020
Forte Copacabana;-22.985874970153922,-43.187643519181584;120;0;600;1140
Santa Teresa;-22.917876964161398,-43.18234504375717;180;0;0;1439
Praia da Reserva;-23.011285357037575,-43.39445089281464;180;0;0;1439
Pedra da Gavea;-22.99597439272693,-43.28472266217296;180;0;0;1439
Pedra do Telegrafo;-23.067969018064595,-43.55776410568555;180;0;0;1439
Prainha;-23.041009699524924,-43.505679936107384;180;0;0;1439
Morro Dois Irmaos;-22.949411255405114,-43.400237546435;120;0;0;1439
Mosteiro De Sao Bento;-22.87216248487049,-43.17383914228873;60;0;450;1020
Mirante Dona Marta;-22.944790699789518,-43.19637404801938;180;0;0;1439
Theatro Municipal do Rio de Janeiro;-22.90879113172147,-43.17652531918517;180;844.37;600;1080
Parque Nacional da Tijuca;-22.940431934212032,-43.292708669579135;180;0;0;1439
Real Gabinete Portugues Da Leitura;-22.905221850616293,-43.18225103452901;120;0;600;1020
Praia do Grumari;-23.0460357615992,-43.52638926202537;180;0;0;1439
Pedra Bonita;-22.985719223823942,-43.27938393838966;120;0;0;1439
Praia Da Joatinga;-23.01450718137832,-43.29039638082129;180;0;0;1439
Centro Cultural Jerusalem;-22.880454262721617,-43.27342889035068;60;0;540;1080
Bondinho de Santa Teresa;-22.90915947846921,-43.1786614073549;60;0;480;1050
Ilha Da Gigoia;-23.001454164140785,-43.308482875801054;120;0;0;1439
Pedra do Sal;-22.89773169416836,-43.1852938191857;60;0;0;1439
Catedral Metropolitana de São Sebastião;-22.910476429715835,-43.180744819185215;60;0;420;1020
Vista Chinesa;-22.97308519896875,-43.24942874801805;60;0;0;1439
Museu Historico Nacional;-22.905466691085117,-43.16957331918533;120;0;0;1439
Igreja de Sao Francisco da Penitência;-22.906428285929053,-43.17917332103655;120;0;0;1439
Parque das Ruinas;-22.91760459425989,-43.18234766336432;60;0;0;1439
Museu de Arte do Rio;-22.896398808250563,-43.181935048021664;180;30;540;1020
Mureta da Urca;-22.94275908887587,-43.16001580176129;120;0;0;1439
Quinta Da Boa Vista;-22.904996521731277,-43.22180686336484;120;0;0;1439
Feira de Sao Cristovao;-22.897493044468007,-43.220203705693464;180;10;600;1320
Biblioteca Nacional;-22.90791763107082,-43.17568936190707;120;0;600;1020
"""

# Registrar a hora para fins de log
timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
log_filename = f"rotas_turisticas_log_{timestamp}.txt"

# Definir nomes de arquivos de saída padrão sem timestamp
car_dist_file = "matriz_distancias_carro_metros.csv"
walk_dist_file = "matriz_distancias_pe_metros.csv"
car_time_file = "matriz_tempos_carro_min.csv"
walk_time_file = "matriz_tempos_pe_min.csv"

def log_message(message):
    """Registra mensagens em arquivo de log e no console"""
    with open(log_filename, "a", encoding="utf-8") as log_file:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_file.write(f"[{timestamp}] {message}\n")
    print(message)

log_message("Iniciando processo de obtenção de matrizes de distância e tempo")
log_message(f"Usando token do Mapbox do arquivo .key")

# Processar os dados
atracoes = []
for linha in atracoes_texto.strip().split("\n"):
    partes = linha.split(";")
    if len(partes) < 6:  # Verificar se há dados suficientes
        continue
    
    nome = partes[0]
    coords = partes[1].split(",")
    lat = float(coords[0])
    lon = float(coords[1])
    tempo_visita = int(partes[2])
    custo = float(partes[3])
    horario_abertura = int(partes[4])
    horario_fechamento = int(partes[5])
    
    atracoes.append({
        "nome": nome,
        "lat": lat,
        "lon": lon,
        "tempo_visita": tempo_visita,
        "custo": custo,
        "horario_abertura": horario_abertura,
        "horario_fechamento": horario_fechamento
    })

log_message(f"Processando {len(atracoes)} atrações turísticas")

# Nomes das atrações para uso nos DataFrames
nomes = [a["nome"] for a in atracoes]
n = len(atracoes)

# Função para obter rota do OSRM entre dois pontos
def obter_rota_osrm(origem_idx, destino_idx, perfil="driving"):
    """
    Obtém a rota entre dois pontos usando a API do OSRM
    
    Args:
        origem_idx (int): Índice da atração de origem
        destino_idx (int): Índice da atração de destino
        perfil (str): Perfil de transporte ('driving')
    
    Returns:
        tuple: (distância em metros, duração em minutos) ou (None, None) se erro
    """
    # Coordenadas no formato longitude,latitude
    origem_lon, origem_lat = atracoes[origem_idx]["lon"], atracoes[origem_idx]["lat"]
    destino_lon, destino_lat = atracoes[destino_idx]["lon"], atracoes[destino_idx]["lat"]
    
    # URL para a API do OSRM
    url = f"{OSRM_URL}/{perfil}/{origem_lon},{origem_lat};{destino_lon},{destino_lat}"
    params = {"overview": "false"}
    
    try:
        response = requests.get(url, params=params)
        
        if response.status_code != 200:
            log_message(f"Erro ao obter rota OSRM: código {response.status_code}")
            return None, None
        
        data = response.json()
        
        if data["code"] != "Ok" or len(data["routes"]) == 0:
            log_message(f"Erro no roteamento OSRM")
            return None, None
        
        distancia = int(data["routes"][0]["distance"])  # Arredondando para metros inteiros
        duracao = int(data["routes"][0]["duration"] / 60)  # Convertendo para minutos inteiros
        return distancia, duracao
        
    except Exception as e:
        log_message(f"Erro na requisição OSRM: {str(e)}")
        return None, None

# Função para obter rota do Mapbox entre dois pontos
def obter_rota_mapbox(origem_idx, destino_idx, perfil="walking"):
    """
    Obtém a rota entre dois pontos usando a API Directions do Mapbox
    
    Args:
        origem_idx (int): Índice da atração de origem
        destino_idx (int): Índice da atração de destino
        perfil (str): Perfil de transporte ('walking')
    
    Returns:
        tuple: (distância em metros, duração em minutos) ou (None, None) se erro
    """
    origem_lon, origem_lat = atracoes[origem_idx]["lon"], atracoes[origem_idx]["lat"]
    destino_lon, destino_lat = atracoes[destino_idx]["lon"], atracoes[destino_idx]["lat"]
    
    coordinates = f"{origem_lon},{origem_lat};{destino_lon},{destino_lat}"
    url = f"{MAPBOX_DIRECTIONS_URL}/{perfil}/{coordinates}"
    
    params = {
        "access_token": MAPBOX_ACCESS_TOKEN,
        "geometries": "geojson",
        "overview": "false",
        "steps": "false"
    }
    
    try:
        response = requests.get(url, params=params)
        
        if response.status_code != 200:
            log_message(f"Erro ao obter rota Mapbox: código {response.status_code}")
            return None, None
        
        data = response.json()
        
        if "routes" not in data or len(data["routes"]) == 0:
            log_message(f"Erro no roteamento Mapbox")
            return None, None
        
        distancia = int(data["routes"][0]["distance"])  # Arredondando para metros inteiros
        duracao = int(data["routes"][0]["duration"] / 60)  # Convertendo para minutos inteiros
        return distancia, duracao
        
    except Exception as e:
        log_message(f"Erro na requisição Mapbox: {str(e)}")
        return None, None

def obter_e_preencher_matrizes(usar_media_simetrica=True):
    """
    Obtém dados das APIs e preenche as matrizes, opcionalmente garantindo simetria
    através da média dos valores em ambas as direções para distâncias a pé.
    
    Args:
        usar_media_simetrica (bool): Se True, calcula a média para garantir simetria nas matrizes a pé
    """
    # Inicializar matrizes
    matriz_distancias_carro = np.zeros((n, n), dtype=int)
    matriz_tempos_carro = np.zeros((n, n), dtype=int)
    matriz_distancias_pe = np.zeros((n, n), dtype=int)
    matriz_tempos_pe = np.zeros((n, n), dtype=int)
    
    # Para armazenar temporariamente os valores originais (antes da média)
    matriz_distancias_pe_original = np.zeros((n, n), dtype=int)
    matriz_tempos_pe_original = np.zeros((n, n), dtype=int)

    # Obter dados do OSRM (distâncias e tempos de carro)
    log_message("\n=== OBTENDO DADOS DE CARRO (OSRM) ===")
    contador = 1
    total_pares = n * (n - 1)

    for i in range(n):
        for j in range(n):
            if i == j:
                continue
                
            log_message(f"Processando par {contador}/{total_pares}: {nomes[i]} → {nomes[j]}")
            contador += 1
            
            distancia, duracao = obter_rota_osrm(i, j, "driving")
            
            if distancia is not None and duracao is not None:
                matriz_distancias_carro[i][j] = distancia
                matriz_tempos_carro[i][j] = duracao
            
            time.sleep(0.5)  # Pausa para não sobrecarregar a API

    # Obter dados do Mapbox (tempos a pé)
    log_message("\n=== OBTENDO DADOS A PÉ (MAPBOX) ===")
    contador = 1

    for i in range(n):
        for j in range(n):
            if i == j:
                continue
                
            log_message(f"Processando par {contador}/{total_pares}: {nomes[i]} → {nomes[j]}")
            contador += 1
            
            distancia_pe, duracao_pe = obter_rota_mapbox(i, j, "walking")
            
            if distancia_pe is not None and duracao_pe is not None:
                # Armazenar valores originais
                matriz_distancias_pe_original[i][j] = distancia_pe
                matriz_tempos_pe_original[i][j] = duracao_pe
                
                # Preencher as matrizes normalmente por enquanto
                matriz_distancias_pe[i][j] = distancia_pe
                matriz_tempos_pe[i][j] = duracao_pe
            
            time.sleep(0.5)  # Pausa

    # CORREÇÃO: Após coletar todos os dados, garantir simetria nas matrizes a pé
    if usar_media_simetrica:
        log_message("\n=== GARANTINDO SIMETRIA NAS MATRIZES DE DISTÂNCIA A PÉ ===")
        for i in range(n):
            for j in range(i+1, n):  # Apenas para pares i,j onde i<j
                # Para distância a pé (deve ser simétrica)
                dist_ij = matriz_distancias_pe_original[i][j]
                dist_ji = matriz_distancias_pe_original[j][i]
                
                # Se ambos os valores foram obtidos, utilize a média
                if dist_ij > 0 and dist_ji > 0:
                    dist_media = int((dist_ij + dist_ji) / 2)
                    matriz_distancias_pe[i][j] = dist_media
                    matriz_distancias_pe[j][i] = dist_media
                    
                    # Registrar se houver discrepância significativa
                    if abs(dist_ij - dist_ji) > 500:  # 500 metros de diferença
                        log_message(f"Discrepância significativa na distância: {nomes[i]} ↔ {nomes[j]}: {dist_ij} vs {dist_ji} (diferença: {abs(dist_ij - dist_ji)}m)")
                
                # Para tempos a pé (podem variar devido a elevação, mas garantimos valores consistentes)
                tempo_ij = matriz_tempos_pe_original[i][j]
                tempo_ji = matriz_tempos_pe_original[j][i]
                
                # Opcionalmente, calcular a média para os tempos também
                # Isso garante que a regra de 15 minutos para caminhada seja aplicada consistentemente
                if tempo_ij > 0 and tempo_ji > 0:
                    if abs(tempo_ij - tempo_ji) > 5:  # 5 minutos de diferença
                        log_message(f"Discrepância significativa no tempo a pé: {nomes[i]} ↔ {nomes[j]}: {tempo_ij} vs {tempo_ji} (diferença: {abs(tempo_ij - tempo_ji)}min)")
                    
                    # Calcular média para os tempos
                    tempo_medio = int((tempo_ij + tempo_ji) / 2)
                    matriz_tempos_pe[i][j] = tempo_medio
                    matriz_tempos_pe[j][i] = tempo_medio
    
    return matriz_distancias_carro, matriz_tempos_carro, matriz_distancias_pe, matriz_tempos_pe

# Obter e preencher as matrizes
matriz_distancias_carro, matriz_tempos_carro, matriz_distancias_pe, matriz_tempos_pe = obter_e_preencher_matrizes(usar_media_simetrica=True)

# Salvar matrizes em CSV com formato brasileiro (ponto e vírgula como separador)
df_distancias_carro = pd.DataFrame(matriz_distancias_carro, index=nomes, columns=nomes)
df_tempos_carro = pd.DataFrame(matriz_tempos_carro, index=nomes, columns=nomes)
df_distancias_pe = pd.DataFrame(matriz_distancias_pe, index=nomes, columns=nomes)
df_tempos_pe = pd.DataFrame(matriz_tempos_pe, index=nomes, columns=nomes)

# Salvar com separador ponto e vírgula
df_distancias_carro.to_csv(car_dist_file, sep=';', decimal=',')
df_tempos_carro.to_csv(car_time_file, sep=';', decimal=',')
df_distancias_pe.to_csv(walk_dist_file, sep=';', decimal=',')
df_tempos_pe.to_csv(walk_time_file, sep=';', decimal=',')

# Criar cópias com timestamp para referência
df_distancias_carro.to_csv(f'matriz_distancias_carro_metros_{timestamp}.csv', sep=';', decimal=',')
df_tempos_carro.to_csv(f'matriz_tempos_carro_min_{timestamp}.csv', sep=';', decimal=',')
df_distancias_pe.to_csv(f'matriz_distancias_pe_metros_{timestamp}.csv', sep=';', decimal=',')
df_tempos_pe.to_csv(f'matriz_tempos_pe_min_{timestamp}.csv', sep=';', decimal=',')

# Gerar relatório resumido
log_message("\n=== FINALIZADO ===")
log_message(f"Arquivos gerados sem timestamp:")
log_message(f"- {car_dist_file} (OSRM)")
log_message(f"- {car_time_file} (OSRM)")
log_message(f"- {walk_dist_file} (Mapbox)")
log_message(f"- {walk_time_file} (Mapbox)")
log_message(f"Arquivos de backup com timestamp {timestamp} também foram criados")
log_message(f"Arquivo de log: {log_filename}")