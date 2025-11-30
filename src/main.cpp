#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

// Define XSHUT pins for the 3 sensors (GPIO pins for ESP32)
#define XSHUT_PIN_EST 25
#define XSHUT_PIN_OVEST 26
#define XSHUT_PIN_NORD 27

// New I2C addresses for the sensors
#define SENSOR_EST_ADDRESS 0x30
#define SENSOR_OVEST_ADDRESS 0x31
#define SENSOR_NORD_ADDRESS 0x32

// Create 3 VL53L0X objects
VL53L0X sensorEst;
VL53L0X sensorOvest;
VL53L0X sensorNord;

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

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin();
  // Optional: Set I2C clock speed (default is 100kHz, can use up to 400kHz)
  Wire.setClock(400000);
  
  delay(100); // Give time for ESP32 to stabilize
  
  // Configure XSHUT pins as outputs
  pinMode(XSHUT_PIN_EST, OUTPUT);
  pinMode(XSHUT_PIN_OVEST, OUTPUT);
  pinMode(XSHUT_PIN_NORD, OUTPUT);

  // Put all sensors in shutdown mode
  digitalWrite(XSHUT_PIN_EST, LOW);
  digitalWrite(XSHUT_PIN_OVEST, LOW);
  digitalWrite(XSHUT_PIN_NORD, LOW);
  delay(10);

  Serial.println("Starting sensor initialization...");

  // Initialize sensors one by one
  if (!initializeSensor(sensorEst, XSHUT_PIN_EST, SENSOR_EST_ADDRESS, 1)) {
    Serial.println("Failed to initialize sensor EST. System halted.");
    while(1) {
      delay(1000);
    }
  }

  if (!initializeSensor(sensorOvest, XSHUT_PIN_OVEST, SENSOR_OVEST_ADDRESS, 2)) {
    Serial.println("Failed to initialize sensor OVEST. System halted.");
    while(1) {
      delay(1000);
    }
  }

  if (!initializeSensor(sensorNord, XSHUT_PIN_NORD, SENSOR_NORD_ADDRESS, 3)) {
    Serial.println("Failed to initialize sensor NORD. System halted.");
    while(1) {
      delay(1000);
    }
  }

  // Start continuous measurement mode for all sensors
  sensorEst.startContinuous();
  sensorOvest.startContinuous();
  sensorNord.startContinuous();
  
  Serial.println("\nAll sensors ready!");
  Serial.println("Reading distances...\n");
}

void loop() {
  // Read distances from all 3 sensors
  uint16_t distanceEst = sensorEst.readRangeContinuousMillimeters();
  uint16_t distanceOvest = sensorOvest.readRangeContinuousMillimeters();
  uint16_t distanceNord = sensorNord.readRangeContinuousMillimeters();

  // Print values
  Serial.print("Sensore EST: ");
  Serial.print(distanceEst);
  if (sensorEst.timeoutOccurred()) Serial.print(" TIMEOUT");
  Serial.print(" mm | ");

  Serial.print("Sensore OVEST: ");
  Serial.print(distanceOvest);
  if (sensorOvest.timeoutOccurred()) Serial.print(" TIMEOUT");
  Serial.print(" mm | ");

  Serial.print("Sensore NORD: ");
  Serial.print(distanceNord);
  if (sensorNord.timeoutOccurred()) Serial.print(" TIMEOUT");
  Serial.println(" mm");
  
  delay(100);
}