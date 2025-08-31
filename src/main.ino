/*
 * ESP32 Vehicle Data Relay
 * 
 * This code implements a vehicle data relay system that:
 * 1. Receives sensor data from Arduinos via serial communication
 * 2. Parses serial messages in KEY:VALUE format
 * 3. Acts as Bluetooth server to send data to Android app (ONE-WAY ONLY)
 * 4. Includes authentication protocol for secure communication
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
#include "bluetooth_server.h"

// Configuration
#define CONNECTION_LED_PIN 2  // Built-in LED pin for most ESP32 boards
#define DATA_SEND_INTERVAL 250  // Send data every 250ms (4Hz)

// Demo mode - set to true to cycle through test values
bool IS_DEMO = true;

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
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println("ESP32 Vehicle Data Relay starting...");
    
    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize vehicle data structure with default values
    Serial.println("Initializing vehicle data structure...");
    initSensors();
    
    // Initialize Bluetooth server
    Serial.println("Initializing Bluetooth server...");
    btServer.init(CONNECTION_LED_PIN, DATA_SEND_INTERVAL);
    
    Serial.println("Setup complete!");
    Serial.printf("Device name: %s\n", "RAVO_CAR_DASH");
    Serial.println("Ready for Bluetooth connections.");
    Serial.println("Waiting for serial data from Arduinos...");
    Serial.println("Message format: KEY:VALUE (e.g., COOLANT:90, FUEL:50)");
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
    
    // Handle serial communication from Arduinos
    if (Serial.available() > 0) {
        String message = Serial.readStringUntil('\n');
        message.trim();
        handleSerialMessage(message);
    }
    
    // Handle Bluetooth communication (send data to Android app)
    btServer.update();
    
    // Small delay to prevent watchdog issues
    delayMicroseconds(100);
}

/**
 * Handle serial messages from Arduinos in KEY:VALUE format
 * Supported keys: SPEED, COOLANT, FUEL, OIL, GLOW, BATTERY, DRL, LOWBEAM, HIGHBEAM, LEFTURN, RIGHTURN, HAZARD
 */
void handleSerialMessage(const String& message) {
    int colonIndex = message.indexOf(':');
    if (colonIndex == -1) {
        Serial.println("Invalid message format. Use KEY:VALUE");
        return;
    }
    
    String key = message.substring(0, colonIndex);
    String valueStr = message.substring(colonIndex + 1);
    
    // Parse the value (handle both integer and float)
    float value = valueStr.toFloat();
    
    if (key == "SPEED") {
        // Speed is GPS-based, not sent to Android app
        Serial.printf("Received speed: %s km/h\n", valueStr.c_str());
    }
    else if (key == "COOLANT") {
        currentVehicleData.coolantTemp = value;
        Serial.printf("Received coolant temp: %.1f°C\n", value);
    }
    else if (key == "FUEL") {
        currentVehicleData.fuelLevel = constrain(value, 0, 100);
        Serial.printf("Received fuel level: %.1f%%\n", currentVehicleData.fuelLevel);
    }
    else if (key == "OIL") {
        currentVehicleData.oilWarning = (value != 0);
        Serial.printf("Received oil warning: %s\n", currentVehicleData.oilWarning ? "ON" : "OFF");
    }
    else if (key == "BATTERY") {
        currentVehicleData.batteryVoltage = value;
        Serial.printf("Received battery voltage: %.2fV\n", value);
    }
    else if (key == "DRL") {
        currentVehicleData.drlOn = (value != 0);
        Serial.printf("Received DRL status: %s\n", currentVehicleData.drlOn ? "ON" : "OFF");
    }
    else if (key == "LOWBEAM") {
        currentVehicleData.lowBeamOn = (value != 0);
        Serial.printf("Received low beam status: %s\n", currentVehicleData.lowBeamOn ? "ON" : "OFF");
    }
    else if (key == "GLOW") {
        // Glow plug status (not sent to Android app)
        Serial.printf("Received glow plug status: %s\n", value != 0 ? "ON" : "OFF");
    }
    else {
        Serial.printf("Unknown key: %s\n", key.c_str());
    }
}
