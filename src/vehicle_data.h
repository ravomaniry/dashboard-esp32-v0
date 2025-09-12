#ifndef VEHICLE_DATA_H
#define VEHICLE_DATA_H

#include <Arduino.h>

// Vehicle sensor data structure
struct VehicleData {
    float coolantTemp;      // Coolant temperature in Celsius
    float fuelLevel;        // Fuel level percentage (0-100)
    bool oilWarning;        // Oil pressure warning (true = low pressure)
    float batteryVoltage;   // Battery voltage
    bool drlOn;            // Daytime running lights
    bool lowBeamOn;        // Low beam headlights
    bool highBeamOn;       // High beam headlights
    bool leftTurnSignal;   // Left turn signal
    bool rightTurnSignal;  // Right turn signal
    bool hazardLights;     // Hazard lights
    float speed;           // Vehicle speed in km/h
    String location;       // GPS location in "lat,lon" format
};

// Sensor pin definitions
struct SensorPins {
    int coolantTempPin;
    int fuelLevelPin;
    int oilPressurePin;
    int batteryVoltagePin;
    int drlPin;
    int lowBeamPin;
    int highBeamPin;
    int leftTurnPin;
    int rightTurnPin;
    int hazardPin;
};

// Critical thresholds
extern const int LOW_FUEL;
extern const int OPTIMUM_TEMP_MIN;
extern const int OPTIMUM_TEMP_MAX;

// Global vehicle data
extern VehicleData currentVehicleData;
extern SensorPins sensorPins;

// Sensor reading functions
void initSensors();
float readCoolantTemp();
float readFuelLevel();
bool readOilWarning();
float readBatteryVoltage();
bool readDRLStatus();
bool readLowBeamStatus();
bool readHighBeamStatus();
bool readLeftTurnSignal();
bool readRightTurnSignal();
bool readHazardLights();

// Update all sensors
void updateAllSensors();

// Utility functions
bool isCritical(int value, int safeMin, int safeMax, bool twoSided = false, bool oilGauge = false, bool oilValue = false);

#endif
