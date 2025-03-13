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
}