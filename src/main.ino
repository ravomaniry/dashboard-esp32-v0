/*
 * ESP32 Vehicle Dashboard with Bluetooth Server
 * 
 * This code implements a complete vehicle dashboard system that:
 * 1. Reads various vehicle sensors (coolant, fuel, oil, battery, lights)
 * 2. Displays dashboard on composite video output
 * 3. Acts as Bluetooth server to send sensor data to Android app
 * 4. Includes authentication protocol for secure communication
 */

// Core ESP32 libraries
#include "esp_pm.h"
#include <soc/rtc.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Display libraries
#include "CompositeGraphics.h"
#include "CompositeColorOutput.h"
#include "font6x8.h"

// Our modular components
#include "graphics_utils.h"
#include "vehicle_data.h"
#include "bluetooth_server.h"
#include "dashboard.h"

// Display objects
CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES, 0);
CompositeColorOutput composite(CompositeColorOutput::NTSC);
Font<CompositeGraphics> font(6, 8, font6x8::pixels);

// Configuration
#define CONNECTION_LED_PIN 2  // Built-in LED pin for most ESP32 boards
#define DATA_SEND_INTERVAL 250  // Send data every 250ms (4Hz)

// Demo mode - set to true to cycle through test values
bool IS_DEMO = false;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println("ESP32 Vehicle Dashboard starting...");
    
    // Configure power management for stable operation
    esp_pm_lock_handle_t lock;
    esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "compositeLock", &lock);
    esp_pm_lock_acquire(lock);
    
    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize display system
    Serial.println("Initializing display...");
    composite.init();
    graphics.init();
    graphics.setFont(font);
    
    // Initialize modular components
    Serial.println("Initializing graphics utilities...");
    initGraphicsUtils(&graphics);
    
    Serial.println("Initializing dashboard...");
    initDashboard(&graphics);
    
    Serial.println("Initializing vehicle sensors...");
    initSensors();
    
    Serial.println("Initializing Bluetooth server...");
    btServer.init(CONNECTION_LED_PIN, DATA_SEND_INTERVAL);
    
    Serial.println("Setup complete!");
    Serial.printf("Device name: %s\n", "RAVO_CAR_DASH");
    Serial.println("Ready for Bluetooth connections.");
    
    // Initial sensor reading
    updateAllSensors();
    
    // Display initial dashboard
    drawDashboard();
    composite.sendFrameHalfResolution(&graphics.frame);
}

void loop() {
    static unsigned long lastSensorUpdate = 0;
    static unsigned long lastDisplayUpdate = 0;
    
    unsigned long currentTime = millis();
    
    // Update sensors every 100ms
    if (currentTime - lastSensorUpdate >= 100) {
        if (IS_DEMO) {
            randomizeValues();  // Use demo values
        } else {
            updateAllSensors(); // Read real sensor data
        }
        lastSensorUpdate = currentTime;
    }
    
    // Update display every 50ms (20 FPS)
    if (currentTime - lastDisplayUpdate >= 50) {
        drawDashboard();
        composite.sendFrameHalfResolution(&graphics.frame);
        lastDisplayUpdate = currentTime;
    }
    
    // Handle Bluetooth communication
    btServer.update();
    
    // Handle serial commands for testing/debugging
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        handleSerialCommand(command);
    }
    
    // Small delay to prevent watchdog issues
    delayMicroseconds(100);
}

/**
 * Handle serial commands for testing and configuration
 * Commands:
 * - DEMO:1 or DEMO:0 - Enable/disable demo mode
 * - INTERVAL:xxx - Set Bluetooth data send interval in ms
 * - STATUS - Show current status
 * - SENSOR:xxx:yyy - Set specific sensor value for testing
 */
void handleSerialCommand(const String& command) {
    if (command.startsWith("DEMO:")) {
        int value = command.substring(5).toInt();
        IS_DEMO = (value == 1);
        Serial.printf("Demo mode: %s\n", IS_DEMO ? "ON" : "OFF");
    }
    else if (command.startsWith("INTERVAL:")) {
        unsigned long interval = command.substring(9).toInt();
        if (interval >= 50 && interval <= 5000) {
            btServer.setDataInterval(interval);
            Serial.printf("Data send interval set to: %lu ms\n", interval);
        } else {
            Serial.println("Invalid interval. Use 50-5000 ms.");
        }
    }
    else if (command == "STATUS") {
        Serial.println("=== ESP32 Vehicle Dashboard Status ===");
        Serial.printf("Demo mode: %s\n", IS_DEMO ? "ON" : "OFF");
        Serial.printf("Bluetooth state: %d\n", btServer.getState());
        Serial.printf("Connected: %s\n", btServer.isConnected() ? "YES" : "NO");
        Serial.printf("Authenticated: %s\n", btServer.isAuthenticated() ? "YES" : "NO");
        Serial.println("Current sensor values:");
        Serial.printf("  Coolant temp: %.1f°C\n", currentVehicleData.coolantTemp);
        Serial.printf("  Fuel level: %.1f%%\n", currentVehicleData.fuelLevel);
        Serial.printf("  Oil warning: %s\n", currentVehicleData.oilWarning ? "ON" : "OFF");
        Serial.printf("  Battery voltage: %.2fV\n", currentVehicleData.batteryVoltage);
        Serial.printf("  DRL: %s\n", currentVehicleData.drlOn ? "ON" : "OFF");
        Serial.printf("  Low beam: %s\n", currentVehicleData.lowBeamOn ? "ON" : "OFF");
        Serial.printf("  High beam: %s\n", currentVehicleData.highBeamOn ? "ON" : "OFF");
        Serial.printf("  Left turn: %s\n", currentVehicleData.leftTurnSignal ? "ON" : "OFF");
        Serial.printf("  Right turn: %s\n", currentVehicleData.rightTurnSignal ? "ON" : "OFF");
        Serial.printf("  Hazard: %s\n", currentVehicleData.hazardLights ? "ON" : "OFF");
    }
    else if (command.startsWith("SENSOR:")) {
        // Parse SENSOR:type:value commands for testing
        int firstColon = command.indexOf(':', 7);
        if (firstColon != -1) {
            String sensorType = command.substring(7, firstColon);
            float value = command.substring(firstColon + 1).toFloat();
            
            if (sensorType == "COOLANT") {
                currentVehicleData.coolantTemp = value;
                Serial.printf("Coolant temperature set to: %.1f°C\n", value);
            }
            else if (sensorType == "FUEL") {
                currentVehicleData.fuelLevel = constrain(value, 0, 100);
                Serial.printf("Fuel level set to: %.1f%%\n", currentVehicleData.fuelLevel);
            }
            else if (sensorType == "BATTERY") {
                currentVehicleData.batteryVoltage = value;
                Serial.printf("Battery voltage set to: %.2fV\n", value);
            }
            else if (sensorType == "OIL") {
                currentVehicleData.oilWarning = (value != 0);
                Serial.printf("Oil warning set to: %s\n", currentVehicleData.oilWarning ? "ON" : "OFF");
            }
            else {
                Serial.println("Unknown sensor type. Use: COOLANT, FUEL, BATTERY, OIL");
            }
        }
    }
    else if (command == "HELP") {
        Serial.println("=== Available Commands ===");
        Serial.println("DEMO:1/0 - Enable/disable demo mode");
        Serial.println("INTERVAL:xxx - Set data send interval (50-5000ms)");
        Serial.println("STATUS - Show current status");
        Serial.println("SENSOR:type:value - Set sensor value for testing");
        Serial.println("  Types: COOLANT, FUEL, BATTERY, OIL");
        Serial.println("HELP - Show this help");
    }
    else {
        Serial.println("Unknown command. Type HELP for available commands.");
    }
}
