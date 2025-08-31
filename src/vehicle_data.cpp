#include "vehicle_data.h"
#include <math.h>

// Critical thresholds
const int LOW_FUEL = 10;
const int OPTIMUM_TEMP_MIN = 85;
const int OPTIMUM_TEMP_MAX = 95;

// Global vehicle data
VehicleData currentVehicleData = {0};

// Sensor pin configuration - adjust these based on your wiring
SensorPins sensorPins = {
    .coolantTempPin = A0,      // Analog pin for coolant temperature sensor
    .fuelLevelPin = A1,        // Analog pin for fuel level sensor
    .oilPressurePin = 2,       // Digital pin for oil pressure switch
    .batteryVoltagePin = A2,   // Analog pin for battery voltage (through voltage divider)
    .drlPin = 3,              // Digital pin for DRL status
    .lowBeamPin = 4,          // Digital pin for low beam status
    .highBeamPin = 5,         // Digital pin for high beam status
    .leftTurnPin = 6,         // Digital pin for left turn signal
    .rightTurnPin = 7,        // Digital pin for right turn signal
    .hazardPin = 8            // Digital pin for hazard lights
};

void initSensors() {
    // Initialize digital input pins with pullup resistors
    pinMode(sensorPins.oilPressurePin, INPUT_PULLUP);
    pinMode(sensorPins.drlPin, INPUT_PULLUP);
    pinMode(sensorPins.lowBeamPin, INPUT_PULLUP);
    pinMode(sensorPins.highBeamPin, INPUT_PULLUP);
    pinMode(sensorPins.leftTurnPin, INPUT_PULLUP);
    pinMode(sensorPins.rightTurnPin, INPUT_PULLUP);
    pinMode(sensorPins.hazardPin, INPUT_PULLUP);
    
    // Analog pins don't need initialization
    Serial.println("Sensors initialized");
}

float readCoolantTemp() {
    // Read analog value from coolant temperature sensor
    int rawValue = analogRead(sensorPins.coolantTempPin);
    
    // Convert to temperature based on sensor type
    // Example for LM35 (10mV/°C): Temp = (analogValue * 5.0 / 1024.0) * 100
    // Example for automotive sensor: custom conversion based on resistance curve
    
    // For LM35 sensor:
    float voltage = (rawValue * 5.0) / 1024.0;
    float temperature = voltage * 100.0; // LM35 outputs 10mV per degree Celsius
    
    // Apply calibration offset if needed
    temperature += 0.0; // Calibration offset
    
    // Clamp to reasonable range
    if (temperature < -40) temperature = -40;
    if (temperature > 150) temperature = 150;
    
    return temperature;
}

float readFuelLevel() {
    // Read analog value from fuel level sensor
    int rawValue = analogRead(sensorPins.fuelLevelPin);
    
    // Convert to percentage (0-100%)
    // Fuel senders typically vary resistance from empty to full
    // Example: 240Ω empty, 33Ω full (common automotive standard)
    
    // Simple linear conversion (adjust based on your fuel sender)
    float percentage = map(rawValue, 0, 1023, 0, 100);
    
    // Apply calibration if needed
    percentage = constrain(percentage, 0, 100);
    
    return percentage;
}

bool readOilWarning() {
    // Read oil pressure switch (typically LOW = oil pressure OK, HIGH = low pressure warning)
    // Using INPUT_PULLUP, so we invert the reading
    return !digitalRead(sensorPins.oilPressurePin);
}

float readBatteryVoltage() {
    // Read battery voltage through voltage divider
    // For 12V system, use voltage divider (e.g., 10kΩ and 10kΩ for 1:2 ratio)
    // This gives max 6V input to ESP32 (safe for 3.3V ADC with some headroom)
    
    int rawValue = analogRead(sensorPins.batteryVoltagePin);
    
    // Convert to actual voltage
    // ESP32 ADC: 0-4095 for 0-3.3V, but with voltage divider we need to scale up
    float adcVoltage = (rawValue * 3.3) / 4095.0;
    
    // Scale back up through voltage divider ratio (adjust based on your divider)
    float batteryVoltage = adcVoltage * 2.0; // For 1:1 voltage divider
    
    // Apply calibration
    batteryVoltage += 0.0; // Calibration offset
    
    return batteryVoltage;
}

bool readDRLStatus() {
    // Read DRL status (inverted because of pullup)
    return !digitalRead(sensorPins.drlPin);
}

bool readLowBeamStatus() {
    // Read low beam status (inverted because of pullup)
    return !digitalRead(sensorPins.lowBeamPin);
}

bool readHighBeamStatus() {
    // Read high beam status (inverted because of pullup)
    return !digitalRead(sensorPins.highBeamPin);
}

bool readLeftTurnSignal() {
    // Read left turn signal status (inverted because of pullup)
    return !digitalRead(sensorPins.leftTurnPin);
}

bool readRightTurnSignal() {
    // Read right turn signal status (inverted because of pullup)
    return !digitalRead(sensorPins.rightTurnPin);
}

bool readHazardLights() {
    // Read hazard lights status (inverted because of pullup)
    return !digitalRead(sensorPins.hazardPin);
}

void updateAllSensors() {
    currentVehicleData.coolantTemp = readCoolantTemp();
    currentVehicleData.fuelLevel = readFuelLevel();
    currentVehicleData.oilWarning = readOilWarning();
    currentVehicleData.batteryVoltage = readBatteryVoltage();
    currentVehicleData.drlOn = readDRLStatus();
    currentVehicleData.lowBeamOn = readLowBeamStatus();
    currentVehicleData.highBeamOn = readHighBeamStatus();
    currentVehicleData.leftTurnSignal = readLeftTurnSignal();
    currentVehicleData.rightTurnSignal = readRightTurnSignal();
    currentVehicleData.hazardLights = readHazardLights();
}

bool isCritical(int value, int safeMin, int safeMax, bool twoSided, bool oilGauge, bool oilValue) {
    if(oilGauge) return oilValue; // LOW = critical
    if(twoSided) return value < safeMin || value > safeMax;
    return value < safeMin;
}
