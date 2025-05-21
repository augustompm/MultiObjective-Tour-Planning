// Dynamic solution selection dial for mobile interfaces

class SolutionDial {
  constructor(containerId, options) {
    this.containerId = containerId;
    this.container = document.getElementById(containerId);
    if (!this.container) return null;

    // Default options
    this.options = {
      radius: 80,
      width: 200,
      height: 200,
      minValue: 1,
      maxValue: 10,
      value: 1,
      labelInterval: 10,
      onChange: null,
      ...options
    };

    // Calculate optimal label interval based on solution count
    if (this.options.maxValue > 30) {
      this.options.labelInterval = Math.max(5, Math.floor(this.options.maxValue / 10));
    } else if (this.options.maxValue > 15) {
      this.options.labelInterval = 5;
    } else {
      this.options.labelInterval = 2;
    }

    // Initialize
    this.createSVG();
    this.drawDial();
    this.setupInteraction();
    
    return this;
  }

  createSVG() {
    // Clear container
    this.container.innerHTML = '';
    
    // Create SVG element
    this.svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    this.svg.setAttribute('width', this.options.width);
    this.svg.setAttribute('height', this.options.height);
    this.svg.setAttribute('class', 'solution-dial');
    this.container.appendChild(this.svg);
    
    // Center point
    this.centerX = this.options.width / 2;
    this.centerY = this.options.height / 2;
  }

  drawDial() {
    // Draw outer circle
    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('cx', this.centerX);
    circle.setAttribute('cy', this.centerY);
    circle.setAttribute('r', this.options.radius);
    circle.setAttribute('class', 'dial-circle');
    circle.setAttribute('stroke-width', '8');
    this.svg.appendChild(circle);
    
    // Draw tick marks
    this.drawTicks();
    
    // Draw needle
    this.needle = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    this.needle.setAttribute('x1', this.centerX);
    this.needle.setAttribute('y1', this.centerY);
    this.needle.setAttribute('class', 'dial-needle');
    this.svg.appendChild(this.needle);
    
    // Draw center circle
    const centerCircle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    centerCircle.setAttribute('cx', this.centerX);
    centerCircle.setAttribute('cy', this.centerY);
    centerCircle.setAttribute('r', '8');
    centerCircle.setAttribute('class', 'dial-center');
    this.svg.appendChild(centerCircle);
    
    // Value text
    this.valueText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    this.valueText.setAttribute('x', this.centerX);
    this.valueText.setAttribute('y', this.centerY + this.options.radius/3);
    this.valueText.setAttribute('class', 'dial-value');
    this.svg.appendChild(this.valueText);
    
    // Set initial position
    this.setValue(this.options.value);
  }

  drawTicks() {
    // Add ticks dynamically based on solution count
    const totalValues = this.options.maxValue - this.options.minValue + 1;
    
    for (let i = this.options.minValue; i <= this.options.maxValue; i++) {
      const angle = this.valueToAngle(i);
      const rads = angle * Math.PI / 180;
      
      // Inner and outer points
      const innerRadius = i % this.options.labelInterval === 0 ? 
                         this.options.radius - 15 : this.options.radius - 5;
      
      const x1 = this.centerX + innerRadius * Math.cos(rads);
      const y1 = this.centerY + innerRadius * Math.sin(rads);
      const x2 = this.centerX + this.options.radius * Math.cos(rads);
      const y2 = this.centerY + this.options.radius * Math.sin(rads);
      
      // Create tick line
      const tick = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      tick.setAttribute('x1', x1);
      tick.setAttribute('y1', y1);
      tick.setAttribute('x2', x2);
      tick.setAttribute('y2', y2);
      tick.setAttribute('class', i % this.options.labelInterval === 0 ? 
                         'dial-tick dial-tick-major' : 'dial-tick');
      this.svg.appendChild(tick);
      
      // Add labels for major ticks
      if (i % this.options.labelInterval === 0 || i === this.options.maxValue) {
        const labelRadius = this.options.radius + 15;
        const labelX = this.centerX + labelRadius * Math.cos(rads);
        const labelY = this.centerY + labelRadius * Math.sin(rads);
        
        const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        label.setAttribute('x', labelX);
        label.setAttribute('y', labelY);
        label.setAttribute('class', 'dial-label');
        label.textContent = i;
        this.svg.appendChild(label);
      }
    }
  }

  valueToAngle(value) {
    // Map value to angle (270 degrees = top)
    const normalized = (value - this.options.minValue) / 
                     (this.options.maxValue - this.options.minValue);
    return -90 + normalized * 360;
  }

  angleToValue(angle) {
    // Convert angle to actual value
    // Adjust angle to be 0-360
    let adjustedAngle = angle;
    if (adjustedAngle < -90) adjustedAngle += 360;
    
    // Map angle back to value
    const normalized = (adjustedAngle + 90) / 360;
    const rawValue = this.options.minValue + 
                   normalized * (this.options.maxValue - this.options.minValue);
    
    // Round to nearest integer
    return Math.round(Math.max(this.options.minValue, 
                             Math.min(this.options.maxValue, rawValue)));
  }

  setupInteraction() {
    let isDragging = false;
    
    // Touch & mouse events
    this.svg.addEventListener('mousedown', this.startDrag.bind(this));
    this.svg.addEventListener('touchstart', this.startDrag.bind(this));
    document.addEventListener('mousemove', this.doDrag.bind(this));
    document.addEventListener('touchmove', this.doDrag.bind(this));
    document.addEventListener('mouseup', this.endDrag.bind(this));
    document.addEventListener('touchend', this.endDrag.bind(this));
    
    // Click events for direct selection
    this.svg.addEventListener('click', this.handleClick.bind(this));
  }
  
  startDrag(event) {
    event.preventDefault();
    this.isDragging = true;
    this.svg.setAttribute('class', 'solution-dial dragging');
  }
  
  doDrag(event) {
    if (!this.isDragging) return;
    event.preventDefault();
    
    // Get coordinate, handling both mouse and touch
    const pointer = event.touches ? event.touches[0] : event;
    const rect = this.svg.getBoundingClientRect();
    const x = pointer.clientX - rect.left - this.centerX;
    const y = pointer.clientY - rect.top - this.centerY;
    
    // Calculate angle in degrees
    let angle = Math.atan2(y, x) * 180 / Math.PI;
    
    this.setValue(this.angleToValue(angle));
  }
  
  endDrag() {
    this.isDragging = false;
    this.svg.setAttribute('class', 'solution-dial');
  }
  
  handleClick(event) {
    // Direct click on the dial
    const rect = this.svg.getBoundingClientRect();
    const x = event.clientX - rect.left - this.centerX;
    const y = event.clientY - rect.top - this.centerY;
    
    // Only process if it's not too close to center (to avoid jumps)
    const distFromCenter = Math.sqrt(x*x + y*y);
    if (distFromCenter > 20) {
      // Calculate angle in degrees
      let angle = Math.atan2(y, x) * 180 / Math.PI;
      this.setValue(this.angleToValue(angle));
    }
  }

  setValue(value) {
    // Validate input
    const validValue = Math.max(this.options.minValue, 
                              Math.min(this.options.maxValue, value));
    
    // Set internal value
    this.value = validValue;
    
    // Position needle
    const angle = this.valueToAngle(this.value);
    const rads = angle * Math.PI / 180;
    const x2 = this.centerX + this.options.radius * Math.cos(rads);
    const y2 = this.centerY + this.options.radius * Math.sin(rads);
    
    this.needle.setAttribute('x2', x2);
    this.needle.setAttribute('y2', y2);
    
    // Update text
    this.valueText.textContent = this.value;
    
    // Call callback if provided
    if (typeof this.options.onChange === 'function') {
      this.options.onChange(this.value);
    }
    
    return this;
  }
  
  getValue() {
    return this.value;
  }
}

// Create solution dial when requested
function createSolutionDial(containerId, options) {
  return new SolutionDial(containerId, options);
}