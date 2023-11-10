// TemperatureSensor.cpp
#include "TemperatureSensor.h"

// Assuming these are declared and initialized elsewhere in your project
extern DeviceAddress w1Address[MAX_READINGS];
extern String w1Name[MAX_READINGS];

TemperatureSensor::TemperatureSensor(OneWire *oneWire) : sensors(oneWire) {}

void TemperatureSensor::requestTemperatures()
{
    sensors.requestTemperatures();
}

TemperatureReading *TemperatureSensor::getTemperatureReadings()
{
    for (int i = 0; i < MAX_READINGS; i++)
    {
        readings[i] = {w1Name[i], sensors.getTempC(w1Address[i])};
    }
    readings[MAX_READINGS] = {"", 0.0f}; // Ending marker with default values
    return readings;
}
