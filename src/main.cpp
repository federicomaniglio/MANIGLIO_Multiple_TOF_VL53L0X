#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "credentials.h"

// Define XSHUT pins for the 3 sensors (GPIO pins for ESP32)
#define XSHUT_PIN1 25
#define XSHUT_PIN2 26
#define XSHUT_PIN3 27

// LED pin for WiFi connection status
#define LED_WIFI_PIN 4
#define LED_SENSOR_OK_PIN 2    // Aggiungi LED per sensori OK
#define LED_ERROR_PIN 15       // Aggiungi LED per errori

// New I2C addresses for the sensors
#define SENSOR1_ADDRESS 0x30
#define SENSOR2_ADDRESS 0x31
#define SENSOR3_ADDRESS 0x32



// Create 3 VL53L0X objects
VL53L0X sensor1; // Ovest
VL53L0X sensor2; // Nord
VL53L0X sensor3; // Est

// Web server and WebSocket
WebServer server(80);
WebSocketsServer webSocket(81);

// Current sensor readings
uint16_t distance1 = 0;
uint16_t distance2 = 0;
uint16_t distance3 = 0;

// HTML page with visualization
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sensori ToF - Vista Lidar</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }

        h1 {
            color: white;
            text-align: center;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }

        .container {
            background: white;
            border-radius: 15px;
            padding: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            max-width: 900px;
            width: 100%;
        }

        .lidar-view {
            position: relative;
            width: 100%;
            height: 600px;
            margin: 30px 0;
            background: #1a1a2e;
            border-radius: 10px;
            overflow: hidden;
        }

        #lidar-canvas {
            width: 100%;
            height: 100%;
            display: block;
        }

        .info-panel {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 20px;
            margin-top: 20px;
        }

        .sensor-info {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            box-shadow: 0 4px 10px rgba(0,0,0,0.2);
        }

        .sensor-info h3 {
            margin: 0 0 10px 0;
            font-size: 1.2em;
        }

        .distance-value {
            font-size: 2em;
            font-weight: bold;
            margin: 10px 0;
        }

        .direction {
            font-size: 0.9em;
            opacity: 0.9;
        }

        .status {
            margin-top: 10px;
            padding: 5px;
            border-radius: 5px;
            background: rgba(255,255,255,0.2);
        }

        .connected {
            color: #2ecc71;
        }

        .disconnected {
            color: #e74c3c;
        }

        @media (max-width: 768px) {
            .info-panel {
                grid-template-columns: 1fr;
            }

            .lidar-view {
                height: 400px;
            }
        }
    </style>
</head>
<body>
    <h1>🎯 Visualizzazione Lidar - Sensori ToF</h1>

    <div class="container">
        <div class="status" id="connection-status">
            <span class="disconnected">⚫ Connessione...</span>
        </div>

        <div class="lidar-view">
            <canvas id="lidar-canvas"></canvas>
        </div>

        <div class="info-panel">
            <div class="sensor-info">
                <h3>⬅️ Sensore Ovest</h3>
                <div class="direction">Direzione: 270°</div>
                <div class="distance-value" id="dist-west">--- mm</div>
            </div>

            <div class="sensor-info">
                <h3>⬆️ Sensore Nord</h3>
                <div class="direction">Direzione: 0°</div>
                <div class="distance-value" id="dist-north">--- mm</div>
            </div>

            <div class="sensor-info">
                <h3>➡️ Sensore Est</h3>
                <div class="direction">Direzione: 90°</div>
                <div class="distance-value" id="dist-east">--- mm</div>
            </div>
        </div>
    </div>

    <script>
        const ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        const canvas = document.getElementById('lidar-canvas');
        const ctx = canvas.getContext('2d');

        // Distanza massima in mm per la scala
        const maxDistance = 2000;

        // Dati sensori
        let sensorData = {
            west: 1000,
            north: 1000,
            east: 1000
        };

        // Imposta dimensioni canvas
        function resizeCanvas() {
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            drawLidarView();
        }

        window.addEventListener('resize', resizeCanvas);
        resizeCanvas();

        ws.onopen = function() {
            console.log('WebSocket connesso');
            document.getElementById('connection-status').innerHTML =
                '<span class="connected">🟢 Connesso</span>';
        };

        ws.onclose = function() {
            console.log('WebSocket disconnesso');
            document.getElementById('connection-status').innerHTML =
                '<span class="disconnected">🔴 Disconnesso</span>';
        };

        ws.onerror = function(error) {
            console.log('WebSocket errore: ', error);
        };

        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);

            sensorData.west = data.west;
            sensorData.north = data.north;
            sensorData.east = data.east;

            // Aggiorna valori testuali
            document.getElementById('dist-west').textContent = data.west + ' mm';
            document.getElementById('dist-north').textContent = data.north + ' mm';
            document.getElementById('dist-east').textContent = data.east + ' mm';

            // Ridisegna la vista lidar
            drawLidarView();
        };

        function drawLidarView() {
            const width = canvas.width;
            const height = canvas.height;

            // Pulisci canvas
            ctx.fillStyle = '#1a1a2e';
            ctx.fillRect(0, 0, width, height);

            // Calcola centro (robot position)
            const centerX = width / 2;
            const centerY = height * 0.75; // Robot più in basso per avere più spazio sopra

            // Scala per la visualizzazione (pixel per mm)
            const scale = Math.min(width, height) / (maxDistance * 2.2);

            // Calcola posizioni delle pareti in base alle distanze
            const westWallX = centerX - (sensorData.west * scale);
            const eastWallX = centerX + (sensorData.east * scale);
            const northWallY = centerY - (sensorData.north * scale);
            const southWallY = height; // Base fissa in basso

            // Disegna griglia di riferimento
            ctx.strokeStyle = '#2a2a3e';
            ctx.lineWidth = 1;
            for (let i = 0; i < maxDistance; i += 200) {
                const dist = i * scale;
                // Cerchi concentrici
                ctx.beginPath();
                ctx.arc(centerX, centerY, dist, 0, Math.PI * 2);
                ctx.stroke();
            }

            // Disegna linee di riferimento (raggi)
            ctx.strokeStyle = '#2a2a3e';
            ctx.beginPath();
            ctx.moveTo(centerX, centerY);
            ctx.lineTo(westWallX, centerY);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(centerX, centerY);
            ctx.lineTo(centerX, northWallY);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(centerX, centerY);
            ctx.lineTo(eastWallX, centerY);
            ctx.stroke();

            // Disegna le pareti
            ctx.strokeStyle = '#00f2fe';
            ctx.lineWidth = 4;
            ctx.shadowColor = '#00f2fe';
            ctx.shadowBlur = 10;

            // Parete Ovest (verticale a sinistra)
            ctx.beginPath();
            ctx.moveTo(westWallX, Math.max(northWallY, 20));
            ctx.lineTo(westWallX, southWallY);
            ctx.stroke();

            // Parete Est (verticale a destra)
            ctx.beginPath();
            ctx.moveTo(eastWallX, Math.max(northWallY, 20));
            ctx.lineTo(eastWallX, southWallY);
            ctx.stroke();

            // Parete Nord (orizzontale in alto)
            ctx.beginPath();
            ctx.moveTo(westWallX, northWallY);
            ctx.lineTo(eastWallX, northWallY);
            ctx.stroke();

            // Parete Sud (base fissa)
            ctx.strokeStyle = '#ff4757';
            ctx.beginPath();
            ctx.moveTo(westWallX, southWallY);
            ctx.lineTo(eastWallX, southWallY);
            ctx.stroke();

            ctx.shadowBlur = 0;

            // Disegna il robot al centro
            const robotSize = 20;
            ctx.fillStyle = '#ff4757';
            ctx.shadowColor = '#ff4757';
            ctx.shadowBlur = 15;
            ctx.beginPath();
            ctx.arc(centerX, centerY, robotSize, 0, Math.PI * 2);
            ctx.fill();

            // Disegna direzione del robot (triangolo che punta a nord)
            ctx.fillStyle = '#fff';
            ctx.shadowBlur = 0;
            ctx.beginPath();
            ctx.moveTo(centerX, centerY - robotSize + 5);
            ctx.lineTo(centerX - 6, centerY - 5);
            ctx.lineTo(centerX + 6, centerY - 5);
            ctx.closePath();
            ctx.fill();

            // Aggiungi etichette distanze sulle pareti
            ctx.fillStyle = '#00f2fe';
            ctx.font = 'bold 14px Arial';
            ctx.textAlign = 'center';
            ctx.shadowBlur = 5;
            ctx.shadowColor = '#000';

            // Etichetta Ovest
            ctx.save();
            ctx.translate(westWallX - 15, centerY);
            ctx.rotate(-Math.PI / 2);
            ctx.fillText(sensorData.west + 'mm', 0, 0);
            ctx.restore();

            // Etichetta Nord
            ctx.fillText(sensorData.north + 'mm', centerX, northWallY - 10);

            // Etichetta Est
            ctx.save();
            ctx.translate(eastWallX + 15, centerY);
            ctx.rotate(Math.PI / 2);
            ctx.fillText(sensorData.east + 'mm', 0, 0);
            ctx.restore();
        }

        // Disegna vista iniziale
        drawLidarView();
    </script>
</body>
</html>
)rawliteral";

/**
 * Initialize a single VL53L0X sensor with a new I2C address
 */
bool initializeSensor(VL53L0X &sensor, uint8_t xshutPin, uint8_t newAddress, uint8_t sensorNumber) {
  digitalWrite(xshutPin, HIGH);
  delay(10);

  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.print("Error initializing sensor ");
    Serial.println(sensorNumber);
    return false;
  }

  sensor.setAddress(newAddress);

  Serial.print("Sensor ");
  Serial.print(sensorNumber);
  Serial.println(" initialized successfully");

  return true;
}

/**
 * Handle root page request
 */
void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

/**
 * WebSocket event handler
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
  }
}

/**
 * Send sensor data via WebSocket
 */
void sendSensorData() {
  String json = "{";
  json += "\"west\":" + String(distance1) + ",";
  json += "\"north\":" + String(distance2) + ",";
  json += "\"east\":" + String(distance3);
  json += "}";

  webSocket.broadcastTXT(json);
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_WIFI_PIN, OUTPUT);
  pinMode(LED_SENSOR_OK_PIN, OUTPUT);
  pinMode(LED_ERROR_PIN, OUTPUT);
  
  digitalWrite(LED_WIFI_PIN, LOW);
  digitalWrite(LED_SENSOR_OK_PIN, LOW);
  digitalWrite(LED_ERROR_PIN, LOW);

  // Initialize I2C
  Wire.begin();
  Wire.setClock(400000);

  delay(100);

  // Configure XSHUT pins as outputs
  pinMode(XSHUT_PIN1, OUTPUT);
  pinMode(XSHUT_PIN2, OUTPUT);
  pinMode(XSHUT_PIN3, OUTPUT);

  // Configure LED pin as output
  pinMode(LED_WIFI_PIN, OUTPUT);
  digitalWrite(LED_WIFI_PIN, LOW);

  // Put all sensors in shutdown mode
  digitalWrite(XSHUT_PIN1, LOW);
  digitalWrite(XSHUT_PIN2, LOW);
  digitalWrite(XSHUT_PIN3, LOW);
  delay(10);

  Serial.println("Starting sensor initialization...");

  // Initialize sensors one by one
  if (!initializeSensor(sensor1, XSHUT_PIN1, SENSOR1_ADDRESS, 1)) {
    digitalWrite(LED_ERROR_PIN, HIGH);  // LED errore ON
    Serial.println("Failed to initialize sensor 1 (Ovest). System halted.");
    while(1) delay(1000);
  }

  if (!initializeSensor(sensor2, XSHUT_PIN2, SENSOR2_ADDRESS, 2)) {
    Serial.println("Failed to initialize sensor 2 (Nord). System halted.");
    while(1) delay(1000);
  }

  if (!initializeSensor(sensor3, XSHUT_PIN3, SENSOR3_ADDRESS, 3)) {
    Serial.println("Failed to initialize sensor 3 (Est). System halted.");
    while(1) delay(1000);
  }

  digitalWrite(LED_SENSOR_OK_PIN, HIGH);  // Sensori OK!

  // Start continuous measurement mode for all sensors
  sensor1.startContinuous();
  sensor2.startContinuous();
  sensor3.startContinuous();

  Serial.println("\nAll sensors ready!");

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(LED_ERROR_PIN, !digitalRead(LED_ERROR_PIN));  // Lampeggia durante connessione
    Serial.print(".");
  }

  digitalWrite(LED_ERROR_PIN, LOW);
  digitalWrite(LED_WIFI_PIN, HIGH);  // WiFi connesso!

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Turn on LED to indicate successful WiFi connection and IP acquisition
  digitalWrite(LED_WIFI_PIN, HIGH);

  // Setup web server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  // Setup WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");

  Serial.println("\nSystem ready! Open browser at: http://" + WiFi.localIP().toString());
}

void loop() {
  // Handle web server and WebSocket
  server.handleClient();
  webSocket.loop();

  // Read distances from all 3 sensors
  distance1 = sensor1.readRangeContinuousMillimeters(); // Ovest
  distance2 = sensor2.readRangeContinuousMillimeters(); // Nord
  distance3 = sensor3.readRangeContinuousMillimeters(); // Est

  // Handle timeouts
  if (sensor1.timeoutOccurred()) distance1 = 8190;
  if (sensor2.timeoutOccurred()) distance2 = 8190;
  if (sensor3.timeoutOccurred()) distance3 = 8190;

  // Send data via WebSocket
  sendSensorData();

  // Print to serial for debugging
  Serial.print("Ovest: ");
  Serial.print(distance1);
  Serial.print(" mm | Nord: ");
  Serial.print(distance2);
  Serial.print(" mm | Est: ");
  Serial.print(distance3);
  Serial.println(" mm");
  
  delay(100);
}