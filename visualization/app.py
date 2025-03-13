#!/usr/bin/env python3
# visualization/app.py

import os
import pandas as pd
import numpy as np
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import dash
from dash import dcc, html, Input, Output, State, callback
import dash_bootstrap_components as dbc
from flask import Flask, render_template, jsonify, send_from_directory
import json
import math
import sys
import traceback

# Configuração de logging
import logging
logging.basicConfig(level=logging.DEBUG, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    handlers=[logging.StreamHandler()])
logger = logging.getLogger(__name__)

# File paths
RESULTS_FILE = "../build/resultados_nsga2_base.csv"
GENERATIONS_FILE = "../build/geracoes_nsga2_base.csv"

# Load data
def load_data():
    try:
        logger.info(f"Tentando carregar o arquivo de resultados: {RESULTS_FILE}")
        if not os.path.exists(RESULTS_FILE):
            logger.error(f"Arquivo não encontrado: {RESULTS_FILE}")
            logger.info("Procurando arquivo resultados_nsga2_base.csv no diretório atual...")
            
            current_dir_file = "resultados_nsga2_base.csv"
            if os.path.exists(current_dir_file):
                logger.info(f"Usando arquivo do diretório atual: {current_dir_file}")
                results_df = pd.read_csv(current_dir_file, sep=';', encoding='utf-8')
            else:
                logger.error("Arquivo de resultados não encontrado!")
                return pd.DataFrame(), pd.DataFrame()
        else:
            # Results data
            results_df = pd.read_csv(RESULTS_FILE, sep=';', encoding='utf-8')
        
        logger.info(f"Arquivo de resultados carregado com sucesso. Linhas: {len(results_df)}")
        
        # Mostrar colunas para debug
        logger.info(f"Colunas do arquivo de resultados: {results_df.columns.tolist()}")
        
        # Parse sequences, times, and transport modes
        for col, new_col in [
            ('Sequencia', 'AttracoesLista'), 
            ('TemposChegada', 'TemposChegadaLista'), 
            ('TemposPartida', 'TemposPartidaLista'), 
            ('ModosTransporte', 'ModosTransporteLista'), 
            ('Bairros', 'BairrosLista')
        ]:
            if col in results_df.columns:
                results_df[new_col] = results_df[col].apply(
                    lambda x: x.split('|') if isinstance(x, str) else [])
                
                # Filter empty elements
                results_df[new_col] = results_df[new_col].apply(lambda x: [i for i in x if i])
                logger.info(f"Coluna processada: {col} -> {new_col}")
            else:
                logger.warning(f"Coluna não encontrada no arquivo de resultados: {col}")
        
        # Generations data
        try:
            logger.info(f"Tentando carregar o arquivo de gerações: {GENERATIONS_FILE}")
            if not os.path.exists(GENERATIONS_FILE):
                logger.error(f"Arquivo não encontrado: {GENERATIONS_FILE}")
                logger.info("Procurando arquivo geracoes_nsga2_base.csv no diretório atual...")
                
                current_dir_file = "geracoes_nsga2_base.csv"
                if os.path.exists(current_dir_file):
                    logger.info(f"Usando arquivo do diretório atual: {current_dir_file}")
                    generations_df = pd.read_csv(current_dir_file, sep=';', encoding='utf-8')
                else:
                    logger.warning("Arquivo de gerações não encontrado!")
                    generations_df = pd.DataFrame(columns=['Generation', 'Front size', 'Best Cost', 'Best Time', 'Max Attractions'])
            else:
                generations_df = pd.read_csv(GENERATIONS_FILE, sep=';', encoding='utf-8')
            
            logger.info(f"Arquivo de gerações carregado com sucesso. Linhas: {len(generations_df)}")
            logger.info(f"Colunas do arquivo de gerações: {generations_df.columns.tolist()}")
            
        except Exception as e:
            logger.error(f"Erro ao processar o arquivo de gerações: {e}")
            logger.error(traceback.format_exc())
            generations_df = pd.DataFrame(columns=['Generation', 'Front size', 'Best Cost', 'Best Time', 'Max Attractions'])
        
        return results_df, generations_df
    
    except Exception as e:
        logger.error(f"Erro ao carregar dados: {e}")
        logger.error(traceback.format_exc())
        # Return empty dataframes as fallback
        return pd.DataFrame(), pd.DataFrame()

# Prepare data for parallel coordinates plot
def prepare_parallel_data(results_df):
    try:
        if results_df.empty:
            logger.warning("DataFrame de resultados vazio. Não é possível preparar dados para coordenadas paralelas.")
            return pd.DataFrame()
        
        logger.info("Preparando dados para o gráfico de coordenadas paralelas")
        
        # Extract the four objectives
        required_cols = ['CustoTotal', 'TempoTotal', 'NumAtracoes', 'NumBairros']
        missing_cols = [col for col in required_cols if col not in results_df.columns]
        
        if missing_cols:
            logger.error(f"Colunas faltando no DataFrame: {missing_cols}")
            logger.info(f"Colunas disponíveis: {results_df.columns.tolist()}")
            # Create empty columns for missing data
            for col in missing_cols:
                results_df[col] = 0
        
        # Add filtering to remove very similar solutions
        filtered_data = []
        seen_objectives = set()
        tolerance = 0.01  # Adjust this value to control similarity tolerance

        for _, row in results_df.iterrows():
            obj_tuple = tuple([round(row[col]/tolerance)*tolerance for col in required_cols])
            if obj_tuple not in seen_objectives:
                filtered_data.append(row)
                seen_objectives.add(obj_tuple)
        
        # Create DataFrame from filtered data
        filtered_df = pd.DataFrame(filtered_data) if filtered_data else results_df
        
        obj_data = filtered_df[required_cols].copy()
        
        # Create a copy for normalized values
        norm_data = obj_data.copy()
        
        # Normalize data for visualization
        for col in norm_data.columns:
            if col in ['NumAtracoes', 'NumBairros']:
                # For maximization objectives, negate values for display
                max_val = obj_data[col].max()
                min_val = obj_data[col].min()
                if max_val != min_val:
                    norm_data[col] = 1 - ((obj_data[col] - min_val) / (max_val - min_val))
                else:
                    norm_data[col] = 0.5  # Default value if all values are the same
            else:
                # For minimization objectives, normalize directly
                max_val = obj_data[col].max()
                min_val = obj_data[col].min()
                if max_val != min_val:
                    norm_data[col] = (obj_data[col] - min_val) / (max_val - min_val)
                else:
                    norm_data[col] = 0.5  # Default value if all values are the same
        
        # Add solution index
        norm_data['SolutionID'] = filtered_df.index + 1
        
        logger.info(f"Dados de coordenadas paralelas preparados com sucesso. Linhas: {len(norm_data)}")
        return norm_data
        
    except Exception as e:
        logger.error(f"Erro ao preparar dados para coordenadas paralelas: {e}")
        logger.error(traceback.format_exc())
        return pd.DataFrame()

# Prepare data for linked arcs visualization
def prepare_linked_arcs_data(results_df):
    try:
        if results_df.empty:
            logger.warning("DataFrame de resultados vazio. Não é possível preparar dados para LinkedArcs.")
            return []
        
        logger.info("Preparando dados para a visualização LinkedArcs")
        
        # Extract objectives
        objectives = ['CustoTotal', 'TempoTotal', 'NumAtracoes', 'NumBairros']
        missing_cols = [col for col in objectives if col not in results_df.columns]
        
        if missing_cols:
            logger.error(f"Colunas faltando no DataFrame: {missing_cols}")
            # Create empty columns for missing data
            for col in missing_cols:
                results_df[col] = 0
        
        # Prepare data for the linked arcs visualization
        data = []
        for i, row in results_df.iterrows():
            solution = {}
            for j, obj in enumerate(objectives):
                # For NumAtracoes and NumBairros (which are to be maximized),
                # we negate the values for the visualization
                if obj in ['NumAtracoes', 'NumBairros']:
                    solution[f'Obj{j+1}'] = -row[obj]
                else:
                    solution[f'Obj{j+1}'] = row[obj]
            data.append(solution)
        
        logger.info(f"Dados LinkedArcs preparados com sucesso. Soluções: {len(data)}")
        return data
        
    except Exception as e:
        logger.error(f"Erro ao preparar dados para LinkedArcs: {e}")
        logger.error(traceback.format_exc())
        return []

# Initialize Flask server
server = Flask(__name__, static_folder='static', template_folder='templates')

# Create Dash app
app = dash.Dash(__name__, 
                server=server, 
                external_stylesheets=[dbc.themes.BOOTSTRAP, 'https://use.fontawesome.com/releases/v5.8.1/css/all.css'],
                assets_folder='assets',
                meta_tags=[
                    {"name": "viewport", "content": "width=device-width, initial-scale=1"}
                ])

# Load data
logger.info("Carregando dados...")
results_df, generations_df = load_data()
parallel_data = prepare_parallel_data(results_df)
linked_arcs_data = prepare_linked_arcs_data(results_df)

# Update app layout to add D3.js script at the top
app.layout = dbc.Container([
    # Add D3.js at the top of the document with a specific version
    html.Script(src="https://d3js.org/d3.v7.min.js", id="d3-script"),
    
    dbc.Row([
        dbc.Col([
            html.H1("Resultados da Otimização Multi-objetivo NSGA-II", 
                    className="text-center text-primary my-4"),
            html.Hr(),
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Visualização das Soluções - Coordenadas Paralelas", className="text-center"),
            dcc.Graph(id='parallel-coords-plot'),
            
            # Replace the static text with a collapsible component
            html.Div([
                dbc.Button(
                    [html.I(className="fas fa-info-circle mr-1"), "Sobre este gráfico"],
                    id="parallel-info-button",
                    color="link",
                    className="mt-2"
                ),
                dbc.Collapse(
                    dbc.Card(
                        dbc.CardBody([
                            html.P([
                                "Este gráfico mostra as soluções não-dominadas encontradas pelo NSGA-II usando coordenadas paralelas. ",
                                "Cada linha representa uma solução diferente, com: "
                            ]),
                            html.Ul([
                                html.Li("Custo Total (R$) - quanto menor, melhor"),
                                html.Li("Tempo Total (minutos) - quanto menor, melhor"),
                                html.Li("Número de Atrações - quanto mais atrações, melhor"),
                                html.Li("Número de Bairros - quanto mais bairros, melhor")
                            ]),
                            html.P("A linha destacada em vermelho representa a solução selecionada atualmente.")
                        ]),
                        className="mt-2"
                    ),
                    id="parallel-info-collapse",
                    is_open=False,
                ),
            ], className="px-4"),
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Explorar Solução Individual", className="text-center"),
            html.Div([
                html.Div(id='solution-dial-container', className="d-flex justify-content-center my-3"),
                dcc.Slider(
                    id='solution-slider',
                    min=1,
                    max=len(results_df) if not results_df.empty else 1,
                    step=1,
                    value=1,
                    marks={i: f'Sol. {i}' for i in range(1, len(results_df)+1, max(1, len(results_df)//10))} if not results_df.empty else {1: 'Sol. 1'},
                ),
            ], className="my-3"),
        ], width=12, className="my-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.Div(id='solution-details', className="p-3 border rounded")
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Itinerário da Solução", className="text-center"),
            html.Div(id='solution-timeline', className="p-3 border rounded")
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Evolução das Gerações", className="text-center"),
            dbc.Tabs([
                dbc.Tab([
                    dcc.Graph(id='front-size-plot')
                ], label="Tamanho da Fronteira"),
                dbc.Tab([
                    dcc.Graph(id='best-cost-plot')
                ], label="Melhor Custo"),
                dbc.Tab([
                    dcc.Graph(id='best-time-plot')
                ], label="Melhor Tempo"),
                dbc.Tab([
                    dcc.Graph(id='max-attractions-plot')
                ], label="Máximo de Atrações"),
            ])
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Visualização de Linked Arcs", className="text-center mb-4"),
            html.Iframe(
                id='linked-arcs-iframe',
                src='/linked-arcs',
                style={'width': '100%', 'height': '600px', 'border': 'none'}
            )
        ], width=12, className="mb-4")
    ]),
    
    # Add D3.js library for the solution dial
    html.Script(src="https://d3js.org/d3.v3.min.js"),
    
    # Add solution dial script
    html.Script(id='solution-dial-script'),
    
    dbc.Row([
        dbc.Col([
            html.Footer([
                html.P("Visualização desenvolvida para Otimização Multiobjetivo de Roteiros Turísticos", 
                       className="text-center text-muted")
            ])
        ], width=12)
    ])
], fluid=True, className="my-4")

# Callback for solution dial script
@app.callback(
    Output('solution-dial-script', 'children'),
    Input('solution-slider', 'value')  # Dummy input to ensure the callback runs at least once
)
def provide_solution_dial_script(_):
    logger.info("Carregando script do solution dial")
    try:
        with open("assets/js/solution_dial.js", "r") as f:
            script_content = f.read()
            logger.info("Script solution_dial.js carregado com sucesso")
            return script_content
    except Exception as e:
        logger.error(f"Erro ao carregar solution_dial.js: {e}")
        logger.error(traceback.format_exc())
        # Create a minimal version of the dial script if file doesn't exist
        return """
        function createSolutionDial(containerId, options) {
            console.log("Solution dial script loaded");
            const el = document.getElementById(containerId);
            if (el) {
                el.innerHTML = `<div style="text-align:center; margin-top: 20px">
                    <h4>Solução #${options.value}</h4>
                    <p>Use o slider abaixo para navegar entre as soluções</p>
                </div>`;
            }
            return {
                setValue: function(val) {
                    if (el) {
                        el.innerHTML = `<div style="text-align:center; margin-top: 20px">
                            <h4>Solução #${val}</h4>
                            <p>Use o slider abaixo para navegar entre as soluções</p>
                        </div>`;
                    }
                }
            };
        }
        """

# Callback for parallel coordinates plot
@app.callback(
    Output('parallel-coords-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_parallel_coords(selected_solution):
    logger.info(f"Atualizando gráfico de coordenadas paralelas para solução {selected_solution}")
    try:
        if parallel_data.empty:
            logger.warning("Dados de coordenadas paralelas vazios")
            return go.Figure()
        
        # Criar uma cópia dos dados para manipulação
        display_data = parallel_data.copy()
        
        # Ajustar a escala de cores para destacar a solução selecionada
        selected_idx = selected_solution - 1
        color_values = np.ones(len(display_data)) * 0.3  # Valor base para todas as soluções
        
        if 0 <= selected_idx < len(display_data):
            color_values[selected_idx] = 1.0  # Valor para solução selecionada
        
        # Extrair os limites corretos para os rótulos dos eixos
        cost_min = results_df['CustoTotal'].min()
        cost_max = results_df['CustoTotal'].max()
        
        time_min = results_df['TempoTotal'].min()
        time_max = results_df['TempoTotal'].max()
        
        attractions_min = results_df['NumAtracoes'].min()
        attractions_max = results_df['NumAtracoes'].max()
        
        neighborhoods_min = results_df['NumBairros'].min()
        neighborhoods_max = results_df['NumBairros'].max()
        
        # Criar o gráfico de coordenadas paralelas com dimensões corretamente formatadas
        fig = go.Figure(data=
            go.Parcoords(
                line=dict(
                    color=color_values,
                    colorscale=[[0, 'rgba(70, 130, 180, 0.3)'], [1, 'rgba(220, 20, 60, 1.0)']],
                    showscale=False
                ),
                dimensions=[
                    dict(
                        range=[0, 1],
                        label='Custo Total (R$)',
                        values=display_data['CustoTotal'],
                        tickvals=[0, 0.25, 0.5, 0.75, 1],
                        ticktext=[f"R$ {cost_max:.0f}", f"R$ {cost_max - (cost_max-cost_min)*0.25:.0f}", 
                                f"R$ {cost_max - (cost_max-cost_min)*0.5:.0f}", 
                                f"R$ {cost_max - (cost_max-cost_min)*0.75:.0f}", f"R$ {cost_min:.0f}"]
                    ),
                    dict(
                        range=[0, 1],
                        label='Tempo Total (min)',
                        values=display_data['TempoTotal'],
                        tickvals=[0, 0.25, 0.5, 0.75, 1],
                        ticktext=[f"{time_max:.0f}", f"{time_max - (time_max-time_min)*0.25:.0f}", 
                                f"{time_max - (time_max-time_min)*0.5:.0f}", 
                                f"{time_max - (time_max-time_min)*0.75:.0f}", f"{time_min:.0f}"]
                    ),
                    dict(
                        range=[0, 1],
                        label='Atrações',
                        values=1 - display_data['NumAtracoes'],  # Inverter para maximização
                        tickvals=[0, 0.25, 0.5, 0.75, 1],
                        ticktext=[f"{attractions_max:.0f}", f"{attractions_max - (attractions_max-attractions_min)*0.25:.0f}", 
                                f"{attractions_max - (attractions_max-attractions_min)*0.5:.0f}", 
                                f"{attractions_max - (attractions_max-attractions_min)*0.75:.0f}", f"{attractions_min:.0f}"]
                    ),
                    dict(
                        range=[0, 1],
                        label='Bairros',
                        values=1 - display_data['NumBairros'],  # Inverter para maximização
                        tickvals=[0, 0.25, 0.5, 0.75, 1],
                        ticktext=[f"{neighborhoods_max:.0f}", f"{neighborhoods_max - (neighborhoods_max-neighborhoods_min)*0.25:.0f}", 
                                f"{neighborhoods_max - (neighborhoods_max-neighborhoods_min)*0.5:.0f}", 
                                f"{neighborhoods_max - (neighborhoods_max-neighborhoods_min)*0.75:.0f}", f"{neighborhoods_min:.0f}"]
                    )
                ]
            )
        )
        
        # Melhorar layout do gráfico
        fig.update_layout(
            margin=dict(l=100, r=100, t=100, b=80),  # Increase top margin
            height=600,
            title={
                'text': "Visualização de Coordenadas Paralelas das Soluções Não-dominadas",
                'y': 0.98,  # Move title higher
                'x': 0.5,
                'xanchor': 'center',
                'yanchor': 'top',
                'font': dict(size=18)
            },
            font=dict(size=14),
            paper_bgcolor='rgba(255,255,255,0.9)',
            plot_bgcolor='rgba(255,255,255,0.9)'
        )
        
        return fig
    except Exception as e:
        logger.error(f"Erro ao atualizar gráfico de coordenadas paralelas: {e}")
        logger.error(traceback.format_exc())
        # Return empty figure in case of error
        return go.Figure()

# Callback for solution details
@app.callback(
    Output('solution-details', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_details(selected_solution):
    logger.info(f"Atualizando detalhes da solução {selected_solution}")
    try:
        if results_df.empty:
            logger.warning("DataFrame de resultados vazio")
            return html.P("Dados não disponíveis")
        
        idx = selected_solution - 1
        if idx < 0 or idx >= len(results_df):
            logger.warning(f"Índice de solução inválido: {idx}")
            return html.P("Solução não encontrada")
        
        solution = results_df.iloc[idx]
        
        # Check if required columns exist
        required_cols = ['CustoTotal', 'TempoTotal', 'NumAtracoes', 'NumBairros', 'HoraInicio', 'HoraFim', 'BairrosLista']
        missing_cols = []
        
        for col in required_cols:
            if col not in solution.index and col not in solution:
                missing_cols.append(col)
        
        if missing_cols:
            logger.warning(f"Colunas faltando nos detalhes da solução: {missing_cols}")
        
        return dbc.Row([
            dbc.Col([
                html.H4(f"Solução #{selected_solution}", className="text-primary"),
                html.P([
                    html.Strong("Custo Total: "), f"R$ {solution.get('CustoTotal', 0):.2f}"
                ]),
                html.P([
                    html.Strong("Tempo Total: "), f"{solution.get('TempoTotal', 0):.1f} minutos"
                ]),
                html.P([
                    html.Strong("Duração: "), 
                    f"{solution.get('HoraInicio', '00:00')} - {solution.get('HoraFim', '00:00')}"
                ]),
            ], width=6),
            dbc.Col([
                html.P([
                    html.Strong("Atrações Visitadas: "), f"{solution.get('NumAtracoes', 0)}"
                ]),
                html.P([
                    html.Strong("Bairros Visitados: "), f"{solution.get('NumBairros', 0)}"
                ]),
                html.P([
                    html.Strong("Bairros: "), 
                    ", ".join(solution.get('BairrosLista', [])) if hasattr(solution, 'BairrosLista') else ""
                ]),
            ], width=6),
        ])
    except Exception as e:
        logger.error(f"Erro ao atualizar detalhes da solução: {e}")
        logger.error(traceback.format_exc())
        return html.P("Erro ao carregar detalhes da solução")

# Callback for solution timeline
@app.callback(
    Output('solution-timeline', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_timeline(selected_solution):
    logger.info(f"Atualizando timeline da solução {selected_solution}")
    try:
        if results_df.empty:
            logger.warning("DataFrame de resultados vazio")
            return html.P("Dados não disponíveis")
        
        idx = selected_solution - 1
        if idx < 0 or idx >= len(results_df):
            logger.warning(f"Índice de solução inválido: {idx}")
            return html.P("Solução não encontrada")
        
        solution = results_df.iloc[idx]
        
        # Check if required columns exist
        required_cols = ['AttracoesLista', 'TemposChegadaLista', 'TemposPartidaLista', 'ModosTransporteLista']
        missing_cols = []
        
        for col in required_cols:
            if col not in solution.index and col not in solution:
                missing_cols.append(col)
        
        if missing_cols:
            logger.warning(f"Colunas faltando no timeline da solução: {missing_cols}")
            return html.P("Dados incompletos para mostrar o itinerário")
        
        attractions = solution.get('AttracoesLista', [])
        arrival_times = solution.get('TemposChegadaLista', [])
        departure_times = solution.get('TemposPartidaLista', [])
        transport_modes = solution.get('ModosTransporteLista', [])
        
        timeline_items = []
        
        for i, attr in enumerate(attractions):
            if i < len(arrival_times) and i < len(departure_times):
                arrival = arrival_times[i]
                departure = departure_times[i]
                
                # Add transportation info if not the first attraction
                if i > 0 and i-1 < len(transport_modes):
                    transport_mode = transport_modes[i-1]
                    transport_icon = "🚶" if transport_mode == "Walk" else "🚗"
                    
                    timeline_items.append(dbc.Row([
                        dbc.Col([
                            html.Div(transport_icon, className="transport-icon text-center h3")
                        ], width=1),
                        dbc.Col([
                            html.P([
                                html.Strong(f"Deslocamento: "), f"{transport_mode}",
                                html.Br(),
                                f"De {attractions[i-1]} para {attr}"
                            ], className="mb-1")
                        ], width=11)
                    ], className="py-2 border-bottom"))
                
                # Add attraction info
                timeline_items.append(dbc.Row([
                    dbc.Col([
                        html.Div("🏛️", className="attraction-icon text-center h3")
                    ], width=1),
                    dbc.Col([
                        html.H5(attr, className="text-primary mb-0"),
                        html.P([
                            html.Strong("Chegada: "), arrival,
                            html.Strong(" | Saída: "), departure,
                        ], className="mb-1")
                    ], width=11)
                ], className="py-2 border-bottom"))
        
        if not timeline_items:
            return html.P("Nenhum item no itinerário")
            
        return dbc.Container(timeline_items, className="p-0")
    except Exception as e:
        logger.error(f"Erro ao atualizar timeline da solução: {e}")
        logger.error(traceback.format_exc())
        return html.P("Erro ao carregar itinerário da solução")

# Callbacks for the generation plots
@app.callback(
    Output('front-size-plot', 'figure'),
    Input('solution-slider', 'value')  # Dummy input to ensure the callback runs
)
def update_front_size_plot(_):
    logger.info("Atualizando gráfico de tamanho da fronteira")
    try:
        if generations_df.empty or 'Front size' not in generations_df.columns:
            logger.warning("Dados de gerações vazios ou coluna 'Front size' não encontrada")
            return go.Figure()
        
        # Make sure we have numeric Generation values
        df_plot = generations_df.copy()
        if df_plot['Generation'].dtype == 'object':
            # Try to convert to numeric, ignore errors
            df_plot['Generation'] = pd.to_numeric(df_plot['Generation'], errors='coerce')
            # Drop rows with NaN values
            df_plot = df_plot.dropna(subset=['Generation'])
        
        # Sort by generation
        df_plot = df_plot.sort_values('Generation')
        
        fig = px.line(df_plot, x='Generation', y='Front size',
                     labels={'Generation': 'Geração', 'Front size': 'Tamanho da Fronteira'},
                     template='plotly_white')
        
        fig.update_layout(
            title='Evolução do Tamanho da Fronteira Não-dominada',
            xaxis_title='Geração',
            yaxis_title='Número de Soluções',
            hovermode='closest'
        )
        
        # Ensure the x-axis shows integers for generations
        fig.update_xaxes(type='category' if len(df_plot) < 30 else 'linear', 
                        tickmode='linear' if len(df_plot) < 30 else 'auto',
                        dtick=5 if len(df_plot) < 30 else None)
        
        return fig
    except Exception as e:
        logger.error(f"Erro ao atualizar gráfico de tamanho da fronteira: {e}")
        logger.error(traceback.format_exc())
        return go.Figure()

@app.callback(
    Output('best-cost-plot', 'figure'),
    Input('solution-slider', 'value')  # Dummy input
)
def update_best_cost_plot(_):
    logger.info("Atualizando gráfico de melhor custo")
    try:
        if generations_df.empty or 'Best Cost' not in generations_df.columns:
            logger.warning("Dados de gerações vazios ou coluna 'Best Cost' não encontrada")
            return go.Figure()
        
        df_plot = generations_df.copy()
        if df_plot['Generation'].dtype == 'object':
            df_plot['Generation'] = pd.to_numeric(df_plot['Generation'], errors='coerce')
            df_plot = df_plot.dropna(subset=['Generation'])
        
        df_plot = df_plot.sort_values('Generation')
        
        fig = px.line(df_plot, x='Generation', y='Best Cost',
                     labels={'Generation': 'Geração', 'Best Cost': 'Melhor Custo (R$)'},
                     template='plotly_white')
        
        fig.update_layout(
            title='Evolução do Melhor Custo',
            xaxis_title='Geração',
            yaxis_title='Custo (R$)',
            hovermode='closest'
        )
        
        fig.update_xaxes(type='category' if len(df_plot) < 30 else 'linear',
                        tickmode='linear' if len(df_plot) < 30 else 'auto',
                        dtick=5 if len(df_plot) < 30 else None)
        
        return fig
    except Exception as e:
        logger.error(f"Erro ao atualizar gráfico de melhor custo: {e}")
        logger.error(traceback.format_exc())
        return go.Figure()

@app.callback(
    Output('max-attractions-plot', 'figure'),
    Input('solution-slider', 'value')  # Dummy input
)
def update_max_attractions_plot(_):
    logger.info("Atualizando gráfico de máximo de atrações")
    try:
        if generations_df.empty or 'Max Attractions' not in generations_df.columns:
            logger.warning("Dados de gerações vazios ou coluna 'Max Attractions' não encontrada")
            return go.Figure()
        
        df_plot = generations_df.copy()
        if df_plot['Generation'].dtype == 'object':
            df_plot['Generation'] = pd.to_numeric(df_plot['Generation'], errors='coerce')
            df_plot = df_plot.dropna(subset(['Generation']))
        
        df_plot = df_plot.sort_values('Generation')
        
        fig = px.line(df_plot, x='Generation', y='Max Attractions',
                     labels={'Generation': 'Geração', 'Max Attractions': 'Máximo de Atrações'},
                     template='plotly_white')
        
        fig.update_layout(
            title='Evolução do Número Máximo de Atrações',
            xaxis_title='Geração',
            yaxis_title='Número de Atrações',
            hovermode='closest'
        )
        
        fig.update_xaxes(type='category' if len(df_plot) < 30 else 'linear',
                        tickmode='linear' if len(df_plot) < 30 else 'auto',
                        dtick=5 if len(df_plot) < 30 else None)
        
        return fig
    except Exception as e:
        logger.error(f"Erro ao atualizar gráfico de máximo de atrações: {e}")
        logger.error(traceback.format_exc())
        return go.Figure()

# Callback for solution dial
app.clientside_callback(
    """
    function(value, max_value) {
        if (!window.solutionDial && document.getElementById('solution-dial-container')) {
            // Create the solution dial component when the container is ready
            window.solutionDial = createSolutionDial('solution-dial-container', {
                radius: 80,
                width: 200,
                height: 200,
                maxValue: max_value,
                value: value,
                onChange: function(newValue) {
                    // Update the slider value
                    document.getElementById('solution-slider').value = newValue;
                    // Trigger the slider's change event
                    const event = new Event('change');
                    document.getElementById('solution-slider').dispatchEvent(event);
                }
            });
            
            return window.dash_clientside.no_update;
        } else if (window.solutionDial) {
            // Update the dial when the slider changes
            window.solutionDial.setValue(value);
            return window.dash_clientside.no_update;
        }
        return window.dash_clientside.no_update;
    }
    """,
    Output('solution-dial-container', 'children'),
    Input('solution-slider', 'value'),
    State('solution-slider', 'max')
)

# Routes for LinkedArcs visualization
@server.route('/linked-arcs')
def linked_arcs():
    logger.info("Rota /linked-arcs acessada")
    return render_template('linked_arcs.html')

@server.route('/data.json')
def get_data():
    logger.info("Rota /data.json acessada")
    return jsonify(linked_arcs_data)

@server.route('/assets/<path:path>')
def serve_static(path):
    logger.info(f"Acessando arquivo estático: /assets/{path}")
    return send_from_directory('assets', path)

@server.route('/static/<path:path>')
def serve_static_files(path):
    logger.info(f"Acessando arquivo estático: /static/{path}")
    return send_from_directory('static', path)

if __name__ == '__main__':
    # Create necessary directories if they don't exist
    for dir_path in ['assets/js', 'templates', 'static/js']:
        os.makedirs(dir_path, exist_ok=True)
        logger.info(f"Diretório criado ou verificado: {dir_path}")
    
    # Ensure the solution_dial.js file exists
    solution_dial_path = 'assets/js/solution_dial.js'
    if not os.path.exists(solution_dial_path):
        logger.info(f"Criando arquivo: {solution_dial_path}")
        with open(solution_dial_path, 'w') as f:
            f.write('''// solution_dial.js - Circular dial for solution selection

function createSolutionDial(containerId, options) {
    const defaults = {
        radius: 100,
        width: 250,
        height: 250,
        maxValue: 100,
        value: 1,
        onChange: null
    };
    
    const settings = Object.assign({}, defaults, options);
    
    // Create SVG element
    const svg = d3.select(`#${containerId}`)
        .append('svg')
        .attr('width', settings.width)
        .attr('height', settings.height);
    
    const centerX = settings.width / 2;
    const centerY = settings.height / 2;
    
    // Draw the outer circle
    svg.append('circle')
        .attr('cx', centerX)
        .attr('cy', centerY)
        .attr('r', settings.radius)
        .attr('fill', 'none')
        .attr('stroke', '#ddd')
        .attr('stroke-width', 10);
    
    // Draw tick marks
    const tickInterval = Math.max(1, Math.floor(settings.maxValue / 20));
    for (let i = 1; i <= settings.maxValue; i++) {
        if (i % tickInterval === 0 || i === 1 || i === settings.maxValue) {
            const angle = (i / settings.maxValue) * 2 * Math.PI - Math.PI / 2;
            const x1 = centerX + (settings.radius - 5) * Math.cos(angle);
            const y1 = centerY + (settings.radius - 5) * Math.sin(angle);
            const x2 = centerX + (settings.radius + 5) * Math.cos(angle);
            const y2 = centerY + (settings.radius + 5) * Math.sin(angle);
            
            svg.append('line')
                .attr('x1', x1)
                .attr('y1', y1)
                .attr('x2', x2)
                .attr('y2', y2)
                .attr('stroke', '#999')
                .attr('stroke-width', 2);
            
            // Add labels for some ticks
            if (i % (tickInterval * 4) === 0 || i === 1 || i === settings.maxValue) {
                const labelX = centerX + (settings.radius + 20) * Math.cos(angle);
                const labelY = centerY + (settings.radius + 20) * Math.sin(angle);
                
                svg.append('text')
                    .attr('x', labelX)
                    .attr('y', labelY)
                    .attr('text-anchor', 'middle')
                    .attr('alignment-baseline', 'middle')
                    .attr('font-size', '12px')
                    .text(i);
            }
        }
    }
    
    // Create the needle
    const needle = svg.append('line')
        .attr('x1', centerX)
        .attr('y1', centerY)
        .attr('x2', centerX)
        .attr('y2', centerY - settings.radius)
        .attr('stroke', '#e74c3c')
        .attr('stroke-width', 3)
        .attr('stroke-linecap', 'round');
    
    // Create the center circle
    svg.append('circle')
        .attr('cx', centerX)
        .attr('cy', centerY)
        .attr('r', 10)
        .attr('fill', '#e74c3c');
    
    // Add value text
    const valueText = svg.append('text')
        .attr('x', centerX)
        .attr('y', centerY + settings.radius / 2)
        .attr('text-anchor', 'middle')
        .attr('font-size', '24px')
        .attr('font-weight', 'bold')
        .text(`${settings.value}`);
    
    // Function to update the dial
    function updateDial(value) {
        if (value < 1) value = 1;
        if (value > settings.maxValue) value = settings.maxValue;
        
        const angle = (value / settings.maxValue) * 2 * Math.PI - Math.PI / 2;
        const x2 = centerX + settings.radius * Math.cos(angle);
        const y2 = centerY + settings.radius * Math.sin(angle);
        
        needle
            .transition()
            .duration(300)
            .attr('x2', x2)
            .attr('y2', y2);
        
        valueText
            .transition()
            .duration(300)
            .text(`${value}`);
        
        if (settings.onChange) {
            settings.onChange(value);
        }
    }
    
    // Initialize the dial
    updateDial(settings.value);
    
    // Make the dial interactive
    const dragBehavior = d3.behavior.drag()
        .on('drag', function() {
            const mouseX = d3.event.x - centerX;
            const mouseY = d3.event.y - centerY;
            
            // Calculate angle from center to mouse position
            let angle = Math.atan2(mouseY, mouseX) + Math.PI / 2;
            if (angle < 0) angle += 2 * Math.PI;
            
            // Convert angle to value
            let value = Math.round((angle / (2 * Math.PI)) * settings.maxValue);
            if (value === 0) value = settings.maxValue;
            
            updateDial(value);
        });
    
    svg.call(dragBehavior);
    
    // Return the public API
    return {
        setValue: updateDial,
        getValue: function() {
            return settings.value;
        }
    };
}''')
    else:
        logger.info(f"Arquivo já existe: {solution_dial_path}")

    # Ensure the linkedArc.js file exists in the static folder
    linkedarc_js_path = 'static/js/linkedArc.js'
    if not os.path.exists(linkedarc_js_path):
        logger.info(f"Criando arquivo: {linkedarc_js_path}")
        with open(linkedarc_js_path, 'w') as f:
            f.write('''// File: visualization/static/js/linkedArc.js

var dim; // the size of dimension of problem
var doc; // Document size
var minObj = [], maxObj = []; // min and max of Objectives
var intervals = []; // The function who calculates and map the values on the arcs
var Data = [];
var selectingPoints = []; // for frame of selection
var sRect; // rectangle of selection
var linksCoord = []; // This array saves the ID, start point and end point of each link
var selectedLinks = []; // This array saves the selected links which are selected by selection frame
var objectiveNames = [
    "Custo Total",
    "Tempo Total",
    "Atrações",
    "Bairros"
];

function Arcs() {
    if (Data.length !== 0) {
        dim = Object.keys(Data[0]).length;
    } else {
        // Default to 4 objectives if no data
        dim = 4;
    }
    
    // Find the min and max of each objective
    for (i = 0; i < dim; i++) {
        var objKey = "Obj" + (i + 1);
        var temp = Data.map(function(obj) { return obj[objKey]; });
        minObj[i] = Math.min.apply(null, temp);
        maxObj[i] = Math.max.apply(null, temp);
        
        // Normalize the data
        for (z = 0; z < temp.length; z++) {
            Data[z][objKey] = (temp[z] - minObj[i]) / (maxObj[i] - minObj[i]);
        }
    }
    
    var min = 0;
    var max = 1;
    var inR = (doc.clientHeight - 100) / 2; // Inner Radius
    var X = doc.clientWidth / 2;
    var Y = doc.clientHeight / 2;
    var usedEnv = 360 * 80 / 100; // The usable Environment
    var unUsedEnv = (360 - usedEnv) / dim; // The distance between arcs
    var arcAngle = usedEnv / dim; // The angle of each Arc
    var Angles = [];
    var arcs = [];

    for (i = 0; i < dim; i++) {
        Angles[i] = [((i + 1) * unUsedEnv) + (i * arcAngle), ((i + 1) * unUsedEnv) + ((i + 1) * arcAngle)];
        arcs[i] = d3.svg.arc()
            .innerRadius(inR)
            .outerRadius(inR + 12)
            .startAngle(Angles[i][0] * (Math.PI / 180))
            .endAngle(Angles[i][1] * (Math.PI / 180));
    }

    var arcPaths = d3.select('#mainSVG').selectAll('.arcPath')
        .data(arcs)
        .enter()
        .append('path')
        .attr('class', 'arcPath')
        .attr('id', function(d, i) {
            return 'obj' + i.toString();
        })
        .attr('d', function(d, i) {
            return arcs[i]();
        })
        .attr('fill', function(d, i) {
            // Different colors for each objective
            var colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12'];
            return colors[i % colors.length];
        })
        .attr('transform', 'translate(' + X + ',' + Y + ')')
        .on('mouseover', function(d, i) {
            var idx = i;
            d3.selectAll('.arcPath').attr('opacity', 0.2);
            var links = d3.selectAll('.link');
            d3.select(this).attr('opacity', 1);
            links[0].forEach(function(d, i) {
                if ((d.id).search(idx) != -1) {
                    d3.selectAll('#' + d.id).attr('class', 'linkOnObj');
                }
            })
        })
        .on('mouseout', function(d, i) {
            mainSVG.selectAll('.arcPath').attr('opacity', 1);
            mainSVG.selectAll('.linkOnObj').attr('class', 'link');
        });

    // Add objective labels
    for (i = 0; i < dim; i++) {
        var labelAngle = ((Angles[i][0] + Angles[i][1]) / 2) * (Math.PI / 180);
        var labelRadius = inR + 30;
        var labelX = X + Math.sin(labelAngle) * labelRadius;
        var labelY = Y - Math.cos(labelAngle) * labelRadius;
        
        d3.select('#mainSVG').append('text')
            .attr('class', 'objectiveLabel')
            .attr('x', labelX)
            .attr('y', labelY)
            .attr('text-anchor', 'middle')
            .text(objectiveNames[i]);
    }

    // Map the objectives' interval on the arcs
    var tempPath = [];
    for (i = 0; i < arcPaths[0].length; i++) {
        tempPath[i] = d3.select(arcPaths[0][i]).node();
    }
    intervals = d3.scale.linear().domain([0, max]).range([0, d3.select('.arcPath').node().getTotalLength()/2]);

    // Finding the point on the arcs
    var normVal = []; // Finding the normal value of each items on the path before calculating of coordinate
    var Points = []; // The Points of each value on the Arcs
    for (i = 0; i < Data.length; i++) {
        normVal[i] = new Array(dim);
        Points[i] = new Array(dim);
        for (j = 0; j < dim; j++) {
            var objKey = "Obj" + (j + 1);
            normVal[i][j] = intervals(Data[i][objKey]);
            Points[i][j] = tempPath[j].getPointAtLength(normVal[i][j]);
            Points[i][j].x += X;
            Points[i][j].y += Y;
        }
    }

    // Drawing Connections
    var connections = [];
    var lineFunc = d3.svg.line()
        .x(function(d) { return d.x; })
        .y(function(d) { return d.y; })
        .interpolate('bundle')
        .tension(0.4);
    
    var rep = (dim * (dim - 1)) / 2; // Number of connection groups based on number of objectives
    var tempPoint = [];
    
    for (j1 = 0; j1 < dim; j1++) {
        for (j2 = j1 + 1; j2 < dim; j2++) {
            for (i = 0; i < Points.length; i++) {
                tempPoint[i] = new Array(3);
                tempPoint[i][0] = Points[i][j1];
                tempPoint[i][1] = {x: X, y: Y}; // For making Curve in bundle interpolation
                tempPoint[i][2] = Points[i][j2];

                d3.select('#mainSVG').append('path')
                    .attr('class', 'link')
                    .attr('id', function(d, idx) {
                        return 'link' + j1.toString() + "-" + j2.toString() + "-idx" + i.toString();
                    })
                    .attr('d', lineFunc(tempPoint[i]))
                    .on('mouseover', function(d, idx) {
                        var coordinate = d3.mouse(this);
                        d3.select(this).attr('class', 'linkOver');
                        d3.selectAll('.link').attr('class', 'linkNoOver');
                        
                        // Make tooltip
                        var info = []; // The information of each link
                        var idText = d3.select(this).attr('id');
                        var O1 = +idText[4];
                        var O2 = +idText[6];
                        var solutionIdx = +idText.substr(11);
                        
                        var objKey1 = "Obj" + (O1 + 1);
                        var objKey2 = "Obj" + (O2 + 1);
                        
                        // Get the original (non-normalized) values
                        var originalValue1 = Data[solutionIdx][objKey1] * (maxObj[O1] - minObj[O1]) + minObj[O1];
                        var originalValue2 = Data[solutionIdx][objKey2] * (maxObj[O2] - minObj[O2]) + minObj[O2];
                        
                        // Negate values for maximization objectives for display
                        if (O1 >= 2) originalValue1 = -originalValue1;
                        if (O2 >= 2) originalValue2 = -originalValue2;
                        
                        info[0] = originalValue1.toFixed(2);
                        info[1] = originalValue2.toFixed(2);
                        makeTooltip(info, O1, O2, coordinate, solutionIdx + 1);
                    })
                    .on('mouseout', function(d, idx) {
                        d3.select(this).attr('class', 'link');
                        d3.selectAll('.linkNoOver').attr('class', 'link');
                        d3.selectAll('.tooltip').remove();
                    });
            }
        }
    }

    // Make the labels on the Arcs
    var labelPos = []; // Position of labels on arcs
    for (i = 0; i < tempPath.length; i++) {
        labelPos[i] = [
            tempPath[i].getPointAtLength(0),
            tempPath[i].getPointAtLength(tempPath[i].getTotalLength()/2)
        ];
        
        labelPos[i][0].x += X;
        labelPos[i][0].y += Y;
        labelPos[i][1].x += X;
        labelPos[i][1].y += Y;
        
        d3.select('#mainSVG').selectAll('#arcLabel'+i.toString())
            .data(labelPos[i])
            .enter()
            .append('text')
            .attr('id', 'arcLabel'+i.toString())
            .attr('x', function(d, idx) {
                return d.x;
            })
            .attr('y', function(d) {
                return d.y;
            })
            .attr('font-size', 12)
            .text(function(d, idx) {
                if (idx == 0) {
                    // For maximization objectives (index 2 and 3), show max value at start
                    if (i >= 2) {
                        return -maxObj[i].toFixed(1);
                    } else {
                        return minObj[i].toFixed(1);
                    }
                } else {
                    // For maximization objectives (index 2 and 3), show min value at end
                    if (i >= 2) {
                        return -minObj[i].toFixed(1);
                    } else {
                        return maxObj[i].toFixed(1);
                    }
                }
            });
    }

    var tempLinks = mainSVG.selectAll('.link');
    for (i = 0; i < tempLinks[0].length; i++) {
        linksCoord[i] = new Array(3);
        linksCoord[i][0] = tempLinks[0][i].id;
        linksCoord[i][1] = tempLinks[0][i].getPointAtLength(0);
        linksCoord[i][2] = tempLinks[0][i].getPointAtLength(tempLinks[0][i].getTotalLength());
    }
}

function makeTooltip(info, O1, O2, coordinate, solutionId) {
    var toolTipSVG = d3.select('#mainSVG').append('svg')
        .attr('class', 'tooltip')
        .attr('width', 240)
        .attr('height', 100)
        .attr('x', coordinate[0] + 15)
        .attr('y', coordinate[1] - 20);
        
    toolTipSVG.append('rect')
        .attr('width', 240)
        .attr('height', 100)
        .attr('x', 0)
        .attr('y', 0)
        .attr('stroke', 'black')
        .attr('stroke-width', 2)
        .attr('stroke-opacity', 0.8)
        .attr('opacity', 0.9)
        .attr('fill', '#f8f9fa');

    var toolText = toolTipSVG.append('text')
        .attr('class', 'tooltipTitle')
        .attr('x', 10)
        .attr('y', 20)
        .text('Solução #' + solutionId);

    var info1 = [
        objectiveNames[O1] + ': ',
        info[0],
        objectiveNames[O2] + ': ',
        info[1]
    ];
    
    toolText.selectAll('tspan')
        .data(info1)
        .enter()
        .append('tspan')
        .attr('class', function(d, idx) {
            if (idx == 0 || idx == 2)
                return 'tooltipTitle';
            else
                return 'tooltipVal';
        })
        .attr('dy', function(d, idx) {
            if (idx == 0 || idx == 2)
                return 20;
            else
                return 0;
        })
        .attr('x', 20)
        .attr('dx', function(d, idx) {
            if (idx == 0 || idx == 2)
                return 0;
            else
                return 10;
        })
        .text(function(d, idx) {
            // Add units
            if (idx == 1 && O1 == 0) return "R$ " + d;
            if (idx == 1 && O1 == 1) return d + " min";
            if (idx == 3 && O2 == 0) return "R$ " + d;
            if (idx == 3 && O2 == 1) return d + " min";
            return d;
        });
}

function mouseDown() {
    selectingPoints[0] = d3.mouse(this);
    mainSVG.selectAll('.link').attr('class', 'unSelectedLink');
    sRect = d3.select('#mainSVG').append('rect')
        .attr('x', selectingPoints[0][0])
        .attr('y', selectingPoints[0][1])
        .attr('width', 0)
        .attr('height', 0)
        .attr('class', 'selectionFrame');

    d3.select('#mainSVG').on('mousemove', mouseMove);
}

function mouseUp() {
    mainSVG.on('mousemove', null);
    mainSVG.selectAll('rect').remove();
}

function mouseMove() {
    mainSVG.selectAll('.link').attr('class', 'unSelectedLink');
    selectingPoints[1] = d3.mouse(this);
    var sX, sY;
    var w = Math.abs(selectingPoints[1][0] - selectingPoints[0][0]);
    var h = Math.abs(selectingPoints[1][1] - selectingPoints[0][1]);
    
    sRect.attr('x', function(d) {
            if (selectingPoints[0][0] <= selectingPoints[1][0]) {
                sX = selectingPoints[0][0];
                return sX;
            } else {
                sX = selectingPoints[1][0];
                return sX;
            }
        })
        .attr('y', function(d) {
            if (selectingPoints[0][1] <= selectingPoints[1][1]) {
                sY = selectingPoints[0][1];
                return sY;
            } else {
                sY = selectingPoints[1][1];
                return sY;
            }
        })
        .attr('width', w)
        .attr('height', h);

    function selectLink(value, idx, arr) {
        if (value[1].x > sX && value[1].x < sX + w && value[1].y > sY && value[1].y < sY + h) {
            return true;
        }
        if (value[2].x > sX && value[2].x < sX + w && value[2].y > sY && value[2].y < sY + h) {
            return true;
        }
        return false;
    }
    
    selectedLinks = linksCoord.filter(selectLink);
    
    for (i = 0; i < selectedLinks.length; i++) {
        mainSVG.select('#' + selectedLinks[i][0]).attr('class', 'selectedLink');
        d3.selectAll('.selectedLink').moveToFront();
    }
    
    d3.selection.prototype.moveToFront = function() {
        return this.each(function() {
            this.parentNode.appendChild(this);
        });
    };
    
    d3.selection.prototype.moveToBack = function() {
        return this.each(function() {
            var firstChild = this.parentNode.firstChild;
            if (firstChild) {
                this.parentNode.insertBefore(this, firstChild);
            }
        });
    };
}

function Reset() {
    mainSVG.selectAll('.unSelectedLink').attr('class', 'link');
    mainSVG.selectAll('.selectedLink').attr('class', 'link');
    mainSVG.selectAll('.coloredLink').attr('class', 'link');
}''')
    else:
        logger.info(f"Arquivo já existe: {linkedarc_js_path}")

    # Create the linked_arcs.html template
    linked_arcs_template = 'templates/linked_arcs.html'
    if not os.path.exists(linked_arcs_template):
        logger.info(f"Criando arquivo: {linked_arcs_template}")
        with open(linked_arcs_template, 'w') as f:
            f.write('''<!doctype html>
<html>
  <head>
<script src="https://d3js.org/d3.v3.min.js"></script>
    <script src="{{ url_for('static', filename='js/linkedArc.js') }}"></script>
    <link rel="stylesheet" type="text/css" href="https://fonts.googleapis.com/css?family=Josefin+Slab">
    <style>
      body {
        margin: 0;
        padding: 0;
        font-family: Arial, sans-serif;
      }
      #mainDiv {
        position: absolute;
        margin: 0 auto;
        width: 100%;
        height: 100%;
        top: 0px;
        left: 0%;
      }
      .tooltipTitle {
        font-family: 'Arial';
        font-size: 13px;
        font-weight: bold;
      }
      .tooltipVal {
        font-family: 'Josefin Slab';
        font-size: 15px;
        font-style: italic;
      }
      .selectionFrame {
        stroke: gray;
        stroke-width: 1px;
        stroke-dasharray: 4px;
        stroke-opacity: 1;
        fill: blue;
        opacity: 0.1;
      }
      .link {
        stroke: #3498db;
        stroke-width: 1.5;
        opacity: 0.5;
        fill: none;
      }
      .coloredLink {
        stroke-width: 1.5;
        fill: none;
      }
      .linkOver {
        stroke: #e74c3c;
        stroke-width: 2;
        opacity: 1;
        fill: none;
      }
      .linkNoOver {
        stroke: #3498db;
        stroke-width: 1;
        opacity: 0.3;
        fill: none;
      }
      .linkOnObj {
        stroke: #e74c3c;
        stroke-width: 1;
        opacity: 1;
        fill: none;
      }
      .selectedLink {
        stroke: #2ecc71;
        stroke-width: 1.5;
        opacity: 1;
        fill: none;
      }
      .unSelectedLink {
        stroke: #3498db;
        stroke-width: 1;
        opacity: 0.01;
        fill: none;
      }
      .arcPath {
        cursor: pointer;
      }
      .objectiveLabel {
        font-family: Arial, sans-serif;
        font-size: 14px;
        font-weight: bold;
        text-anchor: middle;
      }
      h3 {
        text-align: center;
        font-family: Arial, sans-serif;
        margin: 10px 0;
      }
    </style>
    <title>Visualização de Objetivos Interconectados</title>
  </head>
  <body>
    <h3>Visualização de Objetivos Interconectados</h3>
    <div id="mainDiv"></div>
    <script>
      var doc; // Document Size
      doc = document.getElementById('mainDiv');
      var mainSVG = d3.select('#mainDiv').append('svg')
                    .attr('id', 'mainSVG')
                    .attr('width', doc.clientWidth)
                    .attr('height', doc.clientHeight)
                    .on("mousedown", mouseDown)
                    .on("mouseup", mouseUp)
                    .on('click', Reset);

      // Define objective names
      var objectiveNames = [
        "Custo Total",
        "Tempo Total",
        "Atrações",
        "Bairros"
      ];
      
      d3.json("/data.json", function(error, data) {
        if (error) {
          console.error("Error loading data:", error);
          return;
        }
        
        Data = data;
        Arcs();
      });
    </script>
  </body>
</html>''')
    else:
        logger.info(f"Arquivo já existe: {linked_arcs_template}")

    # Start the app
    app.run_server(debug=True, host='0.0.0.0', port=8050)