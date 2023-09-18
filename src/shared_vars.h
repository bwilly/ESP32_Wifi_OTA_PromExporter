#pragma once
#include <Arduino.h>

// Search for parameter in HTTP POST request
extern const char *PARAM_WIFI_SSID; // = "ssid";
extern const char *PARAM_WIFI_PASS; // = "pass";
extern const char *PARAM_LOCATION;  // = "location";
extern const char *PARAM_PIN_DHT;   // = "pinDht";

extern String ssidPath;
extern String passPath;
extern String locationNamePath;
extern String pinDhtPath;

// Variables to save values from HTML form
extern String ssid;
extern String pass;
extern String locationName; // used during regular operation, not only setup
extern String pinDht;

// Building for first use by multiple DS18B20 sensors
struct TemperatureReading
{
    String location; // or std::string location;
    float temperature;
};