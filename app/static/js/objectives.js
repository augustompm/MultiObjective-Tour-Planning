function createObjectivesRadar(data, selectedIndex) {
    const objectives = ['Custo', 'Tempo', 'Atrações', 'Bairros'];
    const container = document.getElementById('objectives-container');
    
    if (!container || !data || data.length === 0) return;
    
    container.innerHTML = '';
    
    const width = Math.min(container.clientWidth, 350);
    const height = width;
    const radius = Math.min(width, height) / 2 - 40;
    
    const svg = d3.select(container)
        .append('svg')
        .attr('width', width)
        .attr('height', height);
    
    const g = svg.append('g')
        .attr('transform', `translate(${width/2}, ${height/2})`);
    
    const angleStep = (2 * Math.PI) / objectives.length;
    
    objectives.forEach((obj, i) => {
        const angle = i * angleStep - Math.PI / 2;
        const x = Math.cos(angle) * (radius + 20);
        const y = Math.sin(angle) * (radius + 20);
        
        g.append('line')
            .attr('x1', 0)
            .attr('y1', 0)
            .attr('x2', Math.cos(angle) * radius)
            .attr('y2', Math.sin(angle) * radius)
            .attr('stroke', '#ddd')
            .attr('stroke-width', 1);
        
        g.append('text')
            .attr('x', x)
            .attr('y', y)
            .attr('text-anchor', 'middle')
            .attr('dominant-baseline', 'middle')
            .style('font-size', '12px')
            .style('font-weight', 'bold')
            .text(obj);
    });
    
    [0.25, 0.5, 0.75, 1].forEach(level => {
        const points = objectives.map((_, i) => {
            const angle = i * angleStep - Math.PI / 2;
            const x = Math.cos(angle) * radius * level;
            const y = Math.sin(angle) * radius * level;
            return `${x},${y}`;
        }).join(' ');
        
        g.append('polygon')
            .attr('points', points)
            .attr('fill', 'none')
            .attr('stroke', '#eee')
            .attr('stroke-width', 1);
    });
    
    data.forEach((solution, index) => {
        const normalizedValues = [
            1 - (solution.CustoTotal - Math.min(...data.map(d => d.CustoTotal))) / 
                (Math.max(...data.map(d => d.CustoTotal)) - Math.min(...data.map(d => d.CustoTotal))),
            1 - (solution.TempoTotal - Math.min(...data.map(d => d.TempoTotal))) / 
                (Math.max(...data.map(d => d.TempoTotal)) - Math.min(...data.map(d => d.TempoTotal))),
            (solution.NumAtracoes - Math.min(...data.map(d => d.NumAtracoes))) / 
                (Math.max(...data.map(d => d.NumAtracoes)) - Math.min(...data.map(d => d.NumAtracoes))),
            (solution.NumBairros - Math.min(...data.map(d => d.NumBairros))) / 
                (Math.max(...data.map(d => d.NumBairros)) - Math.min(...data.map(d => d.NumBairros)))
        ];
        
        const points = normalizedValues.map((value, i) => {
            const angle = i * angleStep - Math.PI / 2;
            const r = radius * value;
            const x = Math.cos(angle) * r;
            const y = Math.sin(angle) * r;
            return `${x},${y}`;
        }).join(' ');
        
        const isSelected = index === selectedIndex;
        
        g.append('polygon')
            .attr('points', points)
            .attr('fill', isSelected ? 'rgba(220, 20, 60, 0.3)' : 'rgba(70, 130, 180, 0.1)')
            .attr('stroke', isSelected ? '#dc143c' : '#4682b4')
            .attr('stroke-width', isSelected ? 2 : 1)
            .style('cursor', 'pointer')
            .on('click', function() {
                if (window.updateSolutionSlider) {
                    window.updateSolutionSlider(index + 1);
                }
            });
    });
}

function createObjectivesMatrix(data, selectedIndex) {
    const container = document.getElementById('matrix-container');
    if (!container || !data || data.length === 0) return;
    
    container.innerHTML = '';
    
    const objectives = ['CustoTotal', 'TempoTotal', 'NumAtracoes', 'NumBairros'];
    const labels = ['Custo (R$)', 'Tempo (min)', 'Atrações', 'Bairros'];
    
    const margin = {top: 20, right: 20, bottom: 40, left: 60};
    const cellSize = 80;
    const width = objectives.length * cellSize + margin.left + margin.right;
    const height = objectives.length * cellSize + margin.top + margin.bottom;
    
    const svg = d3.select(container)
        .append('svg')
        .attr('width', width)
        .attr('height', height);
    
    const g = svg.append('g')
        .attr('transform', `translate(${margin.left}, ${margin.top})`);
    
    objectives.forEach((obj1, i) => {
        objectives.forEach((obj2, j) => {
            if (i !== j) {
                const xScale = d3.scaleLinear()
                    .domain(d3.extent(data, d => d[obj1]))
                    .range([0, cellSize - 10]);
                
                const yScale = d3.scaleLinear()
                    .domain(d3.extent(data, d => d[obj2]))
                    .range([cellSize - 10, 0]);
                
                const cellG = g.append('g')
                    .attr('transform', `translate(${j * cellSize}, ${i * cellSize})`);
                
                cellG.append('rect')
                    .attr('width', cellSize)
                    .attr('height', cellSize)
                    .attr('fill', 'none')
                    .attr('stroke', '#ddd');
                
                data.forEach((d, index) => {
                    const isSelected = index === selectedIndex;
                    
                    cellG.append('circle')
                        .attr('cx', xScale(d[obj1]) + 5)
                        .attr('cy', yScale(d[obj2]) + 5)
                        .attr('r', isSelected ? 4 : 2)
                        .attr('fill', isSelected ? '#dc143c' : '#4682b4')
                        .attr('opacity', isSelected ? 1 : 0.6)
                        .style('cursor', 'pointer')
                        .on('click', function() {
                            if (window.updateSolutionSlider) {
                                window.updateSolutionSlider(index + 1);
                            }
                        });
                });
                
                if (i === objectives.length - 1) {
                    cellG.append('text')
                        .attr('x', cellSize / 2)
                        .attr('y', cellSize + 15)
                        .attr('text-anchor', 'middle')
                        .style('font-size', '10px')
                        .text(labels[j]);
                }
                
                if (j === 0) {
                    cellG.append('text')
                        .attr('x', -10)
                        .attr('y', cellSize / 2)
                        .attr('text-anchor', 'middle')
                        .attr('transform', `rotate(-90, -10, ${cellSize / 2})`)
                        .style('font-size', '10px')
                        .text(labels[i]);
                }
            } else {
                const cellG = g.append('g')
                    .attr('transform', `translate(${j * cellSize}, ${i * cellSize})`);
                
                cellG.append('rect')
                    .attr('width', cellSize)
                    .attr('height', cellSize)
                    .attr('fill', '#f8f9fa')
                    .attr('stroke', '#ddd');
                
                cellG.append('text')
                    .attr('x', cellSize / 2)
                    .attr('y', cellSize / 2)
                    .attr('text-anchor', 'middle')
                    .attr('dominant-baseline', 'middle')
                    .style('font-size', '10px')
                    .style('font-weight', 'bold')
                    .text(labels[i]);
            }
        });
    });
}