#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "voltage_measurement.h"

VoltageMeasurement voltmeter;   // Your voltage measurement class

// Wi-Fi credentials
const char* ssid = "TCMA";
const char* password = "nikhil90409";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#define SAMPLE_COUNT 200        // Number of samples per frame
#define SAMPLE_INTERVAL 1000    // Microseconds between samples (~1 kHz)

float samples[SAMPLE_COUNT];

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP32 Oscilloscope</title>
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    </head>
    <body>
      <h2>ESP32 Oscilloscope</h2>
      <canvas id="waveform" width="800" height="400"></canvas>
      <script>
        var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        var ctx = document.getElementById('waveform').getContext('2d');
        var chart = new Chart(ctx, {
          type: 'line',
          data: { labels: Array(200).fill(''), datasets: [{ label: 'Voltage (V)', borderColor: 'blue', data: Array(200).fill(0), borderWidth: 1, fill: false, pointRadius: 0 }] },
          options: { animation: false, scales: { y: { min: 0, max: 50 } } }
        });

        ws.onmessage = function(event) {
          let data = JSON.parse(event.data);
          chart.data.datasets[0].data = data;
          chart.update();
        };
      </script>
    </body>
    </html>
  )rawliteral");
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  voltmeter.begin();  // Initialize your voltage measurement class

  server.on("/", handleRoot);
  server.begin();
  webSocket.begin();
}

void loop() {
  webSocket.loop();
  server.handleClient();

  // Sample voltage using your class
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    samples[i] = voltmeter.readVoltage();  // Voltage reading
    delayMicroseconds(SAMPLE_INTERVAL);
  }

  // Send samples as JSON to browser
  String json = "[";
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    json += String(samples[i], 3);
    if (i < SAMPLE_COUNT - 1) json += ",";
  }
  json += "]";
  webSocket.broadcastTXT(json);
}
