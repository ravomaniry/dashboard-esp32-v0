#include "wifi_server.h"

// Global WiFi server instance
WiFiVehicleServer wifiServer;

WiFiVehicleServer::WiFiVehicleServer() :
    currentState(WIFI_DISCONNECTED),
    lastDataSend(0),
    dataSendInterval(250),
    connectionStatusLED(2),
    apSSID(WIFI_SSID),
    apPassword(WIFI_PASSWORD),
    lastDataChangeTime(0),
    dataHasChanged(false) {
}

void WiFiVehicleServer::init(int ledPin, unsigned long interval) {
    connectionStatusLED = ledPin;
    dataSendInterval = interval;
    
    // Initialize LED pin
    pinMode(connectionStatusLED, OUTPUT);
    digitalWrite(connectionStatusLED, LOW);
    
    // Initialize long polling variables
    lastJsonData = createJsonData();
    lastDataChangeTime = millis();
    dataHasChanged = false;
    
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
    
    // Data endpoint - returns JSON vehicle data (immediate)
    server.on("/data", [this]() { handleData(); });
    
    // Long polling endpoint - waits for data changes
    server.on("/poll", [this]() { handleLongPoll(); });
    
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
    response += "- /poll    : Long polling for real-time updates\n";
    response += "\nServer ready for vehicle data connections.\n";
    
    server.send(200, "text/plain", response);
}

void WiFiVehicleServer::handleData() {
    String jsonData = createJsonData();
    server.send(200, "application/json", jsonData);
}

void WiFiVehicleServer::handleLongPoll() {
    unsigned long startTime = millis();
    unsigned long timeout = LONG_POLL_TIMEOUT_MS;
    
    // Check if client wants a custom timeout
    if (server.hasArg("timeout")) {
        timeout = server.arg("timeout").toInt();
        if (timeout > LONG_POLL_TIMEOUT_MS) {
            timeout = LONG_POLL_TIMEOUT_MS;
        }
    }
    
    // Wait for data to change or timeout
    while (millis() - startTime < timeout) {
        // Check for data changes
        checkForDataChanges();
        
        if (dataHasChanged) {
            String jsonData = createJsonData();
            server.send(200, "application/json", jsonData);
            dataHasChanged = false;
            return;
        }
        
        // Handle other web server requests
        server.handleClient();
        
        // Small delay to prevent overwhelming the system
        delay(10);
    }
    
    // Timeout reached - send current data anyway
    String jsonData = createJsonData();
    server.send(200, "application/json", jsonData);
}

void WiFiVehicleServer::checkForDataChanges() {
    static unsigned long lastCheck = 0;
    unsigned long currentTime = millis();
    
    // Only check every DATA_CHANGE_CHECK_INTERVAL ms
    if (currentTime - lastCheck < DATA_CHANGE_CHECK_INTERVAL) {
        return;
    }
    lastCheck = currentTime;
    
    String currentJsonData = createJsonData();
    
    // Compare with last known data
    if (currentJsonData != lastJsonData) {
        lastJsonData = currentJsonData;
        lastDataChangeTime = currentTime;
        dataHasChanged = true;
    }
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
    DynamicJsonDocument doc(1024);
    
    doc["timestamp"] = millis();
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
    serializeJson(doc, jsonString);
    
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
