#include "bluetooth_server.h"
#include "ArduinoJson.h"
#include "mbedtls/md.h"
#include <WiFi.h>

// Global Bluetooth server instance
BluetoothVehicleServer btServer;

BluetoothVehicleServer::BluetoothVehicleServer() :
    currentState(BT_DISCONNECTED),
    lastDataSend(0),
    dataSendInterval(250),
    connectionStatusLED(LED_BUILTIN) {
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
    
    // Set up callbacks
    SerialBT.onConnect([this](uint16_t rfcomm) {
        this->onConnect();
    });
    
    SerialBT.onDisConnect([this](uint16_t rfcomm) {
        this->onDisconnect();
    });
}

void BluetoothVehicleServer::setDataInterval(unsigned long intervalMs) {
    dataSendInterval = intervalMs;
}

String BluetoothVehicleServer::generateChallenge() {
    String challenge = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    for (int i = 0; i < CHALLENGE_LENGTH; i++) {
        challenge += charset[random(0, sizeof(charset) - 1)];
    }
    
    return challenge;
}

String BluetoothVehicleServer::calculateExpectedHash(const String& challenge) {
    // Calculate SHA256 hash of challenge + salt
    String input = challenge + AUTH_SALT;
    
    // Use mbedtls for SHA256
    unsigned char hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)input.c_str(), input.length());
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    
    // Convert to hex string
    String hashStr = "";
    for (int i = 0; i < 32; i++) {
        if (hash[i] < 16) hashStr += "0";
        hashStr += String(hash[i], HEX);
    }
    
    return hashStr;
}

bool BluetoothVehicleServer::verifyAuthentication(const String& response) {
    String expectedHash = calculateExpectedHash(currentChallenge);
    return response.equalsIgnoreCase(expectedHash);
}

void BluetoothVehicleServer::sendAuthChallenge() {
    currentChallenge = generateChallenge();
    String challengeMsg = "CHALLENGE:" + currentChallenge + "\n";
    SerialBT.print(challengeMsg);
    Serial.println("Sent challenge: " + challengeMsg);
}

void BluetoothVehicleServer::handleAuthResponse(const String& response) {
    if (verifyAuthentication(response)) {
        currentState = BT_AUTHENTICATED;
        SerialBT.println("AUTH OK");
        Serial.println("Authentication successful");
        digitalWrite(connectionStatusLED, HIGH); // Solid light for authenticated
    } else {
        SerialBT.println("AUTH FAIL");
        Serial.println("Authentication failed");
        currentState = BT_CONNECTED; // Stay connected but not authenticated
    }
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
    if (currentState != BT_AUTHENTICATED) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastDataSend >= dataSendInterval) {
        String jsonData = createJsonData();
        SerialBT.println(jsonData);
        
        // Debug output (comment out for production)
        Serial.println("Sent: " + jsonData);
        
        lastDataSend = currentTime;
    }
}

void BluetoothVehicleServer::update() {
    // Handle incoming data
    if (SerialBT.available()) {
        String message = SerialBT.readStringUntil('\n');
        message.trim();
        
        Serial.println("Received: " + message);
        
        if (currentState == BT_AUTHENTICATING) {
            handleAuthResponse(message);
        }
        // Add other message handling here if needed
    }
    
    // Send vehicle data if authenticated
    if (currentState == BT_AUTHENTICATED) {
        sendVehicleData();
    }
    
    // Update LED status
    if (currentState == BT_CONNECTED) {
        // Blink LED while waiting for authentication
        digitalWrite(connectionStatusLED, (millis() / 500) % 2);
    } else if (currentState == BT_AUTHENTICATED) {
        // Solid LED when authenticated
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
    return currentState != BT_DISCONNECTED;
}

bool BluetoothVehicleServer::isAuthenticated() {
    return currentState == BT_AUTHENTICATED;
}

BluetoothState BluetoothVehicleServer::getState() {
    return currentState;
}

void BluetoothVehicleServer::onConnect() {
    Serial.println("Bluetooth client connected");
    currentState = BT_CONNECTED;
    
    // Wait a moment then send challenge
    delay(100);
    currentState = BT_AUTHENTICATING;
    sendAuthChallenge();
}

void BluetoothVehicleServer::onDisconnect() {
    Serial.println("Bluetooth client disconnected");
    currentState = BT_DISCONNECTED;
    digitalWrite(connectionStatusLED, LOW);
}
