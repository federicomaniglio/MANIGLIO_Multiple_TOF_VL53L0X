#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h"

// Intervallo di aggiornamento in millisecondi (es. 2000 = 2 secondi)
const unsigned long UPDATE_INTERVAL = 2000;

// Define XSHUT pins for the 3 sensors (GPIO pins for ESP32)
#define XSHUT_PIN1 25
#define XSHUT_PIN2 26
#define XSHUT_PIN3 27

// New I2C addresses for the sensors
#define SENSOR1_ADDRESS 0x30
#define SENSOR2_ADDRESS 0x31
#define SENSOR3_ADDRESS 0x32

// Create 3 VL53L0X objects
VL53L0X sensor1;
VL53L0X sensor2;
VL53L0X sensor3;

// Webserver sulla porta 80
WebServer server(80);

// Variabili per memorizzare le ultime letture
struct SensorData {
  uint16_t distance1 = 0;
  uint16_t distance2 = 0;
  uint16_t distance3 = 0;
  bool timeout1 = false;
  bool timeout2 = false;
  bool timeout3 = false;
  unsigned long lastUpdate = 0;
} sensorData;

/**
 * Initialize a single VL53L0X sensor with a new I2C address
 * @param sensor Reference to the VL53L0X object
 * @param xshutPin Pin number for the XSHUT control
 * @param newAddress New I2C address to assign to the sensor
 * @param sensorNumber Sensor identifier for debug messages
 * @return true if initialization successful, false otherwise
 */
bool initializeSensor(VL53L0X &sensor, uint8_t xshutPin, uint8_t newAddress, uint8_t sensorNumber) {
  // Enable the sensor by setting XSHUT HIGH
  digitalWrite(xshutPin, HIGH);
  delay(10);

  // Set timeout and initialize
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.print("Error initializing sensor ");
    Serial.println(sensorNumber);
    return false;
  }

  // Set new I2C address
  sensor.setAddress(newAddress);

  Serial.print("Sensor ");
  Serial.print(sensorNumber);
  Serial.println(" initialized successfully");

  return true;
}

/**
 * Handler per la pagina principale - HTML con auto-refresh
 */
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>VL53L0X Sensors Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".sensor { margin: 15px 0; padding: 15px; background: #e8f4f8; border-radius: 5px; }";
  html += ".sensor-name { font-weight: bold; color: #0066cc; }";
  html += ".distance { font-size: 24px; color: #009900; margin: 5px 0; }";
  html += ".timeout { color: #cc0000; font-weight: bold; }";
  html += ".info { text-align: center; color: #666; font-size: 12px; margin-top: 20px; }";
  html += "</style>";
  html += "<meta http-equiv='refresh' content='" + String(UPDATE_INTERVAL / 1000) + "'>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>📡 VL53L0X Sensors Monitor</h1>";

  // Sensor 1
  html += "<div class='sensor'>";
  html += "<div class='sensor-name'>Sensor 1</div>";
  html += "<div class='distance'>" + String(sensorData.distance1) + " mm</div>";
  if (sensorData.timeout1) html += "<div class='timeout'>⚠ TIMEOUT</div>";
  html += "</div>";

  // Sensor 2
  html += "<div class='sensor'>";
  html += "<div class='sensor-name'>Sensor 2</div>";
  html += "<div class='distance'>" + String(sensorData.distance2) + " mm</div>";
  if (sensorData.timeout2) html += "<div class='timeout'>⚠ TIMEOUT</div>";
  html += "</div>";

  // Sensor 3
  html += "<div class='sensor'>";
  html += "<div class='sensor-name'>Sensor 3</div>";
  html += "<div class='distance'>" + String(sensorData.distance3) + " mm</div>";
  if (sensorData.timeout3) html += "<div class='timeout'>⚠ TIMEOUT</div>";
  html += "</div>";

  html += "<div class='info'>Ultimo aggiornamento: " + String(millis() / 1000) + " secondi</div>";
  html += "<div class='info'>Aggiornamento automatico ogni " + String(UPDATE_INTERVAL / 1000) + " secondi</div>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

/**
 * Handler per API JSON - restituisce dati in formato JSON
 */
void handleJSON() {
  String json = "{";
  json += "\"sensor1\":{\"distance\":" + String(sensorData.distance1) + ",\"timeout\":" + String(sensorData.timeout1 ? "true" : "false") + "},";
  json += "\"sensor2\":{\"distance\":" + String(sensorData.distance2) + ",\"timeout\":" + String(sensorData.timeout2 ? "true" : "false") + "},";
  json += "\"sensor3\":{\"distance\":" + String(sensorData.distance3) + ",\"timeout\":" + String(sensorData.timeout3 ? "true" : "false") + "},";
  json += "\"timestamp\":" + String(millis());
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Connessione WiFi
  Serial.println("\nConnessione al WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connesso!");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

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
    Serial.println("Failed to initialize sensor 1. System halted.");
    while(1) {
      delay(1000);
    }
  }

  if (!initializeSensor(sensor2, XSHUT_PIN2, SENSOR2_ADDRESS, 2)) {
    Serial.println("Failed to initialize sensor 2. System halted.");
    while(1) {
      delay(1000);
    }
  }

  if (!initializeSensor(sensor3, XSHUT_PIN3, SENSOR3_ADDRESS, 3)) {
    Serial.println("Failed to initialize sensor 3. System halted.");
    while(1) {
      delay(1000);
    }
  }

  // Start continuous measurement mode for all sensors
  sensor1.startContinuous();
  sensor2.startContinuous();
  sensor3.startContinuous();

  // Configura i route del webserver
  server.on("/", handleRoot);
  server.on("/api", handleJSON);

  // Avvia il webserver
  server.begin();
  Serial.println("Webserver avviato!");
  Serial.println("\nAll sensors ready!");
  Serial.println("Reading distances...\n");
  Serial.println("Visita http://" + WiFi.localIP().toString() + " per vedere i dati");
  Serial.println("Oppure http://" + WiFi.localIP().toString() + "/api per i dati in JSON\n");
}

void loop() {
  // Gestisci le richieste web
  server.handleClient();

  // Aggiorna i dati dei sensori ogni UPDATE_INTERVAL millisecondi
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentMillis;

    // Read distances from all 3 sensors
    sensorData.distance1 = sensor1.readRangeContinuousMillimeters();
    sensorData.timeout1 = sensor1.timeoutOccurred();

    sensorData.distance2 = sensor2.readRangeContinuousMillimeters();
    sensorData.timeout2 = sensor2.timeoutOccurred();

    sensorData.distance3 = sensor3.readRangeContinuousMillimeters();
    sensorData.timeout3 = sensor3.timeoutOccurred();

    sensorData.lastUpdate = currentMillis;

    // Print values to Serial
    Serial.print("Sensor 1: ");
    Serial.print(sensorData.distance1);
    if (sensorData.timeout1) Serial.print(" TIMEOUT");
    Serial.print(" mm | ");

    Serial.print("Sensor 2: ");
    Serial.print(sensorData.distance2);
    if (sensorData.timeout2) Serial.print(" TIMEOUT");
    Serial.print(" mm | ");

    Serial.print("Sensor 3: ");
    Serial.print(sensorData.distance3);
    if (sensorData.timeout3) Serial.print(" TIMEOUT");
    Serial.println(" mm");
  }
}