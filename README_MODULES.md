# ESP32 Vehicle Dashboard - Modular Structure

This project has been refactored into a modular architecture for better organization, maintainability, and reusability. The ESP32 acts as a **pure sensor data transmitter** - it receives vehicle sensor data from multiple Arduinos via dual serial communication and sends consolidated data to the Android app via Bluetooth, with no display output. This creates a lightweight, power-efficient system focused solely on data collection and transmission.

## Project Structure

```
src/
├── main.ino                 # Main application entry point
├── vehicle_data.h/.cpp      # Sensor data management and reading
└── bluetooth_server.h/.cpp  # Bluetooth server with authentication
```

## Module Responsibilities

### 1. `main.ino`

- **Purpose**: Application entry point and main loop coordination
- **Features**:
  - ESP32 initialization and configuration
  - Dual serial port initialization (Serial and Serial1)
  - Module initialization and coordination
  - Main application loop with sensor timing
  - Dual Arduino communication handling
  - Demo mode for testing sensor values
  - Pure data transmission focus

### 2. `vehicle_data` Module

- **Purpose**: Vehicle sensor data management
- **Features**:
  - Data structure for all vehicle sensors and GPS data
  - Serial message parsing for Arduino communication
  - Data validation and storage
  - Default value initialization
  - Critical threshold definitions

#### Supported Data:

- **Speed**: GPS-based speed data from Arduinos (km/h)
- **Location**: GPS coordinates in "lat,lon" format from Arduinos
- **Coolant Temperature**: Analog sensor (LM35 or automotive sensor)
- **Fuel Level**: Analog fuel sender (0-100%)
- **Oil Pressure**: Digital pressure switch
- **Battery Voltage**: Analog voltage divider circuit
- **Light Status**: Digital inputs for DRL, low/high beam, turn signals, hazard

#### Dual Serial Communication:

The ESP32 supports two Arduinos simultaneously:

- **Arduino 1**: Connected via Serial (TX0/RX0) - typically USB connection
- **Arduino 2**: Connected via Serial2 (TX2/RX2) - hardware serial pins

#### Serial Communication Format:

Both Arduinos send data using the `KEY:VALUE` format:

- `SPEED:65` - Vehicle speed in km/h
- `LOCATION:40.7128,-74.0060` - GPS coordinates (latitude,longitude)
- `COOLANT:90` - Coolant temperature in Celsius
- `FUEL:50` - Fuel level percentage (0-100)
- `OIL:0` - Oil pressure status (0=OK, 1=Low)
- `BATTERY:12.6` - Battery voltage
- `DRL:1` - Daytime running lights (0=OFF, 1=ON)
- `LOWBEAM:0` - Low beam headlights (0=OFF, 1=ON)
- `HIGHBEAM:0` - High beam headlights (0=OFF, 1=ON)
- `LEFTURN:0` - Left turn signal (0=OFF, 1=ON)
- `RIGHTURN:0` - Right turn signal (0=OFF, 1=ON)
- `HAZARD:0` - Hazard lights (0=OFF, 1=ON)
- `GLOW:1` - Glow plug status (0=OFF, 1=ON)

#### Arduino Identification:

Debug messages include Arduino identification:

- `[Arduino1] Received speed: 65.0 km/h`
- `[Arduino2] Received location: 40.7128,-74.0060`

### 3. `bluetooth_server` Module

- **Purpose**: Bluetooth communication with Android app
- **Features**:
  - Bluetooth Classic (SPP) server implementation
  - SHA256-based authentication protocol
  - JSON data transmission
  - Connection status management
  - Configurable data transmission intervals
  - LED status indication

#### Authentication Protocol:

1. ESP32 sends challenge: `CHALLENGE:[random_string]`
2. Android responds with SHA256(challenge + "super_secret_salt")
3. ESP32 verifies and responds with "AUTH OK" or "AUTH FAIL"
4. Data transmission only after successful authentication

#### Data Format:

```json
{
  "coolantTemp": 82.0,
  "fuelLevel": 65.0,
  "oilWarning": false,
  "batteryVoltage": 12.6,
  "drlOn": true,
  "lowBeamOn": false,
  "highBeamOn": false,
  "leftTurnSignal": false,
  "rightTurnSignal": false,
  "hazardLights": false,
  "speed": 65.0,
  "location": "40.7128,-74.0060"
}
```

### 3. `bluetooth_server` Module

- **Purpose**: Bluetooth communication with Android app
- **Features**:
  - Bluetooth Classic (SPP) server implementation
  - SHA256-based authentication protocol
  - JSON data transmission
  - Connection status management
  - Configurable data transmission intervals
  - LED status indication

## Hardware Configuration

### Dual Arduino Serial Connections:

#### Arduino 1 (Primary - Serial)

- **ESP32 RX0**: GPIO3 (connected to Arduino TX via voltage divider)
- **ESP32 TX0**: GPIO1 (connected to Arduino RX directly)
- **Baud Rate**: 115200

#### Arduino 2 (Secondary - Serial2)

- **ESP32 RX2**: GPIO16 (connected to Arduino TX via voltage divider)
- **ESP32 TX2**: GPIO17 (connected to Arduino RX directly)
- **Baud Rate**: 115200

### Pin Assignments (Configurable in `vehicle_data.cpp`):

```cpp
SensorPins sensorPins = {
    .coolantTempPin = A0,      // Coolant temperature sensor
    .fuelLevelPin = A1,        // Fuel level sensor
    .oilPressurePin = 2,       // Oil pressure switch
    .batteryVoltagePin = A2,   // Battery voltage divider
    .drlPin = 3,              // DRL status
    .lowBeamPin = 4,          // Low beam status
    .highBeamPin = 5,         // High beam status
    .leftTurnPin = 6,         // Left turn signal
    .rightTurnPin = 7,        // Right turn signal
    .hazardPin = 8            // Hazard lights
};
```

### Required Circuits:

1. **Battery Voltage**: Voltage divider (10kΩ + 10kΩ) for 12V → 6V max
2. **Temperature Sensor**: LM35 or automotive coolant sensor
3. **Fuel Level**: Standard automotive fuel sender
4. **Oil Pressure**: Automotive oil pressure switch
5. **Light Status**: Optocouplers or voltage dividers for vehicle electrical system

### Power Management:

- **No Display**: Significantly reduced power consumption
- **Sensor Reading**: Only active during data transmission intervals
- **Bluetooth**: Low-power transmission mode
- **Sleep Mode**: Can implement deep sleep between readings for maximum efficiency

## Usage

### Compilation

```bash
# Using PlatformIO
pio run

# Using Arduino IDE
# Open main.ino and compile normally
```

### Demo Mode

- Set `IS_DEMO = true` in the code to enable demo mode
- Demo mode cycles through test values for all sensors and GPS data
- Speed sweeps from 0-120 km/h
- Location simulates circular movement around a test area
- Useful for testing Bluetooth transmission without real sensors
- All sensor values cycle through realistic ranges for testing
- Simulates data from both Arduino1 and Arduino2 serial ports

### Bluetooth Connection

1. Device appears as "RAVO_CAR_DASH"
2. Android app connects and receives authentication challenge
3. App must respond with correct SHA256 hash
4. After authentication, ESP32 sends sensor data every 250ms (configurable)

## Benefits of Modular Design

1. **Maintainability**: Each module has a single responsibility
2. **Testability**: Individual modules can be tested independently
3. **Reusability**: Modules can be used in other projects
4. **Scalability**: Easy to add new sensors or features
5. **Debugging**: Issues can be isolated to specific modules
6. **Code Organization**: Clear separation of concerns

## Future Enhancements

- CAN bus integration for OBD-II data
- WiFi-based communication option
- Data logging to SD card
- Over-the-air firmware updates
- Additional sensor support (tire pressure, etc.)
- Custom dashboard themes/layouts

## Dependencies

- ESP32 Arduino Core
- ArduinoJson library (for JSON serialization)
- Built-in ESP32 Bluetooth and crypto libraries
