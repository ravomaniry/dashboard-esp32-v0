/*
 * ESP32 Vehicle Data Relay
 * 
 * This code implements a vehicle data relay system that:
 * 1. Receives sensor data from Arduinos via serial communication
 * 2. Parses serial messages in KEY:VALUE format
 * 3. Acts as WiFi server to send data to web clients (ONE-WAY ONLY)
 * 4. Includes web interface for data access
 * 5. No sensor reading - pure data relay
 * 
 * Serial Format: KEY:VALUE (e.g., SPEED:120, COOLANT:90, FUEL:50)
 */

// Core ESP32 libraries
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Our modular components
#include "vehicle_data.h"
#include "wifi_server.h"

// Configuration
#define CONNECTION_LED_PIN 2  // Built-in LED pin for most ESP32 boards
#define DATA_SEND_INTERVAL 250  // Send data every 250ms (4Hz)
#define MESSAGE_LED_FLASH_DURATION 100  // LED flash duration in milliseconds

// Demo mode - set to true to cycle through test values
bool IS_DEMO = false;

// LED flash variables
unsigned long ledFlashStartTime = 0;
bool ledFlashActive = false;

void randomizeValues() {
    static unsigned long lastUpdateTime = 0;
    const unsigned long UPDATE_INTERVAL_MS = 1000;
    
    if (millis() > lastUpdateTime + UPDATE_INTERVAL_MS) {
        lastUpdateTime = millis();
        
        // Sweep through ranges for easier testing
        static int sweepCounter = 0;
        sweepCounter++;
        
        // Coolant: sweep 60-120
        currentVehicleData.coolantTemp = 60 + ((sweepCounter * 10) % 61);
        
        // Fuel: sweep 0-100
        currentVehicleData.fuelLevel = (sweepCounter * 8) % 101;
        
        // Oil: alternate between LOW and HIGH
        currentVehicleData.oilWarning = (sweepCounter / 10) % 2;
        
        // Battery: sweep 11.5-14.5V
        currentVehicleData.batteryVoltage = 11.5 + ((sweepCounter * 3) % 31) / 10.0;
        
        // Lights: cycle through different combinations
        currentVehicleData.drlOn = (sweepCounter / 5) % 2;
        currentVehicleData.lowBeamOn = (sweepCounter / 7) % 2;
        currentVehicleData.highBeamOn = (sweepCounter / 11) % 2;
        currentVehicleData.leftTurnSignal = (sweepCounter / 13) % 2;
        currentVehicleData.rightTurnSignal = (sweepCounter / 17) % 2;
        currentVehicleData.hazardLights = (sweepCounter / 19) % 2;
        
        // Reverse gear: cycle every 23 iterations
        currentVehicleData.reverseGear = (sweepCounter / 23) % 2;
        
        // Speed: sweep 0-120 km/h
        currentVehicleData.speed = (sweepCounter * 2) % 121;
        
        // Location: simulate movement around a small area (demo coordinates)
        float lat = 40.7128 + (sin(sweepCounter * 0.1) * 0.01);
        float lon = -74.0060 + (cos(sweepCounter * 0.1) * 0.01);
        currentVehicleData.location = String(lat, 4) + "," + String(lon, 4);
    }
}

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
            // Only turn off LED if WiFi is not connected
            if (!wifiServer.isConnected()) {
                digitalWrite(CONNECTION_LED_PIN, LOW);
            }
        }
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Initialize Serial2 with custom pins: RX=16, TX=17

    // Initialize random number generator
    srand(time(NULL)); 
    initSensors();
    
    // Initialize WiFi server
    Serial.println("Initializing WiFi server...");
    wifiServer.init(CONNECTION_LED_PIN, DATA_SEND_INTERVAL);
    
    Serial.println("Setup complete!");
}

void loop() {
    static unsigned long lastSensorUpdate = 0;
    
    unsigned long currentTime = millis();
    
    // Update sensors every 100ms
    if (currentTime - lastSensorUpdate >= 100) {
        if (IS_DEMO) {
            randomizeValues();  // Use demo values
        } else {
            // In non-demo mode, data comes from serial communication
            // No need to call updateAllSensors() as data is received via serial
        }
        lastSensorUpdate = currentTime;
    }
    
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
    
    // Handle WiFi communication (send data to web clients)
    wifiServer.update();
    
    // Update message LED flash state
    updateMessageLED();
    
    // Small delay to prevent watchdog issues
    delayMicroseconds(100);
}

/**
 * Handle serial messages from Arduinos in KEY:VALUE format
 * Supported keys: SPEED, LOCATION, COOLANT, FUEL, OIL, GLOW, BATTERY, DRL, LOWBEAM, HIGHBEAM, LEFTURN, RIGHTURN, HAZARD, REVERSE
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
    else if (key == "OIL") {
        currentVehicleData.oilWarning = (value != 0);
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
