import os
import pandas as pd
import numpy as np
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import dash
from dash import dcc, html, Input, Output, State, callback
import dash_bootstrap_components as dbc
from flask import Flask

RESULTS_FILE = "../results/nsga2-resultados.csv"
GENERATIONS_FILE = "../results/nsga2-geracoes.csv"

def find_file(primary_path, alternate_names=None):
    if os.path.exists(primary_path):
        return primary_path
    
    if alternate_names:
        primary_dir = os.path.dirname(primary_path)
        for alt_name in alternate_names:
            alt_path = os.path.join(primary_dir, alt_name)
            if os.path.exists(alt_path):
                return alt_path
    
    common_dirs = ["../results/", "results/", "../", "./"]
    filename = os.path.basename(primary_path)
    
    for directory in common_dirs:
        path = os.path.join(directory, filename)
        if os.path.exists(path):
            return path
            
        if alternate_names:
            for alt_name in alternate_names:
                alt_path = os.path.join(directory, alt_name)
                if os.path.exists(alt_path):
                    return alt_path
    
    return None

def load_data():
    try:
        results_path = find_file(RESULTS_FILE)
        if not results_path:
            return pd.DataFrame(), pd.DataFrame()

        results_df = pd.read_csv(results_path, sep=';', encoding='utf-8')
        
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
                results_df[new_col] = results_df[new_col].apply(lambda x: [i for i in x if i])
        
        try:
            generations_path = find_file(GENERATIONS_FILE)
            if not generations_path:
                generations_df = pd.DataFrame(columns=['Generation', 'Front size', 'Best Cost', 'Best Time', 'Max Attractions'])
            else:
                generations_df = pd.read_csv(generations_path, sep=';', encoding='utf-8')
        except Exception:
            generations_df = pd.DataFrame(columns=['Generation', 'Front size', 'Best Cost', 'Best Time', 'Max Attractions'])
        
        return results_df, generations_df
    
    except Exception:
        return pd.DataFrame(), pd.DataFrame()

server = Flask(__name__)

app = dash.Dash(__name__, 
                server=server, 
                external_stylesheets=[dbc.themes.BOOTSTRAP],
                meta_tags=[
                    {"name": "viewport", "content": "width=device-width, initial-scale=1, shrink-to-fit=no"}
                ])

results_df, generations_df = load_data()

app.layout = dbc.Container([
    dbc.Row([
        dbc.Col([
            html.H1("Um Dia no Rio", className="text-center text-primary mb-3"),
            html.P("Explore roteiros turísticos otimizados pelo algoritmo NSGA-II", 
                   className="text-center text-muted mb-4"),
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardBody([
                    html.H5("Navegação de Soluções", className="card-title"),
                    html.Div([
                        dbc.Label("Solução:", className="mb-2"),
                        dcc.Slider(
                            id='solution-slider',
                            min=1,
                            max=len(results_df) if not results_df.empty else 1,
                            step=1,
                            value=1,
                            marks={},
                            tooltip={"placement": "bottom", "always_visible": True}
                        ),
                    ]),
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardBody([
                    html.Div(id='solution-details')
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Itinerário", className="mb-0")
                ]),
                dbc.CardBody([
                    html.Div(id='solution-timeline')
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Visualização dos Objetivos", className="mb-0")
                ]),
                dbc.CardBody([
                    dcc.Graph(id='objectives-plot', style={'height': '70vh'})
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Evolução do Algoritmo", className="mb-0")
                ]),
                dbc.CardBody([
                    dbc.Tabs([
                        dbc.Tab([
                            dcc.Graph(id='front-size-plot')
                        ], label="Fronteira"),
                        dbc.Tab([
                            dcc.Graph(id='best-cost-plot')
                        ], label="Custo"),
                        dbc.Tab([
                            dcc.Graph(id='max-attractions-plot')
                        ], label="Atrações"),
                    ])
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
], fluid=True, className="p-3")

@app.callback(
    Output('objectives-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_objectives_plot(selected_solution):
    try:
        if results_df.empty:
            return go.Figure()
        
        selected_idx = selected_solution - 1
        color_values = np.ones(len(results_df)) * 0.3
        
        if 0 <= selected_idx < len(results_df):
            color_values[selected_idx] = 1.0
        
        cost_min = results_df['CustoTotal'].min()
        cost_max = results_df['CustoTotal'].max()
        time_min = results_df['TempoTotal'].min()
        time_max = results_df['TempoTotal'].max()
        attractions_values = sorted([int(x) for x in results_df['NumAtracoes'].unique()])
        neighborhoods_values = sorted([int(x) for x in results_df['NumBairros'].unique()])
        
        fig = go.Figure(data=
            go.Parcoords(
                line=dict(
                    color=color_values,
                    colorscale=[[0, 'rgba(70, 130, 180, 0.3)'], [1, 'rgba(220, 20, 60, 1.0)']],
                    showscale=False
                ),
                dimensions=[
                    dict(
                        range=[cost_min, cost_max],
                        label='Custo (R$)',
                        values=results_df['CustoTotal'],
                        tickvals=np.linspace(cost_min, cost_max, 5),
                        ticktext=[f"R$ {val:.0f}" for val in np.linspace(cost_min, cost_max, 5)]
                    ),
                    dict(
                        range=[time_min, time_max],
                        label='Tempo (min)',
                        values=results_df['TempoTotal'],
                        tickvals=np.linspace(time_min, time_max, 5),
                        ticktext=[f"{val:.0f}" for val in np.linspace(time_min, time_max, 5)]
                    ),
                    dict(
                        range=[min(attractions_values), max(attractions_values)],
                        label='Atrações',
                        values=results_df['NumAtracoes'],
                        tickvals=attractions_values,
                        ticktext=attractions_values
                    ),
                    dict(
                        range=[min(neighborhoods_values), max(neighborhoods_values)],
                        label='Bairros',
                        values=results_df['NumBairros'],
                        tickvals=neighborhoods_values,
                        ticktext=neighborhoods_values
                    )
                ]
            )
        )
        
        fig.update_layout(
            margin=dict(l=20, r=20, t=20, b=20),
            font=dict(size=12),
            paper_bgcolor='rgba(255,255,255,0.9)',
            plot_bgcolor='rgba(255,255,255,0.9)'
        )
        
        return fig
    except Exception:
        return go.Figure()

@app.callback(
    Output('solution-details', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_details(selected_solution):
    try:
        if results_df.empty:
            return html.P("Dados não disponíveis")
        
        idx = selected_solution - 1
        if idx < 0 or idx >= len(results_df):
            return html.P("Solução não encontrada")
        
        solution = results_df.iloc[idx]
        
        return [
            html.H4(f"Solução #{selected_solution}", className="text-primary mb-3"),
            dbc.Row([
                dbc.Col([
                    html.Div([
                        html.Strong("💰 Custo Total: "),
                        f"R$ {solution.get('CustoTotal', 0):.2f}"
                    ], className="mb-2"),
                    html.Div([
                        html.Strong("⏱️ Tempo Total: "),
                        f"{solution.get('TempoTotal', 0):.0f} minutos"
                    ], className="mb-2"),
                ], xs=12, md=6),
                dbc.Col([
                    html.Div([
                        html.Strong("🎯 Atrações: "),
                        f"{solution.get('NumAtracoes', 0)}"
                    ], className="mb-2"),
                    html.Div([
                        html.Strong("🏘️ Bairros: "),
                        f"{solution.get('NumBairros', 0)}"
                    ], className="mb-2"),
                ], xs=12, md=6),
            ]),
            html.Hr(),
            html.Div([
                html.Strong("🕘 Horário: "),
                f"{solution.get('HoraInicio', '09:00')} - {solution.get('HoraFim', '22:00')}"
            ], className="mb-2"),
            html.Div([
                html.Strong("📍 Bairros Visitados: "),
                ", ".join(solution.get('BairrosLista', [])) if hasattr(solution, 'BairrosLista') else ""
            ]),
        ]
    except Exception:
        return html.P("Erro ao carregar detalhes da solução")

@app.callback(
    Output('solution-timeline', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_timeline(selected_solution):
    try:
        if results_df.empty:
            return html.P("Dados não disponíveis")
        
        idx = selected_solution - 1
        if idx < 0 or idx >= len(results_df):
            return html.P("Solução não encontrada")
        
        solution = results_df.iloc[idx]
        
        attractions = solution.get('AttracoesLista', [])
        arrival_times = solution.get('TemposChegadaLista', [])
        departure_times = solution.get('TemposPartidaLista', [])
        transport_modes = solution.get('ModosTransporteLista', [])
        
        timeline_items = []
        
        for i, attr in enumerate(attractions):
            if i < len(arrival_times) and i < len(departure_times):
                arrival = arrival_times[i]
                departure = departure_times[i]
                
                if i > 0 and i-1 < len(transport_modes):
                    transport_mode = transport_modes[i-1]
                    transport_icon = "🚶" if transport_mode == "Walk" else "🚗"
                    
                    timeline_items.append(
                        dbc.Card([
                            dbc.CardBody([
                                html.Div([
                                    html.Span(transport_icon, className="me-2"),
                                    html.Small(f"De {attractions[i-1]} para {attr}", 
                                             className="text-muted")
                                ])
                            ])
                        ], className="mb-2 border-start border-primary border-3")
                    )
                
                timeline_items.append(
                    dbc.Card([
                        dbc.CardBody([
                            html.H6([
                                html.Span("🏛️", className="me-2"),
                                attr
                            ], className="text-primary mb-1"),
                            html.Small([
                                html.Strong("Chegada: "), arrival,
                                html.Span(" • ", className="mx-2"),
                                html.Strong("Saída: "), departure
                            ], className="text-muted")
                        ])
                    ], className="mb-3")
                )
        
        if not timeline_items:
            return html.P("Nenhum item no itinerário")
            
        return timeline_items
    except Exception:
        return html.P("Erro ao carregar itinerário da solução")

@app.callback(
    Output('front-size-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_front_size_plot(_):
    try:
        if generations_df.empty or 'Front size' not in generations_df.columns:
            return go.Figure()
        
        df_plot = generations_df.copy()
        if df_plot['Generation'].dtype == 'object':
            df_plot['Generation'] = pd.to_numeric(df_plot['Generation'], errors='coerce')
            df_plot = df_plot.dropna(subset=['Generation'])
        
        df_plot = df_plot.sort_values('Generation')
        
        fig = px.line(df_plot, x='Generation', y='Front size',
                     labels={'Generation': 'Geração', 'Front size': 'Tamanho da Fronteira'},
                     template='plotly_white')
        
        fig.update_layout(
            title='Evolução do Tamanho da Fronteira',
            margin=dict(l=20, r=20, t=40, b=20),
            hovermode='closest'
        )
        
        return fig
    except Exception:
        return go.Figure()

@app.callback(
    Output('best-cost-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_best_cost_plot(_):
    try:
        if generations_df.empty or 'Best Cost' not in generations_df.columns:
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
            margin=dict(l=20, r=20, t=40, b=20),
            hovermode='closest'
        )
        
        return fig
    except Exception:
        return go.Figure()

@app.callback(
    Output('max-attractions-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_max_attractions_plot(_):
    try:
        if generations_df.empty or 'Max Attractions' not in generations_df.columns:
            return go.Figure()
        
        df_plot = generations_df.copy()
        if df_plot['Generation'].dtype == 'object':
            df_plot['Generation'] = pd.to_numeric(df_plot['Generation'], errors='coerce')
            df_plot = df_plot.dropna(subset=['Generation'])
        
        df_plot = df_plot.sort_values('Generation')
        
        fig = px.line(df_plot, x='Generation', y='Max Attractions',
                     labels={'Generation': 'Geração', 'Max Attractions': 'Máximo de Atrações'},
                     template='plotly_white')
        
        fig.update_layout(
            title='Evolução do Número Máximo de Atrações',
            margin=dict(l=20, r=20, t=40, b=20),
            hovermode='closest'
        )
        
        return fig
    except Exception:
        return go.Figure()

if __name__ == '__main__':
    app.run_server(debug=True, host='0.0.0.0', port=8051)