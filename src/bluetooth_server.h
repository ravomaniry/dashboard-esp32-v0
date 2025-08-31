#ifndef BLUETOOTH_SERVER_H
#define BLUETOOTH_SERVER_H

#include <Arduino.h>
#include "BluetoothSerial.h"
#include "vehicle_data.h"

// Bluetooth configuration
#define BT_DEVICE_NAME "RAVO_CAR_DASH"
#define MAX_WHITELIST_SIZE 10

// Connection states
enum BluetoothState {
    BT_DISCONNECTED,
    BT_CONNECTED
};

// MAC address whitelist - add your authorized devices here
extern const char* AUTHORIZED_MACS[];
extern const int AUTHORIZED_MACS_COUNT;

// Bluetooth server class
class BluetoothVehicleServer {
private:
    BluetoothSerial SerialBT;
    BluetoothState currentState;
    unsigned long lastDataSend;
    unsigned long dataSendInterval;
    int connectionStatusLED;
    
    bool isMACAuthorized(const String& macAddress);
    void sendVehicleData();
    String createJsonData();

public:
    BluetoothVehicleServer();
    
    void init(int ledPin = 2, unsigned long interval = 250);
    void setDataInterval(unsigned long intervalMs);
    void update();
    void disconnect();
    
    bool isConnected();
    BluetoothState getState();
    
    // MAC address management
    void printAuthorizedMACs();
};

// Global Bluetooth server instance
extern BluetoothVehicleServer btServer;

#endif
