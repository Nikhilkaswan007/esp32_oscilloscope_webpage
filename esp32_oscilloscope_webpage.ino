#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "voltage_measurement.h"

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
VoltageMeasurement voltmeter;

// Oscilloscope settings
const int BUFFER_SIZE = 500;
float voltageBuffer[BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastSampleTime = 0;
int sampleRate = 1000; // samples per second (adjustable)
bool isRunning = true;

void setup() {
  Serial.begin(115200);
  
  // Initialize voltage measurement
  voltmeter.begin();
  
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
  server.begin();
  
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  
  // Sample voltage at specified rate
  if (isRunning && (micros() - lastSampleTime) >= (1000000 / sampleRate)) {
    float voltage = voltmeter.readVoltage();
    voltageBuffer[bufferIndex] = voltage;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
    lastSampleTime = micros();
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
    </style>
</head>
<body>
    <div class="container">
        <h1>🔬 Web Oscilloscope</h1>
        
        <div class="controls">
            <div class="control-group">
                <label>Sample Rate:</label>
                <select id="sampleRate">
                    <option value="100">100 Hz</option>
                    <option value="500">500 Hz</option>
                    <option value="1000" selected>1000 Hz</option>
                    <option value="2000">2000 Hz</option>
                    <option value="5000">5000 Hz</option>
                </select>
            </div>
            
            <div class="control-group">
                <label>Time Scale:</label>
                <select id="timeScale">
                    <option value="1">1 sec</option>
                    <option value="0.5" selected>0.5 sec</option>
                    <option value="0.2">0.2 sec</option>
                    <option value="0.1">0.1 sec</option>
                    <option value="0.05">0.05 sec</option>
                </select>
            </div>
            
            <div class="control-group">
                <label>Voltage Scale:</label>
                <select id="voltageScale">
                    <option value="1">1V/div</option>
                    <option value="2">2V/div</option>
                    <option value="5" selected>5V/div</option>
                    <option value="10">10V/div</option>
                    <option value="20">20V/div</option>
                </select>
            </div>
            
            <button id="startStop" class="start">▶ Start</button>
            <button id="trigger">⚡ Single</button>
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
        </div>
    </div>

    <script>
        let canvas = document.getElementById('oscilloscope');
        let ctx = canvas.getContext('2d');
        let data = [];
        let isRunning = true;
        let timeScale = 0.5;
        let voltageScale = 5;
        
        // Set canvas resolution
        canvas.width = 800;
        canvas.height = 400;
        
        function drawGrid() {
            ctx.strokeStyle = '#003300';
            ctx.lineWidth = 1;
            
            // Vertical grid lines
            for (let x = 0; x <= canvas.width; x += canvas.width / 10) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }
            
            // Horizontal grid lines
            for (let y = 0; y <= canvas.height; y += canvas.height / 8) {
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(canvas.width, y);
                ctx.stroke();
            }
            
            // Center line
            ctx.strokeStyle = '#006600';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(0, canvas.height / 2);
            ctx.lineTo(canvas.width, canvas.height / 2);
            ctx.stroke();
        }
        
        function drawWaveform() {
            if (data.length < 2) return;
            
            ctx.strokeStyle = '#00ff00';
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            for (let i = 0; i < data.length - 1; i++) {
                let x1 = (i / data.length) * canvas.width;
                let y1 = canvas.height / 2 - (data[i] / voltageScale) * (canvas.height / 8);
                let x2 = ((i + 1) / data.length) * canvas.width;
                let y2 = canvas.height / 2 - (data[i + 1] / voltageScale) * (canvas.height / 8);
                
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
            
            // Simple frequency detection
            let crossings = 0;
            let midpoint = (max + min) / 2;
            for (let i = 1; i < data.length; i++) {
                if ((data[i-1] < midpoint && data[i] >= midpoint) || 
                    (data[i-1] > midpoint && data[i] <= midpoint)) {
                    crossings++;
                }
            }
            let frequency = (crossings / 2) / timeScale;
            document.getElementById('frequency').textContent = frequency > 0 ? frequency.toFixed(1) + ' Hz' : '-- Hz';
            
            // Waveform type detection
            let waveformType = detectWaveformType(data, min, max);
            document.getElementById('waveformType').textContent = waveformType;
        }
        
        function detectWaveformType(signal, min, max) {
            if (signal.length < 20) return 'Analyzing...';
            
            let range = max - min;
            if (range < 0.1) return 'DC / Flat';
            
            // Check for PWM (square wave)
            let highCount = 0;
            let lowCount = 0;
            let midpoint = (max + min) / 2;
            
            for (let val of signal) {
                if (val > midpoint + range * 0.3) highCount++;
                else if (val < midpoint - range * 0.3) lowCount++;
            }
            
            let pwmRatio = Math.max(highCount, lowCount) / signal.length;
            if (pwmRatio > 0.6) return 'PWM / Square';
            
            // Check for sine wave
            let sineMatch = 0;
            for (let i = 0; i < Math.min(signal.length, 100); i++) {
                let expected = midpoint + (range / 2) * Math.sin((i / signal.length) * 2 * Math.PI);
                let diff = Math.abs(signal[i] - expected);
                if (diff < range * 0.2) sineMatch++;
            }
            
            if (sineMatch > signal.length * 0.6) return 'Sine Wave';
            
            // Check for triangle wave
            let isIncreasing = false;
            let changeCount = 0;
            for (let i = 1; i < signal.length; i++) {
                let currentIncreasing = signal[i] > signal[i-1];
                if (currentIncreasing !== isIncreasing) {
                    changeCount++;
                    isIncreasing = currentIncreasing;
                }
            }
            
            if (changeCount < 10 && changeCount > 2) return 'Triangle Wave';
            
            return 'Complex / Other';
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
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({sampleRate: rate})
            });
        });
        
        document.getElementById('timeScale').addEventListener('change', function() {
            timeScale = parseFloat(this.value);
            updateDisplay();
        });
        
        document.getElementById('voltageScale').addEventListener('change', function() {
            voltageScale = parseFloat(this.value);
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

void handleControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("sampleRate")) {
      sampleRate = doc["sampleRate"];
      Serial.print("Sample rate changed to: ");
      Serial.println(sampleRate);
    }
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}