#ifndef BLUETOOTH_SERVER_H
#define BLUETOOTH_SERVER_H

#include <Arduino.h>
#include "BluetoothSerial.h"
#include "vehicle_data.h"

// Bluetooth configuration
#define BT_DEVICE_NAME "RAVO_CAR_DASH"
#define AUTH_SALT "super_secret_salt"
#define CHALLENGE_LENGTH 16

// Connection states
enum BluetoothState {
    BT_DISCONNECTED,
    BT_CONNECTED,
    BT_AUTHENTICATING,
    BT_AUTHENTICATED
};

// Bluetooth server class
class BluetoothVehicleServer {
private:
    BluetoothSerial SerialBT;
    BluetoothState currentState;
    String currentChallenge;
    unsigned long lastDataSend;
    unsigned long dataSendInterval;
    int connectionStatusLED;
    
    String generateChallenge();
    String calculateExpectedHash(const String& challenge);
    bool verifyAuthentication(const String& response);
    void sendAuthChallenge();
    void handleAuthResponse(const String& response);
    void sendVehicleData();
    String createJsonData();

public:
    BluetoothVehicleServer();
    
    void init(int ledPin = LED_BUILTIN, unsigned long interval = 250);
    void setDataInterval(unsigned long intervalMs);
    void update();
    void disconnect();
    
    bool isConnected();
    bool isAuthenticated();
    BluetoothState getState();
    
    // Callbacks
    void onConnect();
    void onDisconnect();
};

// Global Bluetooth server instance
extern BluetoothVehicleServer btServer;

#endif
