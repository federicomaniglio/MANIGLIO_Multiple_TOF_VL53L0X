# Multiple VL53L0X ToF Sensors with ESP32

This project demonstrates how to interface **3 VL53L0X Time-of-Flight (ToF)** distance sensors with an **ESP32** microcontroller using the Pololu VL53L0X library. Since all VL53L0X sensors have the same default I2C address (0x29), this project uses the XSHUT pins to change each sensor's address dynamically during initialization.

## Features

- Simultaneous control of 3 VL53L0X sensors on the same I2C bus
- Dynamic I2C address assignment using XSHUT pins
- Continuous distance measurement mode
- Timeout detection and error handling
- Serial output for real-time monitoring

## Hardware Requirements

- **ESP32 Development Board** (ESP32-DevKitC or similar)
- **3x VL53L0X Time-of-Flight Distance Sensors**
- Jumper wires
- Breadboard (optional)
- USB cable for programming and power

## Wiring Diagram

### I2C Bus (shared by all sensors)
| VL53L0X Pin | ESP32 Pin |
|-------------|-----------|
| VIN         | 3.3V      |
| GND         | GND       |
| SDA         | GPIO 21   |
| SCL         | GPIO 22   |

### XSHUT Pins (individual control)
| Sensor   | XSHUT Pin | ESP32 GPIO |
|----------|-----------|------------|
| Sensor 1 | XSHUT     | GPIO 25    |
| Sensor 2 | XSHUT     | GPIO 26    |
| Sensor 3 | XSHUT     | GPIO 27    |

### Connection Notes
- Connect all VIN pins to ESP32 **3.3V** (not 5V)
- Connect all GND pins to ESP32 **GND**
- Connect all SDA pins together to GPIO 21
- Connect all SCL pins together to GPIO 22
- Connect each XSHUT pin to its respective GPIO pin

## Software Requirements

- **PlatformIO** IDE (or Arduino IDE with ESP32 support)
- **pololu/VL53L0X** library v1.3.1

## Installation

### Using PlatformIO (Recommended)

1. Clone or download this repository
2. Open the project folder in PlatformIO
3. The dependencies will be installed automatically from `platformio.ini`
4. Build and upload the project to your ESP32