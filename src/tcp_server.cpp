#include "tcp_server.h"
#include <Arduino.h>
#include <WiFi.h>
#include "vehicle_data.h"

// Global TCP server instance
TCPServer tcpServer;

TCPServer::TCPServer() :
    tcpServer(TCP_PORT),
    lastDataSend(0),
    dataSendInterval(250),
    connectionStatusLED(2),
    apSSID("RAVO_CAR_DASH"),
    apPassword("car123456") {
    
    // Initialize client connection states
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        clientConnected[i] = false;
    }
}

void TCPServer::init(int ledPin, unsigned long interval) {
    connectionStatusLED = ledPin;
    dataSendInterval = interval;
    
    // Initialize LED pin
    pinMode(connectionStatusLED, OUTPUT);
    digitalWrite(connectionStatusLED, LOW);
    
    // Start in AP mode
    startAP(apSSID, apPassword);
    
    // Start TCP server
    tcpServer.begin();
    tcpServer.setNoDelay(true); // Disable Nagle's algorithm for lower latency
    
    // TCP Server initialized
}

void TCPServer::setDataInterval(unsigned long intervalMs) {
    dataSendInterval = intervalMs;
}

void TCPServer::startAP(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;
    
    // Configure WiFi in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    digitalWrite(connectionStatusLED, HIGH);
}

void TCPServer::update() {
    // Handle new client connections
    handleNewClients();
    
    // Handle data from clients
    handleClientData();
    
    // Send data to all connected clients
    sendDataToClients();
    
    // Update LED status
    bool anyClientConnected = false;
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clientConnected[i]) {
            anyClientConnected = true;
            break;
        }
    }
    
    if (anyClientConnected) {
        digitalWrite(connectionStatusLED, HIGH);
    } else {
        digitalWrite(connectionStatusLED, LOW);
    }
}

void TCPServer::handleNewClients() {
    // Check for new client connections
    if (tcpServer.hasClient()) {
        WiFiClient newClient = tcpServer.available();
        
        // Find an available slot
        for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
            if (!clientConnected[i]) {
                tcpClients[i] = newClient;
                clientConnected[i] = true;
                // New client connected
                break;
            }
        }
    }
}

void TCPServer::handleClientData() {
    // Handle incoming data from clients (if needed)
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clientConnected[i] && tcpClients[i].connected()) {
            if (tcpClients[i].available()) {
                // Read and discard any incoming data for now
                // You can implement command handling here if needed
                while (tcpClients[i].available()) {
                    tcpClients[i].read();
                }
            }
        } else if (clientConnected[i] && !tcpClients[i].connected()) {
            // Client disconnected
            disconnectClient(i);
        }
    }
}

void TCPServer::sendDataToClients() {
    unsigned long currentTime = millis();
    
    // Check if it's time to send data
    if (currentTime - lastDataSend >= dataSendInterval) {
        lastDataSend = currentTime;
        
        // Create binary data
        uint8_t binaryData[TCP_BUFFER_SIZE];
        size_t dataSize = 0;
        createBinaryData(binaryData, dataSize);
        
        // Send data to all connected clients
        for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
            if (clientConnected[i] && tcpClients[i].connected()) {
                size_t bytesWritten = tcpClients[i].write(binaryData, dataSize);
                tcpClients[i].flush(); // Ensure data is sent immediately
                
                // Data sent to client
            }
        }
    }
}

void TCPServer::createBinaryData(uint8_t* buffer, size_t& size) {
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

void TCPServer::disconnectClient(int clientIndex) {
    if (clientIndex >= 0 && clientIndex < MAX_TCP_CLIENTS) {
        tcpClients[clientIndex].stop();
        clientConnected[clientIndex] = false;
        // Client disconnected
    }
}

void TCPServer::disconnect() {
    // Disconnect all clients
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clientConnected[i]) {
            disconnectClient(i);
        }
    }
    
    WiFi.disconnect();
    digitalWrite(connectionStatusLED, LOW);
}

bool TCPServer::isConnected() {
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clientConnected[i] && tcpClients[i].connected()) {
            return true;
        }
    }
    return false;
}

int TCPServer::getConnectedClients() {
    int count = 0;
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clientConnected[i] && tcpClients[i].connected()) {
            count++;
        }
    }
    return count;
}

String TCPServer::getIPAddress() {
    return WiFi.softAPIP().toString();
}

String TCPServer::getSSID() {
    return apSSID;
}

void TCPServer::printServerInfo() {
    // Server info available via getIPAddress() and getSSID() methods
}
