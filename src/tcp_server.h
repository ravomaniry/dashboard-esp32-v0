#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "vehicle_data.h"

// TCP configuration
#define TCP_PORT 8888
#define MAX_TCP_CLIENTS 4
#define TCP_BUFFER_SIZE 64

// TCP server class
class TCPServer {
private:
    WiFiServer tcpServer;
    WiFiClient tcpClients[MAX_TCP_CLIENTS];
    bool clientConnected[MAX_TCP_CLIENTS];
    unsigned long lastDataSend;
    unsigned long dataSendInterval;
    int connectionStatusLED;
    String apSSID;
    String apPassword;
    
    void handleNewClients();
    void handleClientData();
    void sendDataToClients();
    void createBinaryData(uint8_t* buffer, size_t& size);
    void disconnectClient(int clientIndex);

public:
    TCPServer();
    
    void init(int ledPin = 2, unsigned long interval = 250);
    void setDataInterval(unsigned long intervalMs);
    void update();
    void disconnect();
    
    bool isConnected();
    int getConnectedClients();
    String getIPAddress();
    String getSSID();
    
    // WiFi management
    void startAP(const String& ssid, const String& password);
    void printServerInfo();
};

// Global TCP server instance
extern TCPServer tcpServer;

#endif
