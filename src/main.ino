/*
 * ESP32 Vehicle Data Relay - TCP Version
 * 
 * This code implements a vehicle data relay system that:
 * 1. Receives sensor data from Arduinos via serial communication
 * 2. Parses serial messages in KEY:VALUE format
 * 3. Acts as TCP server to send data to clients (ONE-WAY ONLY)
 * 4. Uses persistent TCP connections for efficient data streaming
 * 5. No sensor reading - pure data relay
 * 
 * Serial Format: KEY:VALUE (e.g., SPEED:120, COOLANT:90, FUEL:50)
 */

// Core ESP32 libraries

// Our modular components
#include "vehicle_data.h"
#include "tcp_server.h"

// Configuration
#define CONNECTION_LED_PIN 2  // Built-in LED pin for most ESP32 boards
#define DATA_SEND_INTERVAL 250  // Send data every 250ms (4Hz)
#define MESSAGE_LED_FLASH_DURATION 100  // LED flash duration in milliseconds

// LED flash variables
unsigned long ledFlashStartTime = 0;
bool ledFlashActive = false;


/**
 * Flash the built-in LED for a brief moment to indicate message received
 */
void flashMessageLED() {
    ledFlashStartTime = millis();
    ledFlashActive = true;
    digitalWrite(CONNECTION_LED_PIN, HIGH);
}

/**
 * Update LED flash state - call this in the main loop
 */
void updateMessageLED() {
    if (ledFlashActive) {
        unsigned long currentTime = millis();
        if (currentTime - ledFlashStartTime >= MESSAGE_LED_FLASH_DURATION) {
            ledFlashActive = false;
            // Only turn off LED if no TCP clients are connected
            if (!tcpServer.isConnected()) {
                digitalWrite(CONNECTION_LED_PIN, LOW);
            }
        }
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Initialize Serial2 with custom pins: RX=16, TX=17

    initSensors();
    
    // Initialize TCP server
    tcpServer.init(CONNECTION_LED_PIN, DATA_SEND_INTERVAL);
}

void loop() {
    // Handle serial communication from Arduino 1 (Serial)
    if (Serial.available() > 0) {
        String message = Serial.readStringUntil('\n');
        message.trim();
        handleSerialMessage(message, "Arduino1");
    }
    
    // Handle serial communication from Arduino 2 (Serial2)
    if (Serial2.available() > 0) {
        String message = Serial2.readStringUntil('\n');
        message.trim();
        handleSerialMessage(message, "Arduino2");
    }
    
    // Handle TCP communication (send data to clients)
    tcpServer.update();
    
    // Update message LED flash state
    updateMessageLED();
    
    // Small delay to prevent watchdog issues
    delayMicroseconds(100);
}

/**
 * Handle serial messages from Arduinos in KEY:VALUE format
 * Supported keys: SPEED, LOCATION, COOLANT, FUEL, OIL_WARN, GLOW, BATTERY, DRL, LOWBEAM, HIGHBEAM, LEFTURN, RIGHTURN, HAZARD, REVERSE
 * @param message The serial message in KEY:VALUE format
 * @param arduinoId Identifier for which Arduino sent the message ("Arduino1" or "Arduino2")
 */
void handleSerialMessage(const String& message, const String& arduinoId) {
    int colonIndex = message.indexOf(':');
    if (colonIndex == -1) {
        // Debug output (comment out for production to prevent feedback loop)
        return;
    }
    
    // Flash LED to indicate message received
    flashMessageLED();
    
    String key = message.substring(0, colonIndex);
    String valueStr = message.substring(colonIndex + 1);
    
    // Parse the value (handle both integer and float)
    float value = valueStr.toFloat();
    
    if (key == "SPEED") {
        currentVehicleData.speed = value;
    }
    else if (key == "LOCATION") {
        currentVehicleData.location = valueStr;
    }
    else if (key == "COOLANT") {
        currentVehicleData.coolantTemp = value;
    }
    else if (key == "FUEL") {
        currentVehicleData.fuelLevel = constrain(value, 0, 100);
    }
    else if (key == "OIL_WARN") {
        currentVehicleData.oilWarning = (value == 1);  // 1 = low pressure (warning), 0 = normal pressure
    }
    else if (key == "BATTERY") {
        currentVehicleData.batteryVoltage = value;
    }
    else if (key == "DRL") {
        currentVehicleData.drlOn = (value != 0);
    }
    else if (key == "LOWBEAM") {
        currentVehicleData.lowBeamOn = (value != 0);
    }
    else if (key == "HIGHBEAM") {
        currentVehicleData.highBeamOn = (value != 0);
    }
    else if (key == "LEFTURN") {
        currentVehicleData.leftTurnSignal = (value != 0);
    }
    else if (key == "RIGHTURN") {
        currentVehicleData.rightTurnSignal = (value != 0);
    }
    else if (key == "HAZARD") {
        currentVehicleData.hazardLights = (value != 0);
    }
    else if (key == "REVERSE") {
        currentVehicleData.reverseGear = (value != 0);
    }
}
