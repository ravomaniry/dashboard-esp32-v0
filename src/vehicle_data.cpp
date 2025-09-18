#include "vehicle_data.h"
#include <math.h>

// Critical thresholds
const int LOW_FUEL = 10;
const int OPTIMUM_TEMP_MIN = 85;
const int OPTIMUM_TEMP_MAX = 95;

// Global vehicle data
VehicleData currentVehicleData = {0};

void initSensors() {
    // Initialize with zero/safe default values
    currentVehicleData.coolantTemp = 0.0;
    currentVehicleData.fuelLevel = 0.0;
    currentVehicleData.oilWarning = false;
    currentVehicleData.batteryVoltage = 0.0;
    currentVehicleData.drlOn = false;
    currentVehicleData.lowBeamOn = false;
    currentVehicleData.highBeamOn = false;
    currentVehicleData.leftTurnSignal = false;
    currentVehicleData.rightTurnSignal = false;
    currentVehicleData.hazardLights = false;
    currentVehicleData.reverseGear = false;
    currentVehicleData.speed = 0.0;
    currentVehicleData.location = "0.0,0.0";
}

bool isCritical(int value, int safeMin, int safeMax, bool twoSided, bool oilGauge, bool oilValue) {
    if(oilGauge) return oilValue; // LOW = critical
    if(twoSided) return value < safeMin || value > safeMax;
    return value < safeMin;
}
