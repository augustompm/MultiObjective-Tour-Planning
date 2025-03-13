// File: static/js/linkedArc.js

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
    
    // Criar rótulos com valores concretos em vez de min/max
    var labelValues = [
        ["R$0", "R$60", "R$120", "R$179", "R$239"], // Custo Total - valores específicos
        ["60min", "221min", "382min", "543min", "704min"], // Tempo Total - valores específicos
        ["1", "2", "3", "4", "5", "6", "7", "8"], // Atrações - valores inteiros
        ["1", "2", "3", "4", "5", "6"] // Bairros - valores inteiros
    ];
    
    // Find the min and max of each objective
    for (i = 0; i < dim; i++) {
        var objKey = "Obj" + (i + 1);
        var temp = Data.map(function(obj) { return obj[objKey]; });
        minObj[i] = Math.min.apply(null, temp);
        maxObj[i] = Math.max.apply(null, temp);
        
        // Para objetivos de maximização (Atrações e Bairros),
        // os valores são armazenados como negativos
        if (i >= 2) {
            for (z = 0; z < temp.length; z++) {
                // Garantir que usamos o valor absoluto para normalização
                Data[z][objKey] = -temp[z]; // Converter para positivo para normalização
            }
            
            // Ajustar min/max para positivo
            var tempMin = -maxObj[i];
            var tempMax = -minObj[i];
            minObj[i] = tempMin;
            maxObj[i] = tempMax;
        }
    }
    
    // Normalizar dados para [0,1]
    for (i = 0; i < dim; i++) {
        var objKey = "Obj" + (i + 1);
        for (z = 0; z < Data.length; z++) {
            var value;
            
            if (i >= 2) {
                // Objetivos de maximização já foram convertidos para positivo acima
                value = Data[z][objKey];
            } else {
                value = Data[z][objKey];
            }
            
            // Normalização para [0,1] - importante: min é 0 e max é 1
            Data[z][objKey] = (value - minObj[i]) / (maxObj[i] - minObj[i]);
        }
    }
    
    var min = 0;
    var max = 1;
    var inR = (doc.clientHeight - 150) / 2; // Inner Radius
    var X = doc.clientWidth / 2;
    var Y = doc.clientHeight / 2;
    var usedEnv = 360; // The usable Environment - full circle
    var arcAngle = usedEnv / dim; // The angle of each Arc
    var Angles = [];
    var arcs = [];

    // Calculate angles for a complete circle with gaps
    for (i = 0; i < dim; i++) {
        // Create gaps between objectives
        var startAngle = i * (360/dim) + 5;  // Add a small gap (5 degrees)
        var endAngle = (i+1) * (360/dim) - 5; // Subtract a small gap (5 degrees)
        Angles[i] = [startAngle, endAngle];
        arcs[i] = d3.svg.arc()
            .innerRadius(inR)
            .outerRadius(inR + 25)  // Make the arcs thicker
            .startAngle(Angles[i][0] * (Math.PI / 180))
            .endAngle(Angles[i][1] * (Math.PI / 180));
    }
    
    // Draw the outer circle first
    d3.select('#mainSVG').append('circle')
        .attr('cx', X)
        .attr('cy', Y)
        .attr('r', inR + 40)
        .attr('fill', 'none')
        .attr('stroke', '#e74c3c')
        .attr('stroke-width', 3);

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
        .attr('fill', '#3b5ddb') // Blue to match the example image
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
            });
        })
        .on('mouseout', function(d, i) {
            mainSVG.selectAll('.arcPath').attr('opacity', 1);
            mainSVG.selectAll('.linkOnObj').attr('class', 'link');
        });

    // Adicionar rótulos principais para cada objetivo
    for (i = 0; i < dim; i++) {
        // Add objective name labels outside the arcs
        var middleAngle = ((Angles[i][0] + Angles[i][1]) / 2) * (Math.PI / 180);
        var labelRadius = inR + 60;  // Place labels further outside
        var labelX = X + Math.sin(middleAngle) * labelRadius;
        var labelY = Y - Math.cos(middleAngle) * labelRadius;
        
        d3.select('#mainSVG').append('text')
            .attr('class', 'objectiveLabel')
            .attr('x', labelX)
            .attr('y', labelY)
            .attr('text-anchor', 'middle')
            .attr('alignment-baseline', 'middle')
            .attr('font-weight', 'bold')
            .attr('font-size', '16px')
            .text(objectiveNames[i]);
        
        // Calcular posições para marcadores de valores nos arcos
        // Para cada arco, adicionaremos 2-3 marcadores principais
        var numMarkers = (i < 2) ? 5 : (i === 2 ? 8 : 6); // Número de marcadores baseado no range de valores
        var markers = [];
        
        // Determinar os valores e posições dos marcadores
        for (j = 0; j < numMarkers; j++) {
            var markerPos;
            
            if (i < 2) {
                // Custo e Tempo (minimização)
                markerPos = j / (numMarkers - 1); // Valores mapeados para [0,1]
            } else {
                // Atrações e Bairros (maximização)
                markerPos = j / (numMarkers - 1); // Valores inteiros 1-8 ou 1-6
            }
            
            markers.push({
                position: markerPos,
                label: labelValues[i][j]
            });
        }
        
        // Adicionar marcadores (min, max, valores intermediários)
        for (j = 0; j < markers.length; j++) {
            // Determinar posição no arco
            var markerAnglePercent;
            if (i < 2) {
                // Para minimização (Custo, Tempo), valores menores (melhores) ficam à direita do arco
                markerAnglePercent = 1 - markers[j].position;
            } else {
                // Para maximização (Atrações, Bairros), valores maiores (melhores) ficam à direita do arco
                markerAnglePercent = markers[j].position;
            }
            
            var markerAngle = Angles[i][0] + markerAnglePercent * (Angles[i][1] - Angles[i][0]);
            markerAngle = markerAngle * (Math.PI / 180);
            
            // Posição do texto
            var markerX = X + Math.sin(markerAngle) * (inR - 15);
            var markerY = Y - Math.cos(markerAngle) * (inR - 15);
            
            // Se for o primeiro ou último marcador, adicione um pouco mais de espaço
            if (j === 0 || j === markers.length - 1) {
                if (j === 0) {
                    // Valor mínimo - ajustar posição
                    if (i < 2) {
                        markerX = X + Math.sin(markerAngle) * (inR + 35);
                        markerY = Y - Math.cos(markerAngle) * (inR + 35);
                    } else {
                        markerX = X + Math.sin(markerAngle) * (inR - 35);
                        markerY = Y - Math.cos(markerAngle) * (inR - 35);
                    }
                } else {
                    // Valor máximo - ajustar posição
                    if (i < 2) {
                        markerX = X + Math.sin(markerAngle) * (inR - 35);
                        markerY = Y - Math.cos(markerAngle) * (inR - 35);
                    } else {
                        markerX = X + Math.sin(markerAngle) * (inR + 35);
                        markerY = Y - Math.cos(markerAngle) * (inR + 35);
                    }
                }
                
                // Adicionar prefixo min/max para o primeiro e último valores
                var prefix = (j === 0) ? "min: " : "max: ";
                
                // Para minimização, inverter prefixos
                if (i < 2) {
                    prefix = (j === 0) ? "max: " : "min: ";
                }
                
                d3.select('#mainSVG').append('text')
                    .attr('class', 'objectiveValue')
                    .attr('x', markerX)
                    .attr('y', markerY)
                    .attr('text-anchor', 'middle')
                    .attr('alignment-baseline', 'middle')
                    .attr('font-size', '10px')
                    .text(prefix + markers[j].label);
            }
        }
    }

    // Map the objectives' interval on the arcs
    var tempPath = [];
    for (i = 0; i < arcPaths[0].length; i++) {
        tempPath[i] = d3.select(arcPaths[0][i]).node();
    }
    
    // Define as escalas para mapear valores [0,1] para pontos no arco
    var scales = [];
    for (i = 0; i < dim; i++) {
        // Para cada arco, criar uma escala para o seu comprimento
        var pathLength = tempPath[i].getTotalLength();
        if (i < 2) {
            // Para minimização, inverter a escala (maior valor = início do arco)
            scales[i] = d3.scale.linear().domain([0, 1]).range([pathLength, 0]);
        } else {
            // Para maximização, escala normal (maior valor = fim do arco)
            scales[i] = d3.scale.linear().domain([0, 1]).range([0, pathLength]);
        }
    }

    // Finding the point on the arcs
    var normVal = []; // Finding the normal value of each items on the path before calculating of coordinate
    var Points = []; // The Points of each value on the Arcs
    for (i = 0; i < Data.length; i++) {
        normVal[i] = new Array(dim);
        Points[i] = new Array(dim);
        for (j = 0; j < dim; j++) {
            var objKey = "Obj" + (j + 1);
            normVal[i][j] = scales[j](Data[i][objKey]);
            Points[i][j] = tempPath[j].getPointAtLength(normVal[i][j]);
            Points[i][j].x += X;
            Points[i][j].y += Y;
        }
    }

    // Drawing Connections
    var lineFunc = d3.svg.line()
        .x(function(d) { return d.x; })
        .y(function(d) { return d.y; })
        .interpolate('basis'); // Use basis interpolation for smoother curves
    
    // Draw all possible connections between objectives for all solutions
    for (j1 = 0; j1 < dim; j1++) {
        for (j2 = 0; j2 < dim; j2++) {
            if (j1 != j2) { // Don't connect an objective to itself
                for (i = 0; i < Points.length; i++) {
                    // Create an array of points with control points for smoother curves
                    var tempPoints = [
                        Points[i][j1],
                        {x: X + (Points[i][j1].x - X) * 0.3, y: Y + (Points[i][j1].y - Y) * 0.3},
                        {x: X + (Points[i][j2].x - X) * 0.3, y: Y + (Points[i][j2].y - Y) * 0.3},
                        Points[i][j2]
                    ];

                    d3.select('#mainSVG').append('path')
                        .attr('class', 'link')
                        .attr('id', function(d, idx) {
                            return 'link' + j1.toString() + "-" + j2.toString() + "-idx" + i.toString();
                        })
                        .attr('d', lineFunc(tempPoints))
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
                            var originalValue1, originalValue2;
                            
                            if (O1 < 2) {
                                // Custo ou tempo (minimização)
                                originalValue1 = Data[solutionIdx][objKey1] * (maxObj[O1] - minObj[O1]) + minObj[O1];
                            } else {
                                // Atrações ou bairros (maximização)
                                originalValue1 = Math.round(Data[solutionIdx][objKey1] * (maxObj[O1] - minObj[O1]) + minObj[O1]);
                            }
                            
                            if (O2 < 2) {
                                // Custo ou tempo (minimização)
                                originalValue2 = Data[solutionIdx][objKey2] * (maxObj[O2] - minObj[O2]) + minObj[O2];
                            } else {
                                // Atrações ou bairros (maximização)
                                originalValue2 = Math.round(Data[solutionIdx][objKey2] * (maxObj[O2] - minObj[O2]) + minObj[O2]);
                            }
                            
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
}