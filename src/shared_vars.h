#pragma once
#include <Arduino.h>

// File paths to save input values permanently
extern const char *ssidPath;
extern const char *passPath;
extern const char *locationNamePath;
extern const char *pinDhtPath;

// Search for parameter in HTTP POST request
extern const char *PARAM_INPUT_1; // = "ssid";
extern const char *PARAM_INPUT_2; // = "pass";
extern const char *PARAM_INPUT_3; // = "location";
extern const char *PARAM_INPUT_4; // = "pinDht";

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