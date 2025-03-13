// solution_dial.js - Circular dial for solution selection

function createSolutionDial(containerId, options) {
    // Check if D3 is available
    if (typeof d3 === 'undefined') {
        console.error("D3.js is not loaded. Please ensure D3 is loaded before creating the dial.");
        return {
            setValue: function() {},
            getValue: function() { return 1; }
        };
    }
    
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
}
