# Mobile-First Tourist Route Visualization

Modern, responsive visualization app for multi-objective tourism route optimization results.

## Features

- **Mobile-First Design**: Optimized for touch devices and small screens
- **Responsive Layout**: Bootstrap-based responsive design
- **Clean Interface**: Card-based layout with modern styling
- **Touch-Friendly Controls**: Large touch targets and intuitive navigation
- **Progressive Enhancement**: Works on all devices from mobile to desktop

## Architecture

- **Backend**: Flask/Dash application
- **Frontend**: Bootstrap + custom CSS for mobile optimization
- **Visualization**: Plotly for interactive charts
- **Data Processing**: Pandas for CSV data handling

## Components

1. **Solution Navigation**: Touch-friendly slider for solution selection
2. **Solution Details**: Card-based display of objectives and metadata
3. **Timeline View**: Visual itinerary with attractions and transport modes
4. **Objectives Plot**: Interactive parallel coordinates visualization
5. **Algorithm Evolution**: Tabbed charts showing NSGA-II progress

## Mobile Optimizations

- Viewport meta tag for proper mobile scaling
- Responsive grid system
- Touch-optimized slider controls
- Simplified navigation
- Card-based layout for better mobile UX
- Reduced visual complexity for small screens

## Running the App

```bash
cd app/
pip install -r requirements.txt
python app.py
```

App runs on http://localhost:8051

## Data Requirements

- `../results/nsga2-resultados.csv`: Final Pareto solutions
- `../results/nsga2-geracoes.csv`: Generation evolution data

## Independence

This app is completely self-contained and independent of the legacy visualization system. All dependencies and assets are included within the app/ directory.