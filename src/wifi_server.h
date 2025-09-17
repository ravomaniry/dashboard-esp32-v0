#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "vehicle_data.h"

// WiFi configuration
#define WIFI_SSID "RAVO_CAR_DASH"
#define WIFI_PASSWORD "car123456"
#define WIFI_PORT 80
#define MAX_CLIENTS 4

// Connection states
enum WiFiState {
    WIFI_DISCONNECTED,
    WIFI_AP_MODE
};

// WiFi server class
class WiFiVehicleServer {
private:
    WebServer server;
    WiFiState currentState;
    unsigned long lastDataSend;
    unsigned long dataSendInterval;
    int connectionStatusLED;
    String apSSID;
    String apPassword;
    
    
    void setupAP();
    void setupWebServer();
    void sendVehicleData();
    String createJsonData();
    void handleRoot();
    void handleData();
    void handleNotFound();

public:
    WiFiVehicleServer();
    
    void init(int ledPin = 2, unsigned long interval = 250);
    void setDataInterval(unsigned long intervalMs);
    void update();
    void disconnect();
    
    bool isConnected();
    WiFiState getState();
    String getIPAddress();
    String getSSID();
    
    // WiFi management
    void printWiFiInfo();
    void startAP(const String& ssid, const String& password);
};

// Global WiFi server instance
extern WiFiVehicleServer wifiServer;

#endif
