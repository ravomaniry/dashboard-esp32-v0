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
    
    // Data endpoint - returns JSON vehicle data
    server.on("/data", [this]() { handleData(); });
    
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
    response += "- /data    : Get current vehicle data (JSON)\n";
    response += "\nServer ready for vehicle data connections.\n";
    
    server.send(200, "text/plain", response);
}

void WiFiVehicleServer::handleData() {
    String jsonData = createJsonData();
    server.send(200, "application/json", jsonData);
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
