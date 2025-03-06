#!/usr/bin/env python3
# visualization/results_visualizer.py

import os
import pandas as pd
import numpy as np
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import dash
from dash import dcc, html, Input, Output, State, callback
import dash_bootstrap_components as dbc

# File paths
RESULTS_FILE = "../build/resultados_nsga2.csv"
GENERATIONS_FILE = "../build/geracoes_nsga2.csv"

# Load data
def load_data():
    # Results data
    results_df = pd.read_csv(RESULTS_FILE, sep=';', encoding='utf-8')
    
    # Parse sequences, times, and transport modes
    results_df['AttracoesLista'] = results_df['Sequencia'].str.split('|')
    results_df['TemposChegadaLista'] = results_df['TemposChegada'].str.split('|')
    results_df['TemposPartidaLista'] = results_df['TemposPartida'].str.split('|')
    results_df['ModosTransporteLista'] = results_df['ModosTransporte'].str.split('|')
    
    # Generations data
    try:
        # Read the file into lines first
        with open(GENERATIONS_FILE, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        # Find where the summary metrics begin
        summary_start_idx = -1
        for i, line in enumerate(lines):
            if 'Final metrics' in line or 'Final Hypervolume' in line:
                summary_start_idx = i
                break
        
        # Split the data into generations and summary
        gen_lines = lines[:summary_start_idx] if summary_start_idx > 0 else lines
        summary_lines = lines[summary_start_idx:] if summary_start_idx > 0 else []
        
        # Parse generations data
        from io import StringIO
        generations_df = pd.read_csv(StringIO(''.join(gen_lines)), sep=';', encoding='utf-8')
        
        # Parse summary metrics if available
        summary_metrics = {}
        for line in summary_lines:
            if ';' in line:
                key, value = line.strip().split(';')
                if value.replace('.', '').isdigit() or value.replace('.', '').replace('-', '').isdigit():
                    summary_metrics[key] = float(value)
                else:
                    summary_metrics[key] = value
    except Exception as e:
        print(f"Warning: Error processing {GENERATIONS_FILE}: {e}")
        generations_df = pd.DataFrame(columns=['Generation', 'Front size', 'Hypervolume', 'Spread'])
        summary_metrics = {}
    
    return results_df, generations_df, summary_metrics

# Create the Dash app
app = dash.Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])

# Load the data
results_df, generations_df, summary_metrics = load_data()

# App layout
app.layout = dbc.Container([
    dbc.Row([
        dbc.Col([
            html.H1("Resultados da Otimização Multi-objetivo NSGA-II", 
                    className="text-center text-primary my-4"),
            html.Hr(),
        ], width=12)
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Visualização 3D das Soluções - Fronteira de Pareto", className="text-center"),
            dcc.Graph(id='3d-scatter-plot'),
            html.Div([
                html.P([
                    "Este gráfico mostra as soluções não-dominadas encontradas pelo NSGA-II. ",
                    "Cada ponto representa uma solução diferente, com: ",
                    html.Ul([
                        html.Li("Eixo X: Custo Total (R$) - quanto menor, melhor"),
                        html.Li("Eixo Y: Tempo Total (minutos) - quanto menor, melhor"),
                        html.Li("Eixo Z: Número de Atrações (negativo) - quanto mais atrações (valor mais alto), melhor")
                    ]),
                    "A cor dos pontos indica o número de atrações visitadas. O diamante vermelho destaca a solução selecionada."
                ], className="text-muted mt-2")
            ], className="px-4")
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Explorar Solução Individual", className="text-center"),
            dcc.Slider(
                id='solution-slider',
                min=1,
                max=len(results_df),
                step=1,
                value=1,
                marks={i: f'Sol. {i}' for i in range(1, len(results_df)+1, max(1, len(results_df)//10))},
            ),
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
                    dcc.Graph(id='hypervolume-plot')
                ], label="Hipervolume"),
                dbc.Tab([
                    dcc.Graph(id='spread-plot')
                ], label="Spread"),
                dbc.Tab([
                    dcc.Graph(id='front-size-plot')
                ], label="Tamanho da Fronteira"),
            ])
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.H3("Métricas Finais", className="text-center"),
            html.Div(id='summary-metrics', className="p-3 border rounded")
        ], width=12, className="mb-4")
    ]),
    
    dbc.Row([
        dbc.Col([
            html.Footer([
                html.P("Visualização desenvolvida para Otimização Multiobjetivo de Roteiros Turísticos", 
                       className="text-center text-muted")
            ])
        ], width=12)
    ])
], fluid=True, className="my-4")

# Callback for 3D scatter plot
@app.callback(
    Output('3d-scatter-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_3d_scatter(selected_solution):
    fig = go.Figure(data=[go.Scatter3d(
        x=results_df['CustoTotal'],
        y=results_df['TempoTotal'],
        z=-results_df['NumAtracoes'],  # Negative because we maximize attractions
        mode='markers',
        marker=dict(
            size=8,
            color=results_df['NumAtracoes'],
            colorscale='Viridis',
            opacity=0.8,
            colorbar=dict(title="Atrações")
        ),
        text=[f'Solução {i+1}<br>Custo: R$ {row["CustoTotal"]:.2f}<br>Tempo: {row["TempoTotal"]:.1f} min<br>Atrações: {row["NumAtracoes"]}' 
              for i, row in results_df.iterrows()],
        hoverinfo='text'
    )])
    
    # Highlight the selected solution
    selected_idx = selected_solution - 1
    if 0 <= selected_idx < len(results_df):
        selected_row = results_df.iloc[selected_idx]
        fig.add_trace(go.Scatter3d(
            x=[selected_row['CustoTotal']],
            y=[selected_row['TempoTotal']],
            z=[-selected_row['NumAtracoes']],
            mode='markers',
            marker=dict(
                size=12,
                color='red',
                symbol='diamond'
            ),
            name=f'Solução {selected_solution}'
        ))
    
    fig.update_layout(
        scene=dict(
            xaxis_title='Custo Total (R$)',
            yaxis_title='Tempo Total (min)',
            zaxis_title='Atrações (negativo)',
            xaxis=dict(backgroundcolor='rgb(240, 240, 240)'),
            yaxis=dict(backgroundcolor='rgb(240, 240, 240)'),
            zaxis=dict(backgroundcolor='rgb(240, 240, 240)')
        ),
        margin=dict(l=0, r=0, b=0, t=0),
        height=600
    )
    
    return fig

# Callback for solution details
@app.callback(
    Output('solution-details', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_details(selected_solution):
    idx = selected_solution - 1
    if idx < 0 or idx >= len(results_df):
        return html.P("Solução não encontrada")
    
    solution = results_df.iloc[idx]
    
    return dbc.Row([
        dbc.Col([
            html.H4(f"Solução #{selected_solution}", className="text-primary"),
            html.P([
                html.Strong("Custo Total: "), f"R$ {solution['CustoTotal']:.2f}"
            ]),
            html.P([
                html.Strong("Tempo Total: "), f"{solution['TempoTotal']:.1f} minutos"
            ]),
            html.P([
                html.Strong("Duração: "), f"{solution['HoraInicio']} - {solution['HoraFim']}"
            ]),
        ], width=6),
        dbc.Col([
            html.P([
                html.Strong("Atrações Visitadas: "), f"{solution['NumAtracoes']}"
            ]),
            html.P([
                html.Strong("Início: "), f"{solution['HoraInicio']}"
            ]),
            html.P([
                html.Strong("Fim: "), f"{solution['HoraFim']}"
            ]),
        ], width=6),
    ])

# Callback for solution timeline
@app.callback(
    Output('solution-timeline', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_timeline(selected_solution):
    idx = selected_solution - 1
    if idx < 0 or idx >= len(results_df):
        return html.P("Solução não encontrada")
    
    solution = results_df.iloc[idx]
    
    attractions = solution['AttracoesLista']
    arrival_times = solution['TemposChegadaLista']
    departure_times = solution['TemposPartidaLista']
    transport_modes = solution['ModosTransporteLista']
    
    # Remove last empty item if exists
    if attractions[-1] == '':
        attractions = attractions[:-1]
    if arrival_times[-1] == '':
        arrival_times = arrival_times[:-1]
    if departure_times[-1] == '':
        departure_times = departure_times[:-1]
    if transport_modes[-1] == '':
        transport_modes = transport_modes[:-1]
    
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
    
    return dbc.Container(timeline_items, className="p-0")

# Callback for hypervolume plot
@app.callback(
    Output('hypervolume-plot', 'figure'),
    Input('solution-slider', 'value')  # Dummy input to ensure the callback runs
)
def update_hypervolume_plot(_):
    if 'Hypervolume' not in generations_df.columns:
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
    
    fig = px.line(df_plot, x='Generation', y='Hypervolume',
                 labels={'Generation': 'Geração', 'Hypervolume': 'Hipervolume'},
                 template='plotly_white')
    
    fig.update_layout(
        title='Evolução do Hipervolume ao Longo das Gerações',
        xaxis_title='Geração',
        yaxis_title='Hipervolume',
        hovermode='closest'
    )
    
    # Ensure the x-axis shows integers for generations
    fig.update_xaxes(type='category' if len(df_plot) < 30 else 'linear', 
                    tickmode='linear' if len(df_plot) < 30 else 'auto',
                    dtick=5 if len(df_plot) < 30 else None)
    
    return fig

# Callback for spread plot
@app.callback(
    Output('spread-plot', 'figure'),
    Input('solution-slider', 'value')  # Dummy input to ensure the callback runs
)
def update_spread_plot(_):
    if 'Spread' not in generations_df.columns:
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
    
    fig = px.line(df_plot, x='Generation', y='Spread',
                 labels={'Generation': 'Geração', 'Spread': 'Diversidade'},
                 template='plotly_white')
    
    fig.update_layout(
        title='Evolução da Diversidade ao Longo das Gerações',
        xaxis_title='Geração',
        yaxis_title='Spread',
        hovermode='closest'
    )
    
    # Ensure the x-axis shows integers for generations
    fig.update_xaxes(type='category' if len(df_plot) < 30 else 'linear', 
                    tickmode='linear' if len(df_plot) < 30 else 'auto',
                    dtick=5 if len(df_plot) < 30 else None)
    
    return fig

# Callback for front size plot
@app.callback(
    Output('front-size-plot', 'figure'),
    Input('solution-slider', 'value')  # Dummy input to ensure the callback runs
)
def update_front_size_plot(_):
    if 'Front size' not in generations_df.columns:
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

# Callback for summary metrics
@app.callback(
    Output('summary-metrics', 'children'),
    Input('solution-slider', 'value')  # Dummy input to ensure the callback runs
)
def update_summary_metrics(_):
    if not summary_metrics:
        return html.P("Nenhuma métrica de resumo disponível")
    
    metrics_cards = []
    
    # Define which metrics to display and their formats
    display_metrics = {
        'Final Hypervolume': {'format': '.4f', 'label': 'Hipervolume Final', 'icon': '📊'},
        'Final Spread': {'format': '.4f', 'label': 'Spread Final', 'icon': '📏'},
        'Execution Time (ms)': {'format': '.0f', 'label': 'Tempo de Execução', 'icon': '⏱️', 'unit': ' ms'}
    }
    
    # Create a row with metric cards
    row_items = []
    
    for key, config in display_metrics.items():
        if key in summary_metrics:
            try:
                # Check if the value is numeric
                if isinstance(summary_metrics[key], (int, float)):
                    format_str = '{:' + config['format'] + '}'
                    value = format_str.format(summary_metrics[key])
                else:
                    # For non-numeric values
                    value = str(summary_metrics[key])
                
                if 'unit' in config:
                    value += config['unit']
                
                card = dbc.Card(
                    dbc.CardBody([
                        html.Div(config['icon'], className="text-primary h1 text-center"),
                        html.H5(config['label'], className="text-center"),
                        html.P(value, className="text-center h4")
                    ]),
                    className="mb-3 shadow-sm"
                )
                
                row_items.append(dbc.Col(card, width=12 // len(display_metrics)))
            except Exception as e:
                print(f"Error formatting metric {key}: {e}")
    
    if row_items:
        metrics_cards.append(dbc.Row(row_items))
    else:
        metrics_cards.append(html.P("Métricas de resumo não puderam ser exibidas."))
    
    return metrics_cards

if __name__ == '__main__':
    app.run_server(debug=True)