import requests
import pandas as pd
import numpy as np
import time
from datetime import datetime

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

# Criar arquivo de log para acompanhar o processo
log_filename = f"osrm_requisicoes_log_{timestamp}.txt"

def log_message(message):
    """Registra mensagens em arquivo de log e no console"""
    with open(log_filename, "a", encoding="utf-8") as log_file:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_file.write(f"[{timestamp}] {message}\n")
    print(message)

log_message("Iniciando processo de obtenção de matrizes de distância e tempo")

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

# Inicializar matrizes
matriz_tempos_carro = np.zeros((n, n))
matriz_distancias = np.zeros((n, n))

log_message("\n=== CALCULANDO MATRIZ DE TEMPOS DE CARRO E DISTÂNCIAS ===")
for i in range(n):
    for j in range(n):
        if i == j:
            continue  # Diagonal principal é zero
            
        origem_lat, origem_lon = atracoes[i]["lat"], atracoes[i]["lon"]
        destino_lat, destino_lon = atracoes[j]["lat"], atracoes[j]["lon"]
        
        # Usando o formato documentado (longitude,latitude)
        url = f"http://router.project-osrm.org/route/v1/driving/{origem_lon},{origem_lat};{destino_lon},{destino_lat}?overview=false"
        
        log_message(f"Calculando rota de carro: {nomes[i]} → {nomes[j]}")
        
        try:
            response = requests.get(url)
            if response.status_code != 200:
                log_message(f"ERRO: API retornou código {response.status_code}")
                continue
                
            data = response.json()
            if data["code"] != "Ok":
                log_message(f"ERRO: API retornou código {data['code']}")
                continue
                
            # Extrair tempo e distância
            tempo = data["routes"][0]["duration"] / 60  # convertendo para minutos
            distancia = data["routes"][0]["distance"] / 1000  # convertendo para km
            
            matriz_tempos_carro[i][j] = tempo
            matriz_distancias[i][j] = distancia
            
            log_message(f"  Tempo: {tempo:.2f} min, Distância: {distancia:.2f} km")
            
        except Exception as e:
            log_message(f"ERRO na requisição: {str(e)}")
        
        # Pausa para não sobrecarregar a API
        time.sleep(0.5)

# Calcular matriz de tempos a pé com base nas distâncias (estimativa)
log_message("\n=== ESTIMANDO TEMPOS DE CAMINHADA ===")
velocidade_media_caminhada = 5.0  # km/h (velocidade média de uma pessoa caminhando)
matriz_tempos_pe = matriz_distancias / velocidade_media_caminhada * 60  # tempo em minutos

# Ajustar para terreno urbano e topografia do Rio (fator de 1.2 para subidas/descidas)
matriz_tempos_pe = matriz_tempos_pe * 1.2  

# Salvar matrizes em CSV
df_tempos_carro_min = pd.DataFrame(matriz_tempos_carro, index=nomes, columns=nomes)
df_tempos_carro_min.to_csv(f'matriz_tempos_carro_min_{timestamp}.csv')

df_tempos_pe_min = pd.DataFrame(matriz_tempos_pe, index=nomes, columns=nomes)
df_tempos_pe_min.to_csv(f'matriz_tempos_pe_min_{timestamp}.csv')

df_distancias_km = pd.DataFrame(matriz_distancias, index=nomes, columns=nomes)
df_distancias_km.to_csv(f'matriz_distancias_km_{timestamp}.csv')

# Imprimir amostras de cada matriz
log_message("\nAmostras de tempos de carro (em minutos):")
for i in range(min(3, n)):
    for j in range(min(3, n)):
        if i != j:
            log_message(f"  De {nomes[i]} para {nomes[j]}: {df_tempos_carro_min.iloc[i, j]:.2f} min")

log_message("\nAmostras de tempos a pé estimados (em minutos):")
for i in range(min(3, n)):
    for j in range(min(3, n)):
        if i != j:
            log_message(f"  De {nomes[i]} para {nomes[j]}: {df_tempos_pe_min.iloc[i, j]:.2f} min")

log_message("\nAmostras de distâncias (em km):")
for i in range(min(3, n)):
    for j in range(min(3, n)):
        if i != j:
            log_message(f"  De {nomes[i]} para {nomes[j]}: {df_distancias_km.iloc[i, j]:.2f} km")

# Verificar diferenças entre as matrizes
proporcao_media = np.mean(matriz_tempos_pe[matriz_tempos_carro > 0] / matriz_tempos_carro[matriz_tempos_carro > 0])
log_message(f"\nEm média, caminhar leva {proporcao_media:.1f}x mais tempo que dirigir.")

# Gerar um relatório resumido
log_message("\n=== RELATÓRIO RESUMIDO ===")
log_message(f"Arquivos gerados com timestamp {timestamp}:")
log_message(f"- matriz_tempos_carro_min_{timestamp}.csv")
log_message(f"- matriz_tempos_pe_min_{timestamp}.csv (estimativa)")
log_message(f"- matriz_distancias_km_{timestamp}.csv")
log_message(f"- Arquivo de log: {log_filename}")