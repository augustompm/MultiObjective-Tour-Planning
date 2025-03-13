// File: static/js/linkedArc.js

var dim; // the size of dimension of problem
var doc; // Document size
var minObj = [], maxObj = []; // min and max of Objectives
var rawData = []; // Original non-normalized data
var Data = []; // Normalized data for display
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

// Define if each objective is to be minimized (true) or maximized (false)
var toMinimize = [true, true, false, false];

function Arcs() {
    if (Data.length === 0 && rawData.length > 0) {
        // Process data if we received raw data but haven't normalized it yet
        processData();
    }
    
    if (Data.length === 0) {
        // If still no data, use dummy data
        createDummyData();
        processData();
    }
    
    dim = Object.keys(Data[0]).length;
    
    // Setup dimensions and layout
    var inR = (doc.clientHeight - 150) / 2; // Inner Radius
    var outR = inR + 30; // Outer Radius
    var X = doc.clientWidth / 2;
    var Y = doc.clientHeight / 2;
    
    // Calculate angles for a complete circle with even spacing
    var arcAngles = [];
    for (var i = 0; i < dim; i++) {
        // Create gaps between objectives
        var startAngle = i * (360/dim) + 5;  // Add a small gap (5 degrees)
        var endAngle = (i+1) * (360/dim) - 5; // Subtract a small gap (5 degrees)
        arcAngles[i] = {
            start: startAngle * (Math.PI / 180),
            end: endAngle * (Math.PI / 180)
        };
    }
    
    // Draw background elements
    drawBackground(X, Y, inR, outR, arcAngles);
    
    // Create arc generators
    var arcs = [];
    for (i = 0; i < dim; i++) {
        arcs[i] = d3.svg.arc()
            .innerRadius(inR)
            .outerRadius(outR)
            .startAngle(arcAngles[i].start)
            .endAngle(arcAngles[i].end);
    }
    
    // Draw arcs
    var arcPaths = d3.select('#mainSVG').selectAll('.arcPath')
        .data(arcs)
        .enter()
        .append('path')
        .attr('class', 'arcPath')
        .attr('id', function(d, i) {
            return 'obj' + i;
        })
        .attr('d', function(d) { return d(); })
        .attr('transform', 'translate(' + X + ',' + Y + ')')
        .on('mouseover', function(d, i) {
            highlightObjective(i);
        })
        .on('mouseout', function() {
            unhighlightObjectives();
        });
    
    // Add labels for each objective
    addObjectiveLabels(X, Y, inR, outR, arcAngles);
    
    // Create points for each solution on each arc
    var solutionPoints = calculateSolutionPoints(X, Y, inR, outR, arcAngles);
    
    // Draw lines connecting the solutions across objectives
    drawConnectingLines(solutionPoints, X, Y);
    
    // Store line coordinates for selection functionality
    storeLineCoordinates();
}

function processData() {
    // Create a deep copy of the raw data to preserve original values
    Data = JSON.parse(JSON.stringify(rawData));
    
    dim = Object.keys(Data[0]).length;
    
    // First pass: find min/max for each objective
    for (var i = 0; i < dim; i++) {
        var objKey = "Obj" + (i + 1);
        var values = Data.map(function(obj) { return obj[objKey]; });
        minObj[i] = Math.min.apply(null, values);
        maxObj[i] = Math.max.apply(null, values);
    }
    
    // Second pass: normalize data
    for (i = 0; i < dim; i++) {
        var objKey = "Obj" + (i + 1);
        
        // If there's no range, set a default value to avoid division by zero
        var range = maxObj[i] - minObj[i];
        if (range === 0) range = 1;
        
        for (var j = 0; j < Data.length; j++) {
            // Normalize to [0,1] - for all objectives, 1 is best
            var normalized = (Data[j][objKey] - minObj[i]) / range;
            
            // For maximization objectives, invert the normalization
            if (!toMinimize[i]) {
                normalized = 1 - normalized;
            }
            
            Data[j][objKey] = normalized;
        }
    }
}

function createDummyData() {
    // Create dummy data if none is provided
    rawData = [];
    for (var i = 0; i < 20; i++) {
        var item = {};
        for (var j = 1; j <= 4; j++) {
            var objKey = "Obj" + j;
            
            if (j <= 2) {
                // Minimization objectives (cost, time)
                item[objKey] = Math.random() * 200 + 50;
            } else {
                // Maximization objectives (attractions, neighborhoods)
                item[objKey] = Math.floor(Math.random() * 8) + 1;
            }
        }
        rawData.push(item);
    }
}

function drawBackground(X, Y, inR, outR, arcAngles) {
    // Draw outer circle
    d3.select('#mainSVG').append('circle')
        .attr('cx', X)
        .attr('cy', Y)
        .attr('r', outR + 10)
        .attr('fill', 'none')
        .attr('stroke', '#e74c3c')
        .attr('stroke-width', 3);
    
    // Add subtle background for arcs
    for (var i = 0; i < dim; i++) {
        d3.select('#mainSVG').append('path')
            .attr('d', d3.svg.arc()
                .innerRadius(inR)
                .outerRadius(outR)
                .startAngle(arcAngles[i].start)
                .endAngle(arcAngles[i].end)
            )
            .attr('transform', 'translate(' + X + ',' + Y + ')')
            .attr('fill', '#e8eaf6')
            .attr('stroke', '#c5cae9')
            .attr('stroke-width', 1);
    }
}

function addObjectiveLabels(X, Y, inR, outR, arcAngles) {
    for (var i = 0; i < dim; i++) {
        // Add objective name
        var midAngle = (arcAngles[i].start + arcAngles[i].end) / 2;
        var labelRadius = outR + 40;
        var labelX = X + Math.sin(midAngle) * labelRadius;
        var labelY = Y - Math.cos(midAngle) * labelRadius;
        
        d3.select('#mainSVG').append('text')
            .attr('class', 'objectiveLabel')
            .attr('x', labelX)
            .attr('y', labelY)
            .attr('text-anchor', 'middle')
            .attr('alignment-baseline', 'middle')
            .text(objectiveNames[i]);
        
        // Add min/max labels
        var minMaxLabels = [];
        
        if (i < 2) {
            // Continuous objectives (cost, time)
            minMaxLabels = [
                {
                    value: toMinimize[i] ? minObj[i] : maxObj[i],
                    position: 0.9,
                    prefix: 'min: '
                },
                {
                    value: toMinimize[i] ? maxObj[i] : minObj[i],
                    position: 0.1,
                    prefix: 'max: '
                }
            ];
        } else {
            // Integer objectives (attractions, neighborhoods)
            minMaxLabels = [
                {
                    value: Math.round(toMinimize[i] ? minObj[i] : maxObj[i]),
                    position: 0.9,
                    prefix: 'min: '
                },
                {
                    value: Math.round(toMinimize[i] ? maxObj[i] : minObj[i]),
                    position: 0.1,
                    prefix: 'max: '
                }
            ];
        }
        
        // Add the min/max labels
        minMaxLabels.forEach(function(label) {
            var angle = arcAngles[i].start + (arcAngles[i].end - arcAngles[i].start) * label.position;
            var markerX = X + Math.sin(angle) * (outR - 15);
            var markerY = Y - Math.cos(angle) * (outR - 15);
            
            var displayValue = label.value;
            if (i === 0) displayValue = "R$" + Math.round(displayValue);
            if (i === 1) displayValue = Math.round(displayValue) + "min";
            
            d3.select('#mainSVG').append('text')
                .attr('class', 'objectiveValue')
                .attr('x', markerX)
                .attr('y', markerY)
                .attr('text-anchor', 'middle')
                .attr('alignment-baseline', 'middle')
                .text(label.prefix + displayValue);
        });
    }
}

function calculateSolutionPoints(X, Y, inR, outR, arcAngles) {
    var solutionPoints = [];
    
    // For each solution
    for (var i = 0; i < Data.length; i++) {
        solutionPoints[i] = [];
        
        // For each objective
        for (var j = 0; j < dim; j++) {
            var objKey = "Obj" + (j + 1);
            var normalizedValue = Data[i][objKey];
            
            // Calculate position on arc
            var angle = arcAngles[j].start + (arcAngles[j].end - arcAngles[j].start) * normalizedValue;
            var radius = (inR + outR) / 2; // Middle of the arc
            
            var point = {
                x: X + Math.sin(angle) * radius,
                y: Y - Math.cos(angle) * radius,
                objective: j,
                solutionIndex: i,
                value: normalizedValue
            };
            
            solutionPoints[i][j] = point;
        }
    }
    
    return solutionPoints;
}

function drawConnectingLines(solutionPoints, X, Y) {
    // Create a line function that will generate curves passing through the center
    var lineGenerator = d3.svg.line()
        .x(function(d) { return d.x; })
        .y(function(d) { return d.y; })
        .interpolate('bundle')  // Use bundle interpolation for star-like pattern
        .tension(0.5);  // Adjust tension for more curved lines
    
    // Draw lines connecting ALL objectives for each solution
    for (var i = 0; i < solutionPoints.length; i++) {
        // For each pair of objectives
        for (var j = 0; j < dim; j++) {
            for (var k = j + 1; k < dim; k++) {
                var points = [
                    solutionPoints[i][j],  // Source point
                    {x: X, y: Y},           // Center point for the curve
                    solutionPoints[i][k]   // Target point
                ];
                
                d3.select('#mainSVG').append('path')
                    .attr('class', 'link')
                    .attr('id', 'link' + j + '-' + k + '-idx' + i)
                    .attr('d', lineGenerator(points))
                    .on('mouseover', function() {
                        var linkId = d3.select(this).attr('id');
                        highlightLink(linkId);
                    })
                    .on('mouseout', function() {
                        unhighlightLinks();
                    });
            }
        }
    }
}

function highlightObjective(objectiveIndex) {
    d3.selectAll('.arcPath').attr('opacity', 0.2);
    d3.select('#obj' + objectiveIndex).attr('opacity', 1);
    
    d3.selectAll('.link').each(function() {
        var linkId = d3.select(this).attr('id');
        if (linkId && (linkId.indexOf('-' + objectiveIndex + '-') > -1 || linkId.indexOf(objectiveIndex + '-') === 4)) {
            d3.select(this).attr('class', 'linkOnObj');
        }
    });
}

function unhighlightObjectives() {
    d3.selectAll('.arcPath').attr('opacity', 1);
    d3.selectAll('.linkOnObj').attr('class', 'link');
}

function highlightLink(linkId) {
    d3.select('#' + linkId).attr('class', 'linkOver');
    d3.selectAll('.link:not(.linkOver)').attr('class', 'linkNoOver');
    
    // Extract solution index and objectives from link ID
    var parts = linkId.split('-');
    if (parts.length >= 3) {
        var sourceObj = parseInt(parts[0].replace('link', ''));
        var targetObj = parseInt(parts[1]);
        var solutionIdx = parseInt(parts[2].replace('idx', ''));
        
        // Show tooltip with solution information
        var coordinate = d3.mouse(d3.select('#mainSVG').node());
        showTooltip(coordinate, solutionIdx, sourceObj, targetObj);
    }
}

function unhighlightLinks() {
    d3.selectAll('.linkOver').attr('class', 'link');
    d3.selectAll('.linkNoOver').attr('class', 'link');
    d3.selectAll('.tooltip').remove();
}

function showTooltip(coordinate, solutionIdx, sourceObj, targetObj) {
    // Get original (non-normalized) values
    var sourceKey = "Obj" + (sourceObj + 1);
    var targetKey = "Obj" + (targetObj + 1);
    
    var sourceValue, targetValue;
    
    if (toMinimize[sourceObj]) {
        sourceValue = minObj[sourceObj] + (1 - Data[solutionIdx][sourceKey]) * (maxObj[sourceObj] - minObj[sourceObj]);
    } else {
        sourceValue = minObj[sourceObj] + Data[solutionIdx][sourceKey] * (maxObj[sourceObj] - minObj[sourceObj]);
    }
    
    if (toMinimize[targetObj]) {
        targetValue = minObj[targetObj] + (1 - Data[solutionIdx][targetKey]) * (maxObj[targetObj] - minObj[targetObj]);
    } else {
        targetValue = minObj[targetObj] + Data[solutionIdx][targetKey] * (maxObj[targetObj] - minObj[targetObj]);
    }
    
    // Round integer objectives
    if (sourceObj >= 2) sourceValue = Math.round(sourceValue);
    if (targetObj >= 2) targetValue = Math.round(targetValue);
    
    // Format display values
    var sourceDisplay = sourceValue.toFixed(2);
    var targetDisplay = targetValue.toFixed(2);
    
    if (sourceObj === 0) sourceDisplay = "R$ " + sourceDisplay;
    if (sourceObj === 1) sourceDisplay += " min";
    if (targetObj === 0) targetDisplay = "R$ " + targetDisplay;
    if (targetObj === 1) targetDisplay += " min";
    
    // Create tooltip
    var tooltip = d3.select('#mainSVG').append('g')
        .attr('class', 'tooltip')
        .attr('transform', 'translate(' + (coordinate[0] + 15) + ',' + (coordinate[1] - 20) + ')');
    
    tooltip.append('rect')
        .attr('width', 240)
        .attr('height', 110)
        .attr('rx', 5)
        .attr('ry', 5)
        .attr('fill', 'white')
        .attr('stroke', '#333')
        .attr('stroke-width', 1);
    
    // Add solution number
    tooltip.append('text')
        .attr('x', 10)
        .attr('y', 25)
        .attr('class', 'tooltipTitle')
        .text('Solução #' + (solutionIdx + 1));
    
    // Add source objective
    tooltip.append('text')
        .attr('x', 20)
        .attr('y', 50)
        .attr('class', 'tooltipTitle')
        .text(objectiveNames[sourceObj] + ': ');
    
    tooltip.append('text')
        .attr('x', 150)
        .attr('y', 50)
        .attr('class', 'tooltipVal')
        .text(sourceDisplay);
    
    // Add target objective
    tooltip.append('text')
        .attr('x', 20)
        .attr('y', 75)
        .attr('class', 'tooltipTitle')
        .text(objectiveNames[targetObj] + ': ');
    
    tooltip.append('text')
        .attr('x', 150)
        .attr('y', 75)
        .attr('class', 'tooltipVal')
        .text(targetDisplay);
}

function storeLineCoordinates() {
    var links = d3.selectAll('.link');
    linksCoord = [];
    
    links.each(function(d, i) {
        var path = d3.select(this);
        var id = path.attr('id');
        var length = this.getTotalLength();
        
        linksCoord.push([
            id,
            this.getPointAtLength(0),
            this.getPointAtLength(length)
        ]);
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
    }
    
    d3.selectAll('.selectedLink').each(function() {
        this.parentNode.appendChild(this);
    });
}

function Reset() {
    mainSVG.selectAll('.unSelectedLink').attr('class', 'link');
    mainSVG.selectAll('.selectedLink').attr('class', 'link');
    mainSVG.selectAll('.coloredLink').attr('class', 'link');
}

// Helper methods for d3 selections
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