#include "wifi_server.h"

// Global WiFi server instance
WiFiVehicleServer wifiServer;

WiFiVehicleServer::WiFiVehicleServer() :
    currentState(WIFI_DISCONNECTED),
    lastDataSend(0),
    dataSendInterval(250),
    connectionStatusLED(2),
    apSSID(WIFI_SSID),
    apPassword(WIFI_PASSWORD) {
}

void WiFiVehicleServer::init(int ledPin, unsigned long interval) {
    connectionStatusLED = ledPin;
    dataSendInterval = interval;
    
    // Initialize LED pin
    pinMode(connectionStatusLED, OUTPUT);
    digitalWrite(connectionStatusLED, LOW);
    
    
    // Start in AP mode by default
    startAP(apSSID, apPassword);
    
    Serial.println("WiFi Vehicle Server initialized!");
    printWiFiInfo();
}

void WiFiVehicleServer::setDataInterval(unsigned long intervalMs) {
    dataSendInterval = intervalMs;
}

void WiFiVehicleServer::startAP(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;
    
    // Configure WiFi in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    currentState = WIFI_AP_MODE;
    
    // Setup web server
    setupWebServer();
    
    digitalWrite(connectionStatusLED, HIGH);
}


void WiFiVehicleServer::setupWebServer() {
    // Root endpoint - simple status page
    server.on("/", [this]() { handleRoot(); });
    
    // Data endpoint - returns binary vehicle data
    server.on("/data", [this]() { handleData(); });
    
    // Data structure endpoint - returns JSON describing binary data format
    server.on("/data-structure", [this]() { handleDataStructure(); });
    
    // Handle 404
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    Serial.println("Web server started!");
}

void WiFiVehicleServer::handleRoot() {
    String response = "RAVO Car Dashboard Data Server\n";
    response += "==============================\n";
    response += "Status: Access Point Mode\n";
    response += "IP: " + getIPAddress() + "\n";
    response += "SSID: " + getSSID() + "\n";
    response += "MAC: " + WiFi.macAddress() + "\n";
    response += "\nEndpoints:\n";
    response += "- /data           : Get current vehicle data (binary)\n";
    response += "- /data-structure : Get binary data format description (JSON)\n";
    response += "\nServer ready for vehicle data connections.\n";
    
    server.send(200, "text/plain", response);
}

void WiFiVehicleServer::handleData() {
    uint8_t binaryData[64]; // Buffer for binary data
    size_t dataSize = 0;
    
    createBinaryData(binaryData, dataSize);
    
    // Set appropriate headers for binary data
    server.sendHeader("Content-Type", "application/octet-stream");
    server.sendHeader("Content-Length", String(dataSize));
    
    // Create a String from the binary data for the WebServer
    String binaryString = "";
    for (size_t i = 0; i < dataSize; i++) {
        binaryString += (char)binaryData[i];
    }
    
    server.send(200, "application/octet-stream", binaryString);
}

void WiFiVehicleServer::handleDataStructure() {
    String jsonStructure = createDataStructure();
    server.send(200, "application/json", jsonStructure);
}

void WiFiVehicleServer::handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    server.send(404, "text/plain", message);
}

String WiFiVehicleServer::createJsonData() {
    // Create JSON document
    DynamicJsonDocument doc(2048);
    
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
    doc["reverseGear"] = currentVehicleData.reverseGear;
    doc["speed"] = currentVehicleData.speed;
    doc["location"] = currentVehicleData.location;
    
    String jsonString;
    size_t bytesWritten = serializeJson(doc, jsonString);
    
    // Debug: Check if serialization was successful
    if (bytesWritten == 0) {
        Serial.println("ERROR: Failed to serialize JSON data");
        return "{\"error\": \"serialization_failed\"}";
    }
    
    return jsonString;
}

String WiFiVehicleServer::createDataStructure() {
    DynamicJsonDocument doc(1024);
    
    // Define the binary data structure
    doc["reverseGear"]["type"] = "bool";
    doc["reverseGear"]["position"] = 0;
    doc["reverseGear"]["size"] = 1;
    
    doc["hazardLights"]["type"] = "bool";
    doc["hazardLights"]["position"] = 1;
    doc["hazardLights"]["size"] = 1;
    
    doc["rightTurnSignal"]["type"] = "bool";
    doc["rightTurnSignal"]["position"] = 2;
    doc["rightTurnSignal"]["size"] = 1;
    
    doc["leftTurnSignal"]["type"] = "bool";
    doc["leftTurnSignal"]["position"] = 3;
    doc["leftTurnSignal"]["size"] = 1;
    
    doc["highBeamOn"]["type"] = "bool";
    doc["highBeamOn"]["position"] = 4;
    doc["highBeamOn"]["size"] = 1;
    
    doc["lowBeamOn"]["type"] = "bool";
    doc["lowBeamOn"]["position"] = 5;
    doc["lowBeamOn"]["size"] = 1;
    
    doc["drlOn"]["type"] = "bool";
    doc["drlOn"]["position"] = 6;
    doc["drlOn"]["size"] = 1;
    
    doc["oilWarning"]["type"] = "bool";
    doc["oilWarning"]["position"] = 7;
    doc["oilWarning"]["size"] = 1;
    
    doc["speed"]["type"] = "int";
    doc["speed"]["position"] = 1;
    doc["speed"]["size"] = 4;
    doc["speed"]["scale"] = 10;
    
    doc["coolantTemp"]["type"] = "int";
    doc["coolantTemp"]["position"] = 5;
    doc["coolantTemp"]["size"] = 4;
    doc["coolantTemp"]["scale"] = 10;
    
    doc["fuelLevel"]["type"] = "int";
    doc["fuelLevel"]["position"] = 9;
    doc["fuelLevel"]["size"] = 4;
    doc["fuelLevel"]["scale"] = 10;
    
    doc["batteryVoltage"]["type"] = "int";
    doc["batteryVoltage"]["position"] = 13;
    doc["batteryVoltage"]["size"] = 4;
    doc["batteryVoltage"]["scale"] = 10;
    
    // Location is a string, so we'll store it as a length-prefixed string
    doc["location"]["type"] = "string";
    doc["location"]["position"] = 17;
    doc["location"]["size"] = 32; // Max 32 bytes for location string
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void WiFiVehicleServer::createBinaryData(uint8_t* buffer, size_t& size) {
    size_t offset = 0;
    
    // Pack boolean values into bits
    uint8_t boolFlags = 0;
    boolFlags |= (currentVehicleData.reverseGear ? 1 : 0) << 0;
    boolFlags |= (currentVehicleData.hazardLights ? 1 : 0) << 1;
    boolFlags |= (currentVehicleData.rightTurnSignal ? 1 : 0) << 2;
    boolFlags |= (currentVehicleData.leftTurnSignal ? 1 : 0) << 3;
    boolFlags |= (currentVehicleData.highBeamOn ? 1 : 0) << 4;
    boolFlags |= (currentVehicleData.lowBeamOn ? 1 : 0) << 5;
    boolFlags |= (currentVehicleData.drlOn ? 1 : 0) << 6;
    boolFlags |= (currentVehicleData.oilWarning ? 1 : 0) << 7;
    
    buffer[offset++] = boolFlags;
    
    // Pack speed as int (multiply by 10 to preserve 1 decimal place)
    int speedInt = (int)(currentVehicleData.speed * 10);
    memcpy(buffer + offset, &speedInt, sizeof(int));
    offset += sizeof(int);
    
    // Pack coolant temp as int (multiply by 10 to preserve 1 decimal place)
    int coolantTempInt = (int)(currentVehicleData.coolantTemp * 10);
    memcpy(buffer + offset, &coolantTempInt, sizeof(int));
    offset += sizeof(int);
    
    // Pack fuel level as int (multiply by 10 to preserve 1 decimal place)
    int fuelLevelInt = (int)(currentVehicleData.fuelLevel * 10);
    memcpy(buffer + offset, &fuelLevelInt, sizeof(int));
    offset += sizeof(int);
    
    // Pack battery voltage as int (multiply by 10 to preserve 1 decimal place)
    int batteryVoltageInt = (int)(currentVehicleData.batteryVoltage * 10);
    memcpy(buffer + offset, &batteryVoltageInt, sizeof(int));
    offset += sizeof(int);
    
    // Pack location string (null-terminated, max 32 bytes)
    String location = currentVehicleData.location;
    if (location.length() > 31) {
        location = location.substring(0, 31);
    }
    strncpy((char*)(buffer + offset), location.c_str(), 31);
    buffer[offset + 31] = '\0'; // Ensure null termination
    offset += 32;
    
    size = offset;
}

void WiFiVehicleServer::update() {
    // Handle web server requests
    server.handleClient();
    
    // Update LED status - always on when in AP mode
    if (currentState == WIFI_AP_MODE) {
        digitalWrite(connectionStatusLED, HIGH);
    } else {
        digitalWrite(connectionStatusLED, LOW);
    }
}

void WiFiVehicleServer::disconnect() {
    WiFi.disconnect();
    currentState = WIFI_DISCONNECTED;
    digitalWrite(connectionStatusLED, LOW);
}

bool WiFiVehicleServer::isConnected() {
    return currentState == WIFI_AP_MODE;
}

WiFiState WiFiVehicleServer::getState() {
    return currentState;
}

String WiFiVehicleServer::getIPAddress() {
    if (currentState == WIFI_AP_MODE) {
        return WiFi.softAPIP().toString();
    }
    return "Not connected";
}

String WiFiVehicleServer::getSSID() {
    if (currentState == WIFI_AP_MODE) {
        return apSSID;
    }
    return "Unknown";
}

void WiFiVehicleServer::printWiFiInfo() {
}
