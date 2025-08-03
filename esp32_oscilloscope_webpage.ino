#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
// #include "voltage_measurement.h" // Make sure this file exists

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
Adafruit_ADS1115 ads; // Using ADS1115 directly for now
// VoltageMeasurement voltmeter; // Uncomment if you have the custom class

// Oscilloscope settings
const int BUFFER_SIZE = 500;
float voltageBuffer[BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastSampleTime = 0;
int sampleRate = 2000; // samples per second
bool isRunning = true;
bool triggerMode = false;
float triggerLevel = 2.5; // 2.5V trigger level
bool triggered = false;

// Auto-scaling variables
bool autoTimeScale = true;
float detectedFrequency = 0;
float lastTimeScale = 0.5;

// Voltage measurement functions (replace with your custom class if available)
float readVoltage() {
  int16_t adc = ads.readADC_SingleEnded(0);
  float voltage = ads.computeVolts(adc);
  return voltage * 10.0; // Adjust multiplier based on your voltage divider
}

int16_t getAnalogValue() {
  return ads.readADC_SingleEnded(0);
}

float getCalibrationFactor() {
  return 10.0; // Return your calibration factor
}

void setup() {
  Serial.begin(115200);
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }
  ads.setGain(GAIN_ONE); // 1x gain +/- 4.096V
  
  // Initialize voltage measurement
  // voltmeter.begin(); // Uncomment if using custom class
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/status", handleStatus);
  
  // Enable CORS for all routes
  server.enableCORS(true);
  
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  
  // Sample voltage at specified rate
  if (isRunning && (micros() - lastSampleTime) >= (1000000 / sampleRate)) {
    float voltage = readVoltage(); // voltmeter.readVoltage();
    
    // Simple trigger logic
    if (triggerMode && !triggered) {
      static float lastVoltage = 0;
      if (lastVoltage < triggerLevel && voltage >= triggerLevel) {
        triggered = true;
        bufferIndex = 0; // Reset buffer on trigger
      }
      lastVoltage = voltage;
      if (!triggered) return; // Skip sampling until triggered
    }
    
    voltageBuffer[bufferIndex] = voltage;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
    lastSampleTime = micros();
    
    // Reset trigger after buffer is full
    if (triggerMode && bufferIndex == 0 && triggered) {
      triggered = false;
    }
  }
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Web Oscilloscope</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #1a1a1a; 
            color: #fff; 
        }
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
        }
        h1 { 
            text-align: center; 
            color: #00ff00; 
            text-shadow: 0 0 10px #00ff00;
        }
        .controls { 
            background: #2a2a2a; 
            padding: 20px; 
            border-radius: 10px; 
            margin-bottom: 20px;
            display: flex;
            gap: 20px;
            flex-wrap: wrap;
            align-items: center;
        }
        .control-group {
            display: flex;
            flex-direction: column;
            gap: 5px;
            min-width: 120px;
        }
        label { 
            font-weight: bold; 
            color: #ccc;
        }
        select, button, input { 
            background: #333; 
            color: #fff; 
            border: 1px solid #555; 
            padding: 8px 12px; 
            border-radius: 5px; 
        }
        input[type="range"] {
            padding: 4px;
        }
        input[type="checkbox"] {
            transform: scale(1.2);
            margin: 0 5px;
        }
        #triggerValue {
            font-size: 0.9em;
            color: #00ff00;
            text-align: center;
        }
        .auto-indicator {
            font-size: 0.8em;
            color: #ffaa00;
            text-align: center;
        }
        button { 
            cursor: pointer; 
            transition: background 0.3s;
        }
        button:hover { 
            background: #555; 
        }
        .start { background: #006600; }
        .stop { background: #660000; }
        .scope-container { 
            background: #000; 
            border: 2px solid #333; 
            border-radius: 10px; 
            padding: 20px; 
            position: relative;
        }
        canvas { 
            width: 100%; 
            height: 400px; 
            background: #001100; 
            border: 1px solid #004400;
        }
        .info { 
            margin-top: 20px; 
            background: #2a2a2a; 
            padding: 15px; 
            border-radius: 10px;
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        .info-item { 
            background: #333; 
            padding: 10px; 
            border-radius: 5px;
        }
        .info-label { 
            color: #888; 
            font-size: 0.9em;
        }
        .info-value { 
            font-size: 1.2em; 
            font-weight: bold; 
            color: #00ff00;
        }
        .auto-enabled {
            color: #ffaa00 !important;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Web Oscilloscope</h1>
        
        <div class="controls">
            <div class="control-group">
                <label>Sample Rate:</label>
                <select id="sampleRate">
                    <option value="500">500 Hz</option>
                    <option value="1000">1000 Hz</option>
                    <option value="2000" selected>2000 Hz</option>
                    <option value="4000">4000 Hz</option>
                    <option value="8000">8000 Hz (Max)</option>
                </select>
            </div>
            
            <div class="control-group">
                <label>Time Scale:</label>
                <select id="timeScale">
                    <option value="2">2 sec</option>
                    <option value="1">1 sec</option>
                    <option value="0.5" selected>0.5 sec</option>
                    <option value="0.2">0.2 sec</option>
                    <option value="0.1">0.1 sec</option>
                    <option value="0.05">0.05 sec</option>
                    <option value="0.02">0.02 sec</option>
                    <option value="0.01">0.01 sec</option>
                    <option value="0.005">0.005 sec</option>
                    <option value="0.002">0.002 sec</option>
                    <option value="0.001">0.001 sec</option>
                </select>
                <div class="auto-indicator">
                    <input type="checkbox" id="autoTimeScale" checked>
                    <label for="autoTimeScale">Auto Scale</label>
                </div>
            </div>
            
            <div class="control-group">
                <label>Voltage Scale:</label>
                <select id="voltageScale">
                    <option value="0.5">0.5V/div</option>
                    <option value="1">1V/div</option>
                    <option value="2">2V/div</option>
                    <option value="5" selected>5V/div</option>
                    <option value="10">10V/div</option>
                    <option value="20">20V/div</option>
                </select>
            </div>
            
            <div class="control-group">
                <label>Trigger:</label>
                <select id="triggerMode">
                    <option value="auto" selected>Auto</option>
                    <option value="normal">Normal</option>
                </select>
            </div>
            
            <div class="control-group">
                <label>Trigger Level:</label>
                <input type="range" id="triggerLevel" min="0" max="45" value="2.5" step="0.1">
                <span id="triggerValue">2.5V</span>
            </div>
            
            <button id="startStop" class="start">Start</button>
            <button id="trigger">Single</button>
        </div>
        
        <div class="scope-container">
            <canvas id="oscilloscope" width="800" height="400"></canvas>
        </div>
        
        <div class="info">
            <div class="info-item">
                <div class="info-label">Current Voltage</div>
                <div class="info-value" id="currentVoltage">0.00 V</div>
            </div>
            <div class="info-item">
                <div class="info-label">Peak-to-Peak</div>
                <div class="info-value" id="peakToPeak">0.00 V</div>
            </div>
            <div class="info-item">
                <div class="info-label">Frequency</div>
                <div class="info-value" id="frequency">-- Hz</div>
            </div>
            <div class="info-item">
                <div class="info-label">Waveform Type</div>
                <div class="info-value" id="waveformType">Analyzing...</div>
            </div>
            <div class="info-item">
                <div class="info-label">ADC Reading</div>
                <div class="info-value" id="adcReading">0</div>
            </div>
            <div class="info-item">
                <div class="info-label">Effective Time Scale</div>
                <div class="info-value" id="effectiveTimeScale">0.5 sec</div>
            </div>
        </div>
    </div>

    <script>
        let canvas = document.getElementById('oscilloscope');
        let ctx = canvas.getContext('2d');
        let data = [];
        let isRunning = true;
        let timeScale = 0.5;
        let voltageScale = 5;
        let autoTimeScale = true;
        let lastFrequency = 0;
        let sampleRate = 2000;
        
        // Set canvas resolution
        canvas.width = 800;
        canvas.height = 400;
        
        function calculateOptimalTimeScale(frequency) {
            if (frequency <= 0) return timeScale;
            
            // Target: show 2-5 complete cycles on screen
            let cyclesPerSecond = frequency;
            let targetCycles = 3; // Show 3 complete cycles
            let optimalTimeScale = targetCycles / cyclesPerSecond;
            
            // Round to standard oscilloscope time divisions
            let standardTimeScales = [
                0.001, 0.002, 0.005, 
                0.01, 0.02, 0.05, 
                0.1, 0.2, 0.5, 
                1, 2, 5
            ];
            
            // Find the closest standard time scale
            let closest = standardTimeScales[0];
            let minDiff = Math.abs(optimalTimeScale - closest);
            
            for (let scale of standardTimeScales) {
                let diff = Math.abs(optimalTimeScale - scale);
                if (diff < minDiff) {
                    minDiff = diff;
                    closest = scale;
                }
            }
            
            // Prefer slightly longer time scales for better visibility
            let index = standardTimeScales.indexOf(closest);
            if (index < standardTimeScales.length - 1 && 
                standardTimeScales[index + 1] / optimalTimeScale < 2) {
                closest = standardTimeScales[index + 1];
            }
            
            return closest;
        }
        
        function updateTimeScaleDisplay(effectiveTimeScale) {
            document.getElementById('effectiveTimeScale').textContent = effectiveTimeScale.toFixed(3) + ' sec';
            
            // Update the dropdown to show current effective time scale if auto is enabled
            if (autoTimeScale) {
                let timeScaleSelect = document.getElementById('timeScale');
                timeScaleSelect.style.color = '#ffaa00'; // Orange to indicate auto mode
                
                // Find closest option
                let options = Array.from(timeScaleSelect.options);
                let closest = options[0];
                let minDiff = Math.abs(parseFloat(closest.value) - effectiveTimeScale);
                
                for (let option of options) {
                    let diff = Math.abs(parseFloat(option.value) - effectiveTimeScale);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closest = option;
                    }
                }
                
                timeScaleSelect.value = closest.value;
            } else {
                document.getElementById('timeScale').style.color = '#fff';
            }
        }
        
        function drawGrid() {
            ctx.strokeStyle = '#003300';
            ctx.lineWidth = 1;
            
            // Vertical grid lines (time divisions)
            for (let x = 0; x <= canvas.width; x += canvas.width / 10) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }
            
            // Horizontal grid lines (voltage divisions)
            for (let y = 0; y <= canvas.height; y += canvas.height / 8) {
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(canvas.width, y);
                ctx.stroke();
            }
            
            // Center line (0V reference)
            ctx.strokeStyle = '#006600';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(0, canvas.height / 2);
            ctx.lineTo(canvas.width, canvas.height / 2);
            ctx.stroke();
            
            // Trigger level line
            if (document.getElementById('triggerMode').value === 'normal') {
                ctx.strokeStyle = '#ffff00';
                ctx.lineWidth = 1;
                ctx.setLineDash([5, 5]);
                let triggerY = canvas.height / 2 - (parseFloat(document.getElementById('triggerLevel').value) / voltageScale) * (canvas.height / 8);
                ctx.beginPath();
                ctx.moveTo(0, triggerY);
                ctx.lineTo(canvas.width, triggerY);
                ctx.stroke();
                ctx.setLineDash([]);
            }
        }
        
        function drawWaveform() {
            if (data.length < 2) return;
            
            // Calculate effective sample rate for current time scale
            let effectiveTimeScale = autoTimeScale ? calculateOptimalTimeScale(lastFrequency) : timeScale;
            let totalTime = effectiveTimeScale;
            let samplesNeeded = Math.floor(totalTime * sampleRate);
            samplesNeeded = Math.min(samplesNeeded, data.length);
            
            // Update display
            updateTimeScaleDisplay(effectiveTimeScale);
            
            ctx.strokeStyle = '#00ff00';
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            // Draw only the samples that fit in the current time scale
            let startIndex = Math.max(0, data.length - samplesNeeded);
            let displayData = data.slice(startIndex);
            
            for (let i = 0; i < displayData.length - 1; i++) {
                let x1 = (i / (displayData.length - 1)) * canvas.width;
                let y1 = canvas.height / 2 - (displayData[i] / voltageScale) * (canvas.height / 8);
                let x2 = ((i + 1) / (displayData.length - 1)) * canvas.width;
                let y2 = canvas.height / 2 - (displayData[i + 1] / voltageScale) * (canvas.height / 8);
                
                if (i === 0) ctx.moveTo(x1, y1);
                ctx.lineTo(x2, y2);
            }
            ctx.stroke();
        }
        
        function analyzeWaveform() {
            if (data.length < 10) return;
            
            let min = Math.min(...data);
            let max = Math.max(...data);
            let peakToPeak = max - min;
            let current = data[data.length - 1];
            
            document.getElementById('currentVoltage').textContent = current.toFixed(2) + ' V';
            document.getElementById('peakToPeak').textContent = peakToPeak.toFixed(2) + ' V';
            
            // Enhanced frequency detection
            let crossings = 0;
            let midpoint = (max + min) / 2;
            let threshold = peakToPeak * 0.1; // 10% threshold to avoid noise
            
            for (let i = 1; i < data.length; i++) {
                if ((data[i-1] < midpoint - threshold && data[i] >= midpoint + threshold) || 
                    (data[i-1] > midpoint + threshold && data[i] <= midpoint - threshold)) {
                    crossings++;
                }
            }
            
            // Calculate frequency based on current effective time scale
            let effectiveTimeScale = autoTimeScale ? calculateOptimalTimeScale(lastFrequency) : timeScale;
            let totalTime = (data.length / sampleRate);
            let frequency = (crossings / 2) / totalTime;
            
            if (frequency > 0 && frequency < sampleRate / 2) { // Nyquist limit
                lastFrequency = frequency;
                document.getElementById('frequency').textContent = frequency.toFixed(1) + ' Hz';
            } else {
                document.getElementById('frequency').textContent = '-- Hz';
            }
            
            // Waveform type detection
            let waveformType = detectWaveformType(data, min, max);
            document.getElementById('waveformType').textContent = waveformType;
        }
        
        function detectWaveformType(signal, min, max) {
            if (signal.length < 20) return 'Analyzing...';
            
            let range = max - min;
            if (range < 0.1) return 'DC / Flat';
            
            // Check for PWM (square wave) - very common in embedded systems
            let highCount = 0;
            let lowCount = 0;
            let midCount = 0;
            let midpoint = (max + min) / 2;
            let threshold = range * 0.2; // 20% threshold
            
            for (let val of signal) {
                if (val > midpoint + threshold) highCount++;
                else if (val < midpoint - threshold) lowCount++;
                else midCount++;
            }
            
            let extremeRatio = (highCount + lowCount) / signal.length;
            if (extremeRatio > 0.8) {
                let dutyCycle = (highCount / (highCount + lowCount) * 100).toFixed(0);
                return `PWM (${dutyCycle}% duty)`;
            }
            
            // Check for sine wave
            let sineCorrelation = 0;
            let samples = Math.min(signal.length, 100);
            for (let i = 0; i < samples; i++) {
                let phase = (i / samples) * 2 * Math.PI;
                let expected = midpoint + (range / 2) * Math.sin(phase);
                let error = Math.abs(signal[i] - expected) / range;
                if (error < 0.3) sineCorrelation++;
            }
            
            if (sineCorrelation / samples > 0.7) return 'Sine Wave';
            
            // Check for triangle/sawtooth
            let slopes = [];
            for (let i = 1; i < Math.min(signal.length, 50); i++) {
                slopes.push(signal[i] - signal[i-1]);
            }
            let avgSlope = slopes.reduce((a, b) => a + Math.abs(b), 0) / slopes.length;
            let slopeVariance = slopes.reduce((a, b) => a + Math.pow(Math.abs(b) - avgSlope, 2), 0) / slopes.length;
            
            if (slopeVariance < avgSlope * 0.5 && avgSlope > range * 0.01) {
                return 'Triangle/Ramp';
            }
            
            // Check for noise/random
            if (midCount > signal.length * 0.6) return 'Noise/Random';
            
            return 'Complex/Other';
        }
        
        function updateDisplay() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            drawGrid();
            drawWaveform();
            analyzeWaveform();
        }
        
        function fetchData() {
            if (!isRunning) return;
            
            fetch('/data')
                .then(response => response.json())
                .then(newData => {
                    data = newData;
                    updateDisplay();
                })
                .catch(error => console.error('Error fetching data:', error));
                
            // Also fetch status info
            fetch('/status')
                .then(response => response.json())
                .then(status => {
                    document.getElementById('adcReading').textContent = status.adc || '0';
                    sampleRate = status.sampleRate || 2000;
                })
                .catch(error => console.error('Error fetching status:', error));
        }
        
        // Controls
        document.getElementById('startStop').addEventListener('click', function() {
            isRunning = !isRunning;
            this.textContent = isRunning ? '⏸ Stop' : '▶ Start';
            this.className = isRunning ? 'stop' : 'start';
            
            if (isRunning) {
                fetchData();
            }
        });
        
        document.getElementById('trigger').addEventListener('click', function() {
            fetchData();
        });
        
        document.getElementById('sampleRate').addEventListener('change', function() {
            let rate = parseInt(this.value);
            sampleRate = rate;
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({sampleRate: rate})
            });
        });
        
        document.getElementById('timeScale').addEventListener('change', function() {
            timeScale = parseFloat(this.value);
            if (!autoTimeScale) {
                updateDisplay();
            }
        });
        
        document.getElementById('autoTimeScale').addEventListener('change', function() {
            autoTimeScale = this.checked;
            if (!autoTimeScale) {
                timeScale = parseFloat(document.getElementById('timeScale').value);
            }
            updateDisplay();
        });
        
        document.getElementById('voltageScale').addEventListener('change', function() {
            voltageScale = parseFloat(this.value);
            updateDisplay();
        });
        
        document.getElementById('triggerLevel').addEventListener('input', function() {
            document.getElementById('triggerValue').textContent = this.value + 'V';
            updateDisplay();
        });
        
        document.getElementById('triggerMode').addEventListener('change', function() {
            let isTriggerMode = this.value === 'normal';
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    triggerMode: isTriggerMode,
                    triggerLevel: parseFloat(document.getElementById('triggerLevel').value)
                })
            });
            updateDisplay();
        });
        
        // Start fetching data
        setInterval(fetchData, 50); // Update display at 20 FPS
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handleData() {
  DynamicJsonDocument doc(8192);
  JsonArray array = doc.to<JsonArray>();
  
  // Send current buffer data
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int index = (bufferIndex + i) % BUFFER_SIZE;
    array.add(voltageBuffer[index]);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  DynamicJsonDocument doc(1024);
  
  // Get current readings for display
  float currentVoltage = readVoltage(); // voltmeter.readVoltage();
  int16_t adcValue = getAnalogValue(); // voltmeter.getAnalogValue();
  float calFactor = getCalibrationFactor(); // voltmeter.getCalibrationFactor();
  
  doc["voltage"] = currentVoltage;
  doc["adc"] = adcValue;
  doc["calibration"] = calFactor;
  doc["sampleRate"] = sampleRate;
  doc["triggerMode"] = triggerMode;
  doc["triggerLevel"] = triggerLevel;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("sampleRate")) {
      sampleRate = doc["sampleRate"];
      Serial.print("Sample rate changed to: ");
      Serial.println(sampleRate);
    }
    
    if (doc.containsKey("triggerMode")) {
      triggerMode = doc["triggerMode"];
      triggered = false; // Reset trigger state
      Serial.print("Trigger mode: ");
      Serial.println(triggerMode ? "Normal" : "Auto");
    }
    
    if (doc.containsKey("triggerLevel")) {
      triggerLevel = doc["triggerLevel"];
      Serial.print("Trigger level: ");
      Serial.println(triggerLevel);
    }
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}