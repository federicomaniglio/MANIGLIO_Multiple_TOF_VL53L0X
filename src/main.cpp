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
    <title>Sensori ToF - Real-Time</title>
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

        .compass {
            position: relative;
            width: 100%;
            height: 500px;
            margin: 30px 0;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .center-point {
            position: absolute;
            width: 30px;
            height: 30px;
            background: #ff4757;
            border-radius: 50%;
            z-index: 10;
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }

        .sensor-bar {
            position: absolute;
            background: linear-gradient(to right, #4facfe 0%, #00f2fe 100%);
            transition: all 0.3s ease;
            border-radius: 5px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.2);
        }

        .sensor-west {
            left: 0;
            top: 50%;
            transform: translateY(-50%);
            height: 60px;
            transform-origin: right center;
        }

        .sensor-north {
            top: 0;
            left: 50%;
            transform: translateX(-50%);
            width: 60px;
            transform-origin: center bottom;
        }

        .sensor-east {
            right: 0;
            top: 50%;
            transform: translateY(-50%);
            height: 60px;
            transform-origin: left center;
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

            .compass {
                height: 400px;
            }
        }
    </style>
</head>
<body>
    <h1>🎯 Monitoraggio Sensori ToF Real-Time</h1>

    <div class="container">
        <div class="status" id="connection-status">
            <span class="disconnected">⚫ Connessione...</span>
        </div>

        <div class="compass">
            <div class="sensor-bar sensor-west" id="bar-west"></div>
            <div class="sensor-bar sensor-north" id="bar-north"></div>
            <div class="sensor-bar sensor-east" id="bar-east"></div>
            <div class="center-point"></div>
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
        const maxDistance = 2000; // Maximum distance in mm for scaling

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

            // Update distance values
            document.getElementById('dist-west').textContent = data.west + ' mm';
            document.getElementById('dist-north').textContent = data.north + ' mm';
            document.getElementById('dist-east').textContent = data.east + ' mm';

            // Calculate bar widths/heights (inverse: closer = longer bar)
            const westWidth = calculateBarSize(data.west);
            const northHeight = calculateBarSize(data.north);
            const eastWidth = calculateBarSize(data.east);

            // Update visual bars
            document.getElementById('bar-west').style.width = westWidth + 'px';
            document.getElementById('bar-north').style.height = northHeight + 'px';
            document.getElementById('bar-east').style.width = eastWidth + 'px';
        };

        function calculateBarSize(distance) {
            // Inverse relationship: closer objects = longer bars
            // Clamp distance between 30mm and maxDistance
            const clampedDist = Math.max(30, Math.min(distance, maxDistance));
            // Scale from 30-2000mm to 200-30px (inverse)
            const size = 200 - ((clampedDist - 30) / (maxDistance - 30) * 170);
            return Math.max(30, size);
        }
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

  // Initialize I2C
  Wire.begin();
  Wire.setClock(400000);

  delay(100);

  // Configure XSHUT pins as outputs
  pinMode(XSHUT_PIN1, OUTPUT);
  pinMode(XSHUT_PIN2, OUTPUT);
  pinMode(XSHUT_PIN3, OUTPUT);

  // Put all sensors in shutdown mode
  digitalWrite(XSHUT_PIN1, LOW);
  digitalWrite(XSHUT_PIN2, LOW);
  digitalWrite(XSHUT_PIN3, LOW);
  delay(10);

  Serial.println("Starting sensor initialization...");

  // Initialize sensors one by one
  if (!initializeSensor(sensor1, XSHUT_PIN1, SENSOR1_ADDRESS, 1)) {
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
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

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