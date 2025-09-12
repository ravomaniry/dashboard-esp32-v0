# ESP32 Vehicle Data Connector

A vehicle data relay system that receives sensor data from Arduinos via serial communication and transmits it to an Android app via Bluetooth. The ESP32 acts as a bridge between vehicle sensors and mobile applications.

## Features

- **Dual Arduino Support**: Connects to two Arduinos simultaneously via separate serial ports
- **Speed and Location Tracking**: Receives GPS-based speed and location data
- **Vehicle Sensors**: Coolant temperature, fuel level, oil status, battery voltage
- **Light Status**: DRL, headlights, turn signals, hazard lights
- **Bluetooth Communication**: Secure data transmission to Android apps
- **Serial Data Relay**: Receives data from multiple Arduinos via serial communication
- **Arduino Identification**: Debug messages identify which Arduino sent each data packet

## Hardware

- ESP32 Dev Module

## Software

- Arduino Framework
- PlatformIO
- [ESP32CompositeColorVideo](https://github.com/marciot/ESP32CompositeColorVideo) library

## Setup

1. Clone the repository.
2. Install [PlatformIO](https://platformio.org/).
3. Connect the ESP32 to your computer.
4. Build and upload the project using the `/home/ravo/.platformio/penv/bin/pio run --target upload` command.

## Demo Mode

Set the `IS_DEMO` variable in `src/main.ino` to `true` to enable a demo mode that cycles through various values. This is useful for testing the Bluetooth transmission without real sensor data. Demo mode includes simulated speed (0-120 km/h) and location data (circular movement around a test area).

## Serial Communication

The ESP32 receives data from Arduinos via serial communication.

The message format is `KEY:VALUE`, where `KEY` can be `SPEED`, `LOCATION`, `COOLANT`, `FUEL`, `OIL`, `BATTERY`, `DRL`, `LOWBEAM`, `HIGHBEAM`, `LEFTURN`, `RIGHTURN`, `HAZARD`, or `GLOW`.

**Example:**

```
SPEED:65
LOCATION:40.7128,-74.0060
COOLANT:90
FUEL:50
OIL:0
BATTERY:12.6
DRL:1
LOWBEAM:0
HIGHBEAM:0
LEFTURN:0
RIGHTURN:0
HAZARD:0
GLOW:1
```

## Bluetooth Data Format

The ESP32 sends JSON data over Bluetooth to connected Android apps:

```json
{
  "coolantTemp": 85.0,
  "fuelLevel": 50.0,
  "oilWarning": false,
  "batteryVoltage": 12.6,
  "drlOn": false,
  "lowBeamOn": false,
  "highBeamOn": false,
  "leftTurnSignal": false,
  "rightTurnSignal": false,
  "hazardLights": false,
  "speed": 65.0,
  "location": "40.7128,-74.0060"
}
```

## Dual Arduino Wiring

### Arduino 1 (Primary - Serial)

```
Arduino TX (5V) ──470Ω──┐── ESP32 RX0 (GPIO3)
                          │
                          1kΩ
                          │
                         GND

ESP32 TX0 (GPIO1) ────────> Arduino RX  (direct connection)
GND ───────────────────── GND
```

### Arduino 2 (Secondary - Serial2)

```
Arduino TX (5V) ──470Ω──┐── ESP32 RX2 (GPIO16)
                          │
                          1kΩ
                          │
                         GND


ESP32 TX2 (GPIO17) ───────> Arduino RX  (direct connection)
GND ───────────────────── GND
```

### Pin Assignments

- **Arduino 1**: Uses ESP32 Serial (TX0/RX0) - typically USB connection
- **Arduino 2**: Uses ESP32 Serial2 (TX2/RX2) - hardware serial pins

**Note:** The ESP32 operates at a 3.3V logic level. The provided wiring diagram includes a voltage divider to protect the ESP32's 3.3V RX pin from the Arduino's 5V TX signal. While the direct connection from the ESP32's 3.3V TX to the Arduino's 5V RX might work, a logic level shifter is recommended for reliable communication.
