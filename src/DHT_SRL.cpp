
#include <Arduino.h>
#include <DHT_SRL.h>
#include <DHT_U.h>
#include "shared_vars.h"

#define DHTTYPE DHT22 // DHT 22 (AM2302)

// DHT dht(DHTPIN, DHTTYPE);
DHT *dht = nullptr; // global pointer declaration

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

// DHT Temperature
String readDHTTemperature()
{
    Serial.println("readDHTTemperature...");

    if (dht == nullptr)
    {
        configDHT();
        if (dht == nullptr)
        { // Double check after configDHT()
            Serial.println("DHT not configured correctly!");
            return "--";
        }
    }

    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    // Read temperature as Celsius (the default)
    float t = dht->readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    // float t = dht.readTemperature(true);
    // Check if any reads failed and exit early (to try again).
    if (isnan(t))
    {
        Serial.println("Failed to read from DHT sensor!");
        return "--";
    }
    else
    {
        Serial.println(t);
        return String(t);
    }
}

String readDHTHumidity()
{
    Serial.println("readDHTHumidity...");

    if (dht == nullptr)
    {
        configDHT();
        if (dht == nullptr)
        { // Double check after configDHT()
            Serial.println("DHT not configured correctly!");
            return "--";
        }
    }

    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht->readHumidity();
    if (isnan(h))
    {
        Serial.println("Failed to read from DHT sensor!");
        return "--";
    }
    else
    {
        Serial.println(h);
        return String(h);
    }
}
