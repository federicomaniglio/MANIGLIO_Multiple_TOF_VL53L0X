#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

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
  
  Serial.println("\nAll sensors ready!");
  Serial.println("Reading distances...\n");
}

void loop() {
  // Read distances from all 3 sensors
  uint16_t distance1 = sensor1.readRangeContinuousMillimeters();
  uint16_t distance2 = sensor2.readRangeContinuousMillimeters();
  uint16_t distance3 = sensor3.readRangeContinuousMillimeters();
  
  // Print values
  Serial.print("Sensor 1: ");
  Serial.print(distance1);
  if (sensor1.timeoutOccurred()) Serial.print(" TIMEOUT");
  Serial.print(" mm | ");
  
  Serial.print("Sensor 2: ");
  Serial.print(distance2);
  if (sensor2.timeoutOccurred()) Serial.print(" TIMEOUT");
  Serial.print(" mm | ");
  
  Serial.print("Sensor 3: ");
  Serial.print(distance3);
  if (sensor3.timeoutOccurred()) Serial.print(" TIMEOUT");
  Serial.println(" mm");
  
  delay(100);
}