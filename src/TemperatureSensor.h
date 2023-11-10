// TemperatureSensor.h
#pragma once

#include <DallasTemperature.h>
#include "TemperatureReading.h"
#include "Config.h"

class TemperatureSensor
{
public:
    TemperatureSensor(OneWire *oneWire);

    void requestTemperatures();
    TemperatureReading *getTemperatureReadings();

    DallasTemperature sensors; // Now public
private:
    TemperatureReading readings[MAX_READINGS + 1]; // +1 for the ending marker
};
