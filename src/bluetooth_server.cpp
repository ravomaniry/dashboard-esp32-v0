#include "bluetooth_server.h"
#include "ArduinoJson.h"
#include <WiFi.h>

// MAC address whitelist - add your authorized devices here
const char* AUTHORIZED_MACS[] = {
    "14:8:8:a6:17:c2",
};
const int AUTHORIZED_MACS_COUNT = sizeof(AUTHORIZED_MACS) / sizeof(AUTHORIZED_MACS[0]);

// Global Bluetooth server instance
BluetoothVehicleServer btServer;

BluetoothVehicleServer::BluetoothVehicleServer() :
    currentState(BT_DISCONNECTED),
    lastDataSend(0),
    dataSendInterval(250),
    connectionStatusLED(2) {
}

void BluetoothVehicleServer::init(int ledPin, unsigned long interval) {
    connectionStatusLED = ledPin;
    dataSendInterval = interval;
    
    // Initialize LED pin
    pinMode(connectionStatusLED, OUTPUT);
    digitalWrite(connectionStatusLED, LOW);
    
    // Initialize Bluetooth
    if (!SerialBT.begin(BT_DEVICE_NAME)) {
        Serial.println("An error occurred initializing Bluetooth");
        return;
    }
    
    Serial.printf("Bluetooth device \"%s\" started, ready for connections!\n", BT_DEVICE_NAME);
    Serial.println("Bluetooth MAC address: " + WiFi.macAddress());
    Serial.println("MAC address authorization enabled - only authorized devices can connect");
    
    printAuthorizedMACs();
}

void BluetoothVehicleServer::setDataInterval(unsigned long intervalMs) {
    dataSendInterval = intervalMs;
}

bool BluetoothVehicleServer::isMACAuthorized(const String& macAddress) {
    for (int i = 0; i < AUTHORIZED_MACS_COUNT; i++) {
        if (String(AUTHORIZED_MACS[i]).equalsIgnoreCase(macAddress)) {
            return true;
        }
    }
    return false;
}

void BluetoothVehicleServer::printAuthorizedMACs() {
    Serial.println("=== Authorized MAC Addresses ===");
    for (int i = 0; i < AUTHORIZED_MACS_COUNT; i++) {
        Serial.printf("%d: %s\n", i + 1, AUTHORIZED_MACS[i]);
    }
    Serial.println("================================");
}

String BluetoothVehicleServer::createJsonData() {
    // Create JSON document
    DynamicJsonDocument doc(512);
    
    doc["coolantTemp"] = currentVehicleData.coolantTemp;
    doc["fuelLevel"] = currentVehicleData.fuelLevel;
    doc["oilWarning"] = currentVehicleData.oilWarning;
    doc["batteryVoltage"] = currentVehicleData.batteryVoltage;
    doc["drlOn"] = currentVehicleData.drlOn;
    doc["lowBeamOn"] = currentVehicleData.lowBeamOn;
    doc["highBeamOn"] = currentVehicleData.highBeamOn;
    doc["leftTurnSignal"] = currentVehicleData.leftTurnSignal;
    doc["rightTurnSignal"] = currentVehicleData.rightTurnSignal;
    doc["hazardLights"] = currentVehicleData.hazardLights;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    return jsonString;
}

void BluetoothVehicleServer::sendVehicleData() {
    if (currentState != BT_CONNECTED) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastDataSend >= dataSendInterval) {
        String jsonData = createJsonData();
        
        // Send data with error checking
        if (SerialBT.println(jsonData)) {
            // Debug output (comment out for production)
            Serial.println("Sent: " + jsonData);
        } else {
            Serial.println("Failed to send data");
        }
        
        lastDataSend = currentTime;
    }
}

void BluetoothVehicleServer::update() {
    // Check if client is connected by trying to read connection status
    static unsigned long lastConnectionCheck = 0;
    unsigned long currentTime = millis();
    
    // Check connection status every 2 seconds
    if (currentTime - lastConnectionCheck >= 2000) {
        lastConnectionCheck = currentTime;
        
        // Try to detect connection by checking if we can write
        if (SerialBT.hasClient()) {
            if (currentState == BT_DISCONNECTED) {
                // Get the connected device's MAC address
                uint8_t mac[6];
                SerialBT.getBtAddress(mac);
                String clientMAC = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX);
                Serial.printf("Bluetooth client attempting to connect: %s\n", clientMAC.c_str());
                
                // Check if MAC is authorized
                if (isMACAuthorized(clientMAC)) {
                    Serial.println("MAC address authorized - connection accepted!");
                    currentState = BT_CONNECTED;
                    digitalWrite(connectionStatusLED, HIGH);
                } else {
                    Serial.println("MAC address not authorized - connection rejected!");
                    SerialBT.disconnect();
                    digitalWrite(connectionStatusLED, LOW);
                }
            }
        } else {
            if (currentState != BT_DISCONNECTED) {
                Serial.println("Bluetooth client disconnected!");
                currentState = BT_DISCONNECTED;
                digitalWrite(connectionStatusLED, LOW);
            }
        }
    }
    
    // Handle incoming data (just for debugging)
    if (SerialBT.available()) {
        String message = SerialBT.readStringUntil('\n');
        message.trim();
        Serial.println("Received: " + message);
    }
    
    // Send vehicle data if connected
    if (currentState == BT_CONNECTED) {
        sendVehicleData();
    }
    
    // Update LED status
    if (currentState == BT_CONNECTED) {
        // Solid LED when connected
        digitalWrite(connectionStatusLED, HIGH);
    } else {
        // LED off when disconnected
        digitalWrite(connectionStatusLED, LOW);
    }
}

void BluetoothVehicleServer::disconnect() {
    SerialBT.disconnect();
    currentState = BT_DISCONNECTED;
    digitalWrite(connectionStatusLED, LOW);
}

bool BluetoothVehicleServer::isConnected() {
    return currentState == BT_CONNECTED;
}

BluetoothState BluetoothVehicleServer::getState() {
    return currentState;
}
