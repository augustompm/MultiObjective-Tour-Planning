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
Corcovado - Cristo Redentor;-22.95112457318234,-43.21057352457689;120;241.54;480;1140
Jardim Botanico;-22.96352231913769,-43.22267669743484;180;15;480;1020
Praia de Ipanema;-22.98648190982438,-43.20668238449197;180;0;0;1439
Parque Lage;-22.957975657009477,-43.21163217653976;120;0;540;1020
Praia do Arpoador;-22.988371516628888,-43.19400661701697;180;0;0;1439
Maracana;-22.911925545881697,-43.23019601022307;240;116;540;1020
Praia da Barra da Tijuca;-23.01102228463985,-43.366251547561305;180;0;0;1439
Centro Cultural Banco do Brasil;-22.9008215829648,-43.17658567839236;180;0;540;1260
Museu do Amanhã;-22.894391989927207,-43.179644120721214;180;30;600;1080
AquaRio;-22.893263266025134,-43.19215237748509;120;75;540;1020
"""

# Registrar a hora para fins de log
timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
log_filename = f"rotas_turisticas_log_{timestamp}.txt"

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
        tuple: (distância em metros, duração em segundos) ou (None, None) se erro
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

# Inicializar matrizes
matriz_distancias_osrm = np.zeros((n, n), dtype=int)
matriz_tempos_carro_osrm = np.zeros((n, n), dtype=int)
matriz_distancias_pe_mapbox = np.zeros((n, n), dtype=int)
matriz_tempos_pe_mapbox = np.zeros((n, n), dtype=int)

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
            matriz_distancias_osrm[i][j] = distancia
            matriz_tempos_carro_osrm[i][j] = duracao
        
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
            matriz_distancias_pe_mapbox[i][j] = distancia_pe
            matriz_tempos_pe_mapbox[i][j] = duracao_pe
        
        time.sleep(0.5)  # Pausa

# Salvar matrizes em CSV com formato brasileiro (ponto e vírgula como separador)
df_distancias_osrm = pd.DataFrame(matriz_distancias_osrm, index=nomes, columns=nomes)
df_tempos_carro_osrm = pd.DataFrame(matriz_tempos_carro_osrm, index=nomes, columns=nomes)
df_distancias_pe_mapbox = pd.DataFrame(matriz_distancias_pe_mapbox, index=nomes, columns=nomes)
df_tempos_pe_mapbox = pd.DataFrame(matriz_tempos_pe_mapbox, index=nomes, columns=nomes)

# Salvar com separador ponto e vírgula
df_distancias_osrm.to_csv(f'matriz_distancias_carro_metros_{timestamp}.csv', sep=';', decimal=',')
df_tempos_carro_osrm.to_csv(f'matriz_tempos_carro_min_{timestamp}.csv', sep=';', decimal=',')
df_distancias_pe_mapbox.to_csv(f'matriz_distancias_pe_metros_{timestamp}.csv', sep=';', decimal=',')
df_tempos_pe_mapbox.to_csv(f'matriz_tempos_pe_min_{timestamp}.csv', sep=';', decimal=',')

# Gerar relatório detalhado da matriz de tempos e distâncias
log_message("\n=== RELATÓRIO DAS MATRIZES ===")
log_message("\nDistâncias de carro (OSRM) em metros:")
for i in range(n):
    for j in range(n):
        if i != j:
            log_message(f"{nomes[i]} → {nomes[j]}: {matriz_distancias_osrm[i][j]}")

log_message("\nTempos de carro (OSRM) em minutos:")
for i in range(n):
    for j in range(n):
        if i != j:
            log_message(f"{nomes[i]} → {nomes[j]}: {matriz_tempos_carro_osrm[i][j]}")

log_message("\nDistâncias a pé (Mapbox) em metros:")
for i in range(n):
    for j in range(n):
        if i != j:
            log_message(f"{nomes[i]} → {nomes[j]}: {matriz_distancias_pe_mapbox[i][j]}")

log_message("\nTempos a pé (Mapbox) em minutos:")
for i in range(n):
    for j in range(n):
        if i != j:
            log_message(f"{nomes[i]} → {nomes[j]}: {matriz_tempos_pe_mapbox[i][j]}")

# Gerar relatório resumido
log_message("\n=== FINALIZADO ===")
log_message(f"Arquivos gerados com timestamp {timestamp}:")
log_message(f"- matriz_distancias_carro_metros_{timestamp}.csv (OSRM)")
log_message(f"- matriz_tempos_carro_min_{timestamp}.csv (OSRM)")
log_message(f"- matriz_distancias_pe_metros_{timestamp}.csv (Mapbox)")
log_message(f"- matriz_tempos_pe_min_{timestamp}.csv (Mapbox)")
log_message(f"- Arquivo de log: {log_filename}")