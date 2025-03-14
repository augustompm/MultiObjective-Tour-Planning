import pandas as pd
from io import StringIO

# (1) DADOS DAS ATRAÇÕES, incluindo horários de funcionamento e tempo mínimo de visita
csv_atracoes = """# Nome;Bairro;Coordenadas;Tempo de Visita (min);Custo (R$);Horário de Abertura;Horário de Fechamento
Cristo Redentor;Cosme Velho;-22.95112457318234,-43.21057352457689;120;241.54;480;1140
Jardim Botanico;Jardim Botanico;-22.96352231913769,-43.22267669743484;180;15;480;1020
Praia de Ipanema;Ipanema;-22.98648190982438,-43.20668238449197;180;0;0;1439
Parque Lage;Jardim Botanico;-22.957975657009477,-43.21163217653976;120;0;540;1020
Praia do Arpoador;Arpoador;-22.988371516628888,-43.19400661701697;180;0;0;1439
Maracana;Maracana;-22.911925545881697,-43.23019601022307;240;116;540;1020
Praia da Barra da Tijuca;Barra da Tijuca;-23.01102228463985,-43.366251547561305;180;0;0;1439
Centro Cultural Banco do Brasil;Centro;-22.9008215829648,-43.17658567839236;180;0;540;1260
Museu do Amanha;Centro;-22.894391989927207,-43.179644120721214;180;30;600;1080
AquaRio;Gamboa;-22.893263266025134,-43.19215237748509;120;75;540;1020
Escadaria Selaron;Santa Teresa;-22.915144047171196,-43.17917141918494;60;0;540;1020
Forte Copacabana;Copacabana;-22.985874970153922,-43.187643519181584;120;0;600;1140
Santa Teresa;Santa Teresa;-22.917876964161398,-43.18234504375717;180;0;0;1439
Praia da Reserva;Barra da Tijuca;-23.011285357037575,-43.39445089281464;180;0;0;1439
Pedra da Gavea;Sao Conrado;-22.99597439272693,-43.28472266217296;180;0;0;1439
Pedra do Telegrafo;Guaratiba;-23.067969018064595,-43.55776410568555;180;0;0;1439
Prainha;Recreio;-23.041009699524924,-43.505679936107384;180;0;0;1439
Morro Dois Irmaos;Vidigal;-22.949411255405114,-43.400237546435;120;0;0;1439
Mosteiro De Sao Bento;Centro;-22.87216248487049,-43.17383914228873;60;0;450;1020
Mirante Dona Marta;Cosme Velho;-22.944790699789518,-43.19637404801938;180;0;0;1439
Theatro Municipal do Rio de Janeiro;Centro;-22.90879113172147,-43.17652531918517;180;844.37;600;1080
Parque Nacional da Tijuca;Tijuca;-22.940431934212032,-43.292708669579135;180;0;0;1439
Real Gabinete Portugues Da Leitura;Centro;-22.905221850616293,-43.18225103452901;120;0;600;1020
Praia do Grumari;Grumari;-23.0460357615992,-43.52638926202537;180;0;0;1439
Pedra Bonita;Sao Conrado;-22.985719223823942,-43.27938393838966;120;0;0;1439
Praia Da Joatinga;Joatinga;-23.01450718137832,-43.29039638082129;180;0;0;1439
Centro Cultural Jerusalem;Del Castilho;-22.880454262721617,-43.27342889035068;60;0;540;1080
Bondinho de Santa Teresa;Santa Teresa;-22.90915947846921,-43.1786614073549;60;0;480;1050
Ilha Da Gigoia;Barra da Tijuca;-23.001454164140785,-43.308482875801054;120;0;0;1439
Pedra do Sal;Saude;-22.89773169416836,-43.1852938191857;60;0;0;1439
Catedral Metropolitana de São Sebastião;Centro;-22.910476429715835,-43.180744819185215;60;0;420;1020
Vista Chinesa;Alto da Boa Vista;-22.97308519896875,-43.24942874801805;60;0;0;1439
Museu Historico Nacional;Centro;-22.905466691085117,-43.16957331918533;120;0;0;1439
Igreja de Sao Francisco da Penitência;Centro;-22.906428285929053,-43.17917332103655;120;0;0;1439
Parque das Ruinas;Santa Teresa;-22.91760459425989,-43.18234766336432;60;0;0;1439
Museu de Arte do Rio;Centro;-22.896398808250563,-43.181935048021664;180;30;540;1020
Mureta da Urca;Urca;-22.94275908887587,-43.16001580176129;120;0;0;1439
Quinta Da Boa Vista;Sao Cristovao;-22.904996521731277,-43.22180686336484;120;0;0;1439
Feira de Sao Cristovao;Sao Cristovao;-22.897493044468007,-43.220203705693464;180;10;600;1320
Biblioteca Nacional;Centro;-22.90791763107082,-43.17568936190707;120;0;600;1020
"""

df_atracoes = pd.read_csv(StringIO(csv_atracoes), sep=';', comment='#')
df_atracoes.columns = [
    "Nome", "Bairro", "Coordenadas", "TempoVisitaMin",
    "CustoIngresso", "HorarioAbertura", "HorarioFechamento"
]

# Cria um dicionário para acesso rápido pelas chaves (nome da atração)
# Ex: atracao_info["Cristo Redentor"] -> { TempoVisitaMin: 120, ... }
atracao_info = {}
for _, row in df_atracoes.iterrows():
    nome = row["Nome"].strip()
    atracao_info[nome] = {
        "tempo_visita": row["TempoVisitaMin"],
        "custo_ingresso": row["CustoIngresso"],
        "abre": row["HorarioAbertura"],    # em minutos após meia-noite (ex: 480 = 08:00)
        "fecha": row["HorarioFechamento"]  # idem
    }

# (2) DADOS DAS SOLUÇÕES (o seu CSV enorme com as 50 soluções)
# Aqui, como exemplo, leremos apenas parte dele em uma string,
# mas na prática você rodaria: df_solucoes = pd.read_csv("solucoes.csv", sep=';')
csv_solucoes = """Solucao;CustoTotal;TempoTotal;NumAtracoes;NumBairros;HoraInicio;HoraFim;Bairros;Sequencia;TemposChegada;TemposPartida;ModosTransporte
1;512.24;831.00;9;8;09:00;22:51;Sao Cristovao|Jardim Botanico|Saude|Santa Teresa|Centro|Alto da Boa Vista|Gamboa|Del Castilho|;Centro Cultural Jerusalem|Catedral Metropolitana de São Sebastião|Bondinho de Santa Teresa|Parque Lage|AquaRio|Pedra do Sal|Quinta Da Boa Vista|Vista Chinesa|Parque das Ruinas|;09:00|10:17|11:21|12:33|14:45|16:56|18:02|20:25|21:51|;10:00|11:17|12:21|14:33|16:45|17:56|20:02|21:25|22:51|;Car|Walk|Car|Car|Walk|Car|Car|Car|
2;314.92;795.00;9;7;09:00;22:15;Urca|Sao Cristovao|Saude|Santa Teresa|Centro|Gamboa|Del Castilho|;Centro Cultural Jerusalem|Catedral Metropolitana de São Sebastião|Bondinho de Santa Teresa|AquaRio|Pedra do Sal|Escadaria Selaron|Parque das Ruinas|Quinta Da Boa Vista|Mureta da Urca|;09:00|10:17|11:21|12:24|14:35|15:39|16:48|17:58|20:15|;10:00|11:17|12:21|14:24|15:35|16:39|17:48|19:58|22:15|;Car|Walk|Car|Walk|Car|Walk|Car|Car|
"""

df_solucoes = pd.read_csv(StringIO(csv_solucoes), sep=';')

# Se você tiver *todas* as 50 soluções, é só trocar:
# df_solucoes = pd.read_csv("arquivo_solucoes.csv", sep=';')

# (3) OPCIONAL: ler as matrizes de tempo/distância para validar transporte
# Por exemplo, se você tem "matriz_tempos_pe_min.csv" e "matriz_distancias_carro_metros.csv"
# e um mapeamento de "nome da atração" -> índice da matriz, etc.
# Vamos pular isso aqui e fazer apenas uma verificação *aproximada*
# a partir do tempo entre as partidas e chegadas, para ilustrar.

def hora_para_minutos(hhmm_str):
    """Converte 'HH:MM' em inteiro (minutos depois da meia-noite)."""
    h, m = hhmm_str.split(':')
    return int(h)*60 + int(m)

def checar_solucao(row):
    sol_id = row["Solucao"]
    tempo_total = row["TempoTotal"]
    
    # 1) Restrição de tempo diário
    if tempo_total > 840:
        return f"Solução {sol_id}: ERRO - TempoTotal > 840."
    
    chegadas = row["TemposChegada"].strip('|').split('|')
    partidas = row["TemposPartida"].strip('|').split('|')
    modos    = row["ModosTransporte"].strip('|').split('|')
    atracoes = row["Sequencia"].strip('|').split('|')
    
    # 2) Restrição de unicidade
    if len(atracoes) != len(set(atracoes)):
        return f"Solução {sol_id}: ERRO - Atração repetida no roteiro."
    
    # 3) Para cada atração i, checar:
    #    - (Partida[i] - Chegada[i]) >= TempoMínimo
    #    - Chegada[i] >= HorarioAbertura e Partida[i] <= HorarioFechamento
    for i, nome_atracao in enumerate(atracoes):
        if nome_atracao not in atracao_info:
            # Se alguma atração não consta no dicionário, ignore ou sinalize erro
            return f"Solução {sol_id}: ERRO - Atração '{nome_atracao}' não encontrada na base."
        
        # pega info do dicionário
        info = atracao_info[nome_atracao]
        chegada_i = hora_para_minutos(chegadas[i])
        partida_i = hora_para_minutos(partidas[i])
        
        tempo_visita_real = partida_i - chegada_i
        if tempo_visita_real < info["tempo_visita"]:
            return (f"Solução {sol_id}: ERRO - Atração '{nome_atracao}' "
                    f"não respeita tempo mínimo de {info['tempo_visita']} min.")
        
        # Verifica se está dentro do horário de funcionamento
        if chegada_i < info["abre"] or partida_i > info["fecha"]:
            return (f"Solução {sol_id}: ERRO - Atração '{nome_atracao}' visitada fora do horário "
                    f"({chegadas[i]}-{partidas[i]} vs abre {info['abre']} fecha {info['fecha']}).")
    
    # 4) Restrição de sequencialidade (sem sobreposição)
    #    Chegada na atração (i+1) >= Partida da atração i
    for i in range(len(atracoes) - 1):
        p_i = hora_para_minutos(partidas[i])
        c_i1 = hora_para_minutos(chegadas[i+1])
        if c_i1 < p_i:
            return f"Solução {sol_id}: ERRO - Horários sobrepostos (chega antes de sair)."
    
    # 5) Restrição de transporte (tempo de caminhada > 15 min é proibido)
    #    i.e. se Modo = 'Walk', então tempo de desloc. (Chegada i+1 - Partida i) <= 15
    for i in range(len(modos)):
        # Se tiver n atrações, normalmente há (n-1) deslocamentos
        # Mas seu CSV às vezes repete e no final fica '|' – portanto, cheque i < len(atracoes)-1
        if i < len(atracoes)-1:
            desloc = hora_para_minutos(chegadas[i+1]) - hora_para_minutos(partidas[i])
            if modos[i].lower() == 'walk':
                if desloc > 15:
                    return (f"Solução {sol_id}: ERRO - Caminhada de {desloc} min "
                            f"ultrapassa 15 min no trecho {atracoes[i]} -> {atracoes[i+1]}.")
    
    # 6) Restrição do custo de transporte (R$6/km) - verificação BÁSICA
    #    Precisaríamos cruzar distâncias REAIS por carro e somar. Aqui, mostraremos a lógica:
    #    - Some ingresso das atrações, se for parte do custo (isso depende do seu modelo)
    #    - Some custo de carro: Distancia(i->i+1)/1000 * 6
    #    Para realmente funcionar, teríamos que:
    #       * Ler "matriz_distancias_carro_metros.csv" em df_dist_carro
    #       * Mapear (atracao[i], atracao[i+1]) -> df_dist_carro[dist_metros]
    #    Abaixo, deixo *apenas* o esqueleto:

    # custo_calculado = 0.0
    # # a) soma custos de ingresso
    # for atr in atracoes:
    #     custo_calculado += atracao_info[atr]["custo_ingresso"]
    #
    # # b) soma custos de transporte
    # for i in range(len(atracoes)-1):
    #     if modos[i].lower() == 'car':
    #         nomeA = atracoes[i]
    #         nomeB = atracoes[i+1]
    #         # descobre a distância em metros no CSV (carro)
    #         # Ex: dist_metros = df_dist_carro.loc[nomeA, nomeB]
    #         dist_metros = 0  # <-- ler de uma matriz real
    #         custo_calculado += (dist_metros/1000) * 6
    #
    # # Ao final, comparar com CustoTotal
    # custo_informado = row["CustoTotal"]
    # # Se quiser tolerância, use algo como abs(custo_calculado - custo_informado) < 0.01
    # if round(custo_calculado, 2) != round(custo_informado, 2):
    #     return (f"Solução {sol_id}: ERRO - Custo de transporte/ingresso não bate "
    #             f"(calculado={custo_calculado:.2f}, planilha={custo_informado:.2f}).")

    # Se passou de todas as verificações, consideramos OK
    return f"Solução {sol_id}: OK."

# (4) Executar a checagem em cada linha
resultados = df_solucoes.apply(checar_solucao, axis=1)

# (5) Imprimir resultados
for r in resultados:
    print(r)
