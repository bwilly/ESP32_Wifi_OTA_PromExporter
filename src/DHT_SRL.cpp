#include <Arduino.h>
#include <DHT_SRL.h>
#include <DHT_U.h>
#include "shared_vars.h"

#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT *dht = nullptr; // global pointer declaration

unsigned long lastSensorAccess = 0;                // Stores the last timestamp when the sensor was accessed
const unsigned long SENSOR_ACCESS_INTERVAL = 2200; // 2 seconds + 10%

void configDHT()
{
    Serial.println("config DHT...");
    if (pinDht != nullptr)
    {
        Serial.println("About to convert pin to int.");
        int _pinDht = std::stoi(pinDht.c_str());
        dht = new DHT(_pinDht, DHTTYPE);
    }
    else
    {
        Serial.println("Cannot configure DHT because pin not defined.");
    }
}

float roundToHundredth(float number)
{
    return round(number * 100.0) / 100.0;
}

String floatToStringTwoDecimals(float value)
{
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    return String(buffer);
}

float readFromSensor(bool isTemperature)
{
    unsigned long currentMillis = millis();

    // This is not needed b/c the DHT.cpp has its own algorithm to prevent recurring reads too quickly
    // if (currentMillis - lastSensorAccess < SENSOR_ACCESS_INTERVAL)
    // {
    //     Serial.println("Sensor call too soon!");
    //     return NAN;
    // }

    lastSensorAccess = currentMillis; // update the last accessed timestamp

    if (dht == nullptr)
    {
        configDHT();
        if (dht == nullptr)
        {
            Serial.println("DHT not configured correctly!");
            return NAN;
        }
    }

    float value = isTemperature ? dht->readTemperature() : dht->readHumidity();

    if (isnan(value))
    {
        Serial.println("Failed to read from DHT sensor!");
        return NAN;
    }
    else
    {
        value = roundToHundredth(value);
        Serial.println(value);
        return value;
    }
}

float readDHTTemperature()
{
    Serial.println("readDHTTemperature...");
    return readFromSensor(true); // true indicates reading temperature
}

float readDHTHumidity()
{
    Serial.println("readDHTHumidity...");
    return readFromSensor(false); // false indicates reading humidity
}
