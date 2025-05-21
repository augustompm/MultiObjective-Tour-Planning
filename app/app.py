import os
import pandas as pd
import numpy as np
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import dash
from dash import dcc, html, Input, Output, State, callback, ClientsideFunction
import dash_bootstrap_components as dbc
from flask import Flask, send_from_directory, render_template_string, jsonify

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

def generate_slider_marks(max_value):
    """
    Gera marcas inteligentes para o slider baseado no número total de soluções.
    Sempre inclui 1, valores intermediários distribuídos e o valor máximo.
    """
    if max_value <= 1:
        return {1: "1"}
    
    marks = {}
    
    # Sempre incluir 1 e o máximo
    marks[1] = "1"
    marks[max_value] = str(max_value)
    
    # Determinar quantas marcas intermediárias mostrar
    if max_value <= 10:
        # Para poucos valores, mostrar todos
        for i in range(2, max_value):
            marks[i] = str(i)
    elif max_value <= 20:
        # Para valores médios, mostrar múltiplos de 5
        for i in range(5, max_value, 5):
            marks[i] = str(i)
    elif max_value <= 50:
        # Para mais valores, mostrar múltiplos de 10
        for i in range(10, max_value, 10):
            marks[i] = str(i)
    elif max_value <= 100:
        # Para muitos valores, mostrar múltiplos de 20
        for i in range(20, max_value, 20):
            marks[i] = str(i)
    else:
        # Para valores muito altos, mostrar cerca de 5-7 marcas bem distribuídas
        step = max_value // 6
        for i in range(step, max_value, step):
            marks[i] = str(i)
    
    return marks

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
                external_stylesheets=[
                    dbc.themes.BOOTSTRAP,
                    "https://d3js.org/d3.v3.min.js"
                ],
                external_scripts=[
                    "https://d3js.org/d3.v3.min.js"
                ],
                meta_tags=[
                    {"name": "viewport", "content": "width=device-width, initial-scale=1, shrink-to-fit=no"}
                ],
                suppress_callback_exceptions=True)

results_df, generations_df = load_data()

app.layout = dbc.Container([
    # Header with logo
    dbc.Row([
        dbc.Col([
            html.Div([
                html.Img(src='/images/logo-1.png', className="app-logo"),
                html.P("Explore roteiros turísticos otimizados pelo algoritmo NSGA-II", 
                       className="app-subtitle"),
            ], className="app-header text-center d-flex flex-column align-items-center justify-content-center")
        ], width=12, className="text-center")
    ], className="text-center justify-content-center"),
    
    # Objectives visualization (moved to top)
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Visualização dos Objetivos", className="mb-0")
                ]),
                dbc.CardBody([
                    dcc.Graph(
                        id='objectives-plot', 
                        style={'height': '50vh'}, 
                        className="clickable-plot",
                        config={'displayModeBar': False}
                    )
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    # Solution selector with modern slider
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Navegação de Soluções", className="mb-0")
                ]),
                dbc.CardBody([
                    html.Div([
                        html.Label([
                            "Solução: ",
                            html.Span(id='solution-number', children="1", className="fw-bold text-primary")
                        ], className="mb-3"),
                        
                        # Modern range slider with intelligent marks
                        dcc.Slider(
                            id='solution-slider',
                            min=1,
                            max=len(results_df) if not results_df.empty else 1,
                            step=1,
                            value=1,
                            marks=generate_slider_marks(len(results_df) if not results_df.empty else 1),
                            tooltip={"placement": "bottom", "always_visible": True},
                            className="modern-slider"
                        ),
                        
                        html.Div([
                            dbc.ButtonGroup([
                                dbc.Button("◀ Anterior", id="prev-btn", color="outline-secondary", size="sm"),
                                dbc.Button("Próxima ▶", id="next-btn", color="outline-secondary", size="sm"),
                            ], className="mt-3")
                        ], className="d-flex justify-content-center"),
                    ]),
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    # Solution details
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Detalhes da Solução", className="mb-0")
                ]),
                dbc.CardBody([
                    html.Div(id='solution-details')
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    # Timeline / Itinerary
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Itinerário", className="mb-0")
                ]),
                dbc.CardBody([
                    html.Div(id='solution-timeline', className="timeline-container")
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    # Evolution graph (simplified to only show attractions)
    dbc.Row([
        dbc.Col([
            dbc.Card([
                dbc.CardHeader([
                    html.H5("Visualização Harmony - Multi-Objetivo", className="mb-0"),
                    html.P("Visualização interativa das soluções do Pareto", className="text-muted mb-0 mt-1")
                ]),
                dbc.CardBody([
                    html.Div([
                        html.Div([
                            html.Iframe(
                                id='harmony-iframe',
                                src='/harmony-visualization',
                                style={
                                    'width': '100%', 
                                    'height': '500px', 
                                    'border': '1px solid #dee2e6',
                                    'border-radius': '8px'
                                }
                            )
                        ]),
                        html.Div([
                            dbc.ButtonGroup([
                                dbc.Button("Reset Seleção", id="harmony-reset-btn", color="outline-secondary", size="sm"),
                                dbc.Button("Exportar SVG", id="harmony-export-btn", color="outline-primary", size="sm"),
                            ], className="mt-3")
                        ], className="d-flex justify-content-center"),
                    ])
                ])
            ], className="mb-4")
        ], width=12)
    ]),
    
    # Selected solution info
    dbc.Row([
        dbc.Col([
            html.Div(id='selection-info', className="mt-2 text-center")
        ], width=12)
    ]),
    
    # Hidden elements for JS interaction (none needed for now)
    html.Div(style={'display': 'none'}),
    
], fluid=True, className="p-3")

@app.callback(
    Output('objectives-plot', 'figure'),
    Input('solution-slider', 'value')
)
def update_objectives_plot(selected_solution):
    try:
        # Get current data (allows for testing with mocked data)
        current_df = globals().get('results_df', pd.DataFrame())
        
        if current_df.empty:
            return go.Figure()
        
        selected_idx = selected_solution - 1
        color_values = np.ones(len(current_df)) * 0.3
        
        if 0 <= selected_idx < len(current_df):
            color_values[selected_idx] = 1.0
        
        cost_min = current_df['CustoTotal'].min()
        cost_max = current_df['CustoTotal'].max()
        time_min = current_df['TempoTotal'].min()
        time_max = current_df['TempoTotal'].max()
        attractions_values = sorted([int(x) for x in current_df['NumAtracoes'].unique() if pd.notna(x)])
        neighborhoods_values = sorted([int(x) for x in current_df['NumBairros'].unique() if pd.notna(x)])
        
        # Create tooltip data
        customdata = np.column_stack((
            np.arange(len(current_df)) + 1,  # Solution ID (1-indexed)
            current_df['CustoTotal'].values,
            current_df['TempoTotal'].values,
            current_df['NumAtracoes'].values,
            current_df['NumBairros'].values
        ))
        
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
                        values=current_df['CustoTotal'],
                        tickvals=np.linspace(cost_min, cost_max, 5),
                        ticktext=[f"R$ {val:.0f}" for val in np.linspace(cost_min, cost_max, 5)]
                    ),
                    dict(
                        range=[time_min, time_max],
                        label='Tempo (min)',
                        values=current_df['TempoTotal'],
                        tickvals=np.linspace(time_min, time_max, 5),
                        ticktext=[f"{val:.0f}" for val in np.linspace(time_min, time_max, 5)]
                    ),
                    dict(
                        range=[min(attractions_values), max(attractions_values)],
                        label='Atrações',
                        values=current_df['NumAtracoes'],
                        tickvals=attractions_values,
                        ticktext=attractions_values
                    ),
                    dict(
                        range=[min(neighborhoods_values), max(neighborhoods_values)],
                        label='Bairros',
                        values=current_df['NumBairros'],
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
            plot_bgcolor='rgba(255,255,255,0.9)',
            clickmode='event+select'
        )
        
        return fig
    except Exception:
        return go.Figure()

# Callback para capturar seleções no gráfico de coordenadas paralelas
@app.callback(
    [Output('solution-slider', 'value', allow_duplicate=True),
     Output('selection-info', 'children')],
    [Input('objectives-plot', 'selectedData'),
     Input('objectives-plot', 'clickData')],
    prevent_initial_call=True
)
def update_from_objectives_selection(selectedData, clickData):
    try:
        ctx = dash.callback_context
        if not ctx.triggered:
            return dash.no_update, ""
        
        trigger_id = ctx.triggered[0]['prop_id'].split('.')[1]
        
        if trigger_id == 'selectedData' and selectedData and 'points' in selectedData:
            # Multiple solutions selected via brushing
            selected_indices = [point['pointIndex'] for point in selectedData['points']]
            if selected_indices:
                # Take the first selected solution
                new_value = selected_indices[0] + 1
                info_msg = f"Selecionadas {len(selected_indices)} soluções. Visualizando solução #{new_value}"
                return new_value, html.P(info_msg, className="text-info")
        
        elif trigger_id == 'clickData' and clickData and 'points' in clickData:
            # Single solution clicked
            point_index = clickData['points'][0]['pointIndex']
            new_value = point_index + 1
            info_msg = f"Solução #{new_value} selecionada via clique"
            return new_value, html.P(info_msg, className="text-success")
        
        return dash.no_update, ""
    except Exception:
        return dash.no_update, ""

# Callback para atualizar o número da solução
@app.callback(
    Output('solution-number', 'children'),
    Input('solution-slider', 'value')
)
def update_solution_number(value):
    return str(value)

# Callback para botões de navegação
@app.callback(
    Output('solution-slider', 'value', allow_duplicate=True),
    [Input('prev-btn', 'n_clicks'),
     Input('next-btn', 'n_clicks')],
    State('solution-slider', 'value'),
    State('solution-slider', 'max'),
    prevent_initial_call=True
)
def navigate_solution(prev_clicks, next_clicks, current_value, max_value):
    try:
        ctx = dash.callback_context
        if not ctx.triggered:
            return dash.no_update
        
        button_id = ctx.triggered[0]['prop_id'].split('.')[0]
        
        if button_id == 'prev-btn' and current_value > 1:
            return current_value - 1
        elif button_id == 'next-btn' and current_value < max_value:
            return current_value + 1
        
        return dash.no_update
    except Exception:
        return dash.no_update

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
        
        # Create stats cards with improved styling
        return html.Div([
            html.Div([
                dbc.Row([
                    dbc.Col([
                        html.Div([
                            html.Div("💰", className="h3 text-primary"),
                            html.Div("Custo Total", className="small text-muted"),
                            html.Div(f"R$ {solution.get('CustoTotal', 0):.2f}", className="h5")
                        ], className="stat-item")
                    ], xs=6, md=3),
                    dbc.Col([
                        html.Div([
                            html.Div("⏱️", className="h3 text-primary"),
                            html.Div("Tempo Total", className="small text-muted"),
                            html.Div(f"{solution.get('TempoTotal', 0):.0f} min", className="h5")
                        ], className="stat-item")
                    ], xs=6, md=3),
                    dbc.Col([
                        html.Div([
                            html.Div("🎯", className="h3 text-primary"),
                            html.Div("Atrações", className="small text-muted"),
                            html.Div(f"{solution.get('NumAtracoes', 0)}", className="h5")
                        ], className="stat-item")
                    ], xs=6, md=3),
                    dbc.Col([
                        html.Div([
                            html.Div("🏘️", className="h3 text-primary"),
                            html.Div("Bairros", className="small text-muted"),
                            html.Div(f"{solution.get('NumBairros', 0)}", className="h5")
                        ], className="stat-item")
                    ], xs=6, md=3),
                ], className="mb-3"),
                
                dbc.Row([
                    dbc.Col([
                        html.Div([
                            html.Div("🕘", className="h5 text-primary d-inline-block me-2"),
                            html.Div([
                                html.Span("Horário: ", className="text-muted me-1"),
                                f"{solution.get('HoraInicio', '09:00')} - {solution.get('HoraFim', '22:00')}"
                            ], className="d-inline-block")
                        ], className="mb-2")
                    ], width=12),
                    dbc.Col([
                        html.Div([
                            html.Div("📍", className="h5 text-primary d-inline-block me-2 align-top"),
                            html.Div([
                                html.Span("Bairros: ", className="text-muted me-1"),
                                ", ".join(solution.get('BairrosLista', [])) if hasattr(solution, 'BairrosLista') else ""
                            ], className="d-inline-block")
                        ])
                    ], width=12),
                ])
            ], className="solution-stats")
        ])
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
                
                # Add transport info if not the first attraction
                if i > 0 and i-1 < len(transport_modes):
                    transport_mode = transport_modes[i-1]
                    transport_icon = "🚶" if transport_mode == "Walk" else "🚗"
                    
                    timeline_items.append(html.Div([
                        html.Div(transport_icon, className="h4 me-2 d-inline-block"),
                        html.Div([
                            f"De {attractions[i-1]} para {attr}",
                        ], className="d-inline-block small text-muted")
                    ], className="transport-item"))
                
                # Add attraction info
                timeline_items.append(html.Div([
                    html.Div([
                        html.Div("🏛️", className="h4 me-2 d-inline-block align-top"),
                        html.Div([
                            html.Div(attr, className="fw-bold text-primary"),
                            html.Div([
                                html.Span("Chegada: ", className="text-muted"),
                                html.Span(arrival, className="me-3"),
                                html.Span("Saída: ", className="text-muted"),
                                html.Span(departure)
                            ], className="small")
                        ], className="d-inline-block")
                    ])
                ], className="timeline-item"))
        
        if not timeline_items:
            return html.P("Nenhum item no itinerário")
            
        return timeline_items
    except Exception:
        return html.P("Erro ao carregar itinerário da solução")

# Harmony visualization route

@app.callback(
    Output('harmony-export-btn', 'children'),
    Input('harmony-export-btn', 'n_clicks'),
    prevent_initial_call=True
)
def export_harmony_svg(n_clicks):
    """Handle SVG export button click"""
    if n_clicks:
        # Add some feedback to the button
        return "✓ Exportado"
    return "Exportar SVG"

# Add client-side callback for reset button
app.clientside_callback(
    """
    function(n_clicks) {
        if (n_clicks) {
            // Find the harmony iframe and send reset message
            var iframe = document.getElementById('harmony-iframe');
            if (iframe && iframe.contentWindow) {
                iframe.contentWindow.postMessage({action: 'reset'}, '*');
            }
        }
        return window.dash_clientside.no_update;
    }
    """,
    Output('harmony-reset-btn', 'children'),
    Input('harmony-reset-btn', 'n_clicks'),
    prevent_initial_call=True
)

# Routes for Harmony visualization
@server.route('/harmony-visualization')
def harmony_visualization():
    """Serve the harmony visualization page"""
    return render_template_string(create_harmony_html())

@server.route('/harmony-data.json')
def get_harmony_data():
    """Serve the harmony data as JSON"""
    try:
        harmony_data = []
        for idx, row in results_df.iterrows():
            harmony_data.append({
                'Obj1': row['CustoTotal'],
                'Obj2': row['TempoTotal'],  
                'Obj3': row['NumAtracoes'],
                'Obj4': row['NumBairros']
            })
        return jsonify(harmony_data)
    except Exception as e:
        return jsonify([])

def create_harmony_html():
    """Create the complete HTML for the harmony visualization"""
    return '''<!doctype html>
<html>
<head>
    <script src="https://d3js.org/d3.v3.min.js"></script>
    <script src="/static/js/harmony.js"></script>
    <style>
        body {
            margin: 0;
            padding: 10px;
            font-family: 'Segoe UI', sans-serif;
            background-color: #f8f9fa;
        }
        
        #mainDiv {
            position: relative;
            width: 100%;
            height: calc(100vh - 20px);
            min-height: 300px;
            min-width: 300px;
        }
        
        #mainSVG {
            width: 100% !important;
            height: 100% !important;
            min-height: 300px;
            min-width: 300px;
        }
        
        .arcPath {
            fill: #e8eaf6;
            stroke: #3f51b5;
            stroke-width: 2;
            opacity: 0.8;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .arcPath:hover {
            fill: #3f51b5;
            opacity: 1;
        }
        
        /* Remove any default shadows or filters */
        svg * {
            filter: none !important;
            -webkit-filter: none !important;
        }
        
        /* Ensure no drop shadows on any elements */
        circle, path, line {
            filter: none !important;
            box-shadow: none !important;
        }
        
        .link {
            fill: none !important;
            stroke: rgba(63, 81, 181, 0.4);
            stroke-width: 1;
            cursor: pointer;
            opacity: 0.8;
        }
        
        .linkOver {
            stroke: #e91e63;
            stroke-width: 3;
            opacity: 1;
        }
        
        .linkNoOver {
            opacity: 0.2;
        }
        
        .linkOnObj {
            stroke: #ff9800;
            stroke-width: 2.5;
            opacity: 1;
        }
        
        .selectedLink {
            stroke: #4caf50;
            stroke-width: 3;
            opacity: 1;
        }
        
        .unSelectedLink {
            opacity: 0.05;
        }
        
        /* Prevent dark center effect from overlapping lines */
        #mainSVG {
            background-color: #f8f9fa;
        }
        
        /* Ensure all paths have no fill to prevent dark areas */
        #mainSVG path {
            fill: none !important;
        }
        
        .selectionFrame {
            fill: rgba(76, 175, 80, 0.2);
            stroke: #4caf50;
            stroke-width: 2;
            stroke-dasharray: 5,5;
        }
        
        .objectiveLabel {
            font-family: 'Segoe UI', sans-serif;
            font-size: 16px;
            font-weight: bold;
            fill: #333;
            text-anchor: middle;
        }
        
        .objectiveValue {
            font-family: 'Segoe UI', sans-serif;
            font-size: 12px;
            font-weight: bold;
            fill: #666;
            text-anchor: middle;
        }
        
        .tooltipTitle {
            font-family: 'Segoe UI', sans-serif;
            font-size: 14px;
            font-weight: bold;
            fill: #333;
        }
        
        .tooltipVal {
            font-family: 'Segoe UI', sans-serif;
            font-size: 13px;
            fill: #666;
        }
    </style>
    <title>Harmony Visualization - Multi-Objective</title>
</head>
<body>
    <div id="mainDiv"></div>
    
    <script>
        var doc = document.getElementById('mainDiv');
        var mainSVG = d3.select('#mainDiv').append('svg')
                        .attr('id', 'mainSVG')
                        .attr('width', '100%')
                        .attr('height', '100%')
                        .on("mousedown", mouseDown)
                        .on("mouseup", mouseUp)
                        .on("click", function() {
                            // Only reset if clicking on the background (not on arcs or links)
                            var target = d3.event.target;
                            if (target.tagName === 'svg' || target.id === 'mainSVG') {
                                if (typeof Reset === 'function') {
                                    Reset();
                                }
                            }
                        });
        
        // Load data and initialize visualization
        d3.json("/harmony-data.json", function(error, data) {
            if (error) {
                console.error("Error loading harmony data:", error);
                return;
            }
            
            // Set global data
            rawData = data;
            
            // Initialize the visualization
            if (typeof Arcs === 'function') {
                Arcs();
            } else {
                console.error("Arcs function not found in harmony.js");
            }
        });
        
        // Listen for messages from parent window (reset button)
        window.addEventListener('message', function(event) {
            if (event.data && event.data.action === 'reset') {
                if (typeof Reset === 'function') {
                    Reset();
                }
            }
        });
        
        // Make visualization responsive
        var resizeTimeout;
        window.addEventListener('resize', function() {
            // Debounce resize events to avoid excessive redraws
            clearTimeout(resizeTimeout);
            resizeTimeout = setTimeout(function() {
                resizeVisualization();
            }, 250);
        });
        
        function resizeVisualization() {
            // Get current container dimensions
            var container = document.getElementById('mainDiv');
            if (!container) return;
            
            var newWidth = container.clientWidth;
            var newHeight = container.clientHeight;
            
            // Only resize if dimensions actually changed
            var currentSVG = d3.select('#mainSVG');
            var currentWidth = parseInt(currentSVG.attr('width'));
            var currentHeight = parseInt(currentSVG.attr('height'));
            
            if (Math.abs(newWidth - currentWidth) < 10 && Math.abs(newHeight - currentHeight) < 10) {
                return; // Skip if change is minimal
            }
            
            console.log('Resizing harmony visualization:', newWidth + 'x' + newHeight);
            
            // Clear existing visualization
            currentSVG.selectAll('*').remove();
            
            // Update SVG dimensions
            currentSVG
                .attr('width', newWidth)
                .attr('height', newHeight);
            
            // Update global doc reference
            doc = container;
            
            // Redraw the visualization with new dimensions
            if (typeof Arcs === 'function' && rawData && rawData.length > 0) {
                Arcs();
            }
        }
        
        // Also detect iframe resize events
        var ro = new ResizeObserver(function(entries) {
            for (let entry of entries) {
                if (entry.target.id === 'mainDiv') {
                    clearTimeout(resizeTimeout);
                    resizeTimeout = setTimeout(function() {
                        resizeVisualization();
                    }, 250);
                }
            }
        });
        
        if (doc) {
            ro.observe(doc);
        }
    </script>
</body>
</html>'''

# Add custom route for serving JS and logo
@server.route('/static/<path:path>')
def serve_static(path):
    return send_from_directory('static', path)

@server.route('/images/<path:path>')
def serve_images(path):
    return send_from_directory('images', path)

# Clean modern app - no client-side scripts needed

if __name__ == '__main__':
    import signal
    import sys
    import os
    import atexit
    
    # Flag to prevent recursive signal handling
    shutting_down = False
    
    def cleanup():
        """Clean up any remaining resources"""
        try:
            # Only cleanup if we're not already shutting down
            if not shutting_down:
                pass  # Remove the pkill that was causing the loop
        except:
            pass
    
    def signal_handler(sig, frame):
        global shutting_down
        if shutting_down:
            return  # Prevent recursive calls
        shutting_down = True
        print('\nShutting down gracefully...')
        cleanup()
        sys.exit(0)
    
    # Register cleanup function for normal exits
    atexit.register(cleanup)
    
    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        # Disable hot reload and use_reloader to prevent multiple processes
        app.run_server(
            debug=True, 
            host='0.0.0.0', 
            port=8051, 
            dev_tools_hot_reload=False,
            use_reloader=False,
            threaded=True
        )
    except KeyboardInterrupt:
        print('\nApplication stopped by user')
        cleanup()
        sys.exit(0)
    except Exception as e:
        print(f'\nApplication error: {e}')
        cleanup()
        sys.exit(1)