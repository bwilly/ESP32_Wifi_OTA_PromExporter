// In shared_vars.h
#ifndef SHARED_VARS_H
#define SHARED_VARS_H

#pragma once
#include <Arduino.h>
#include <vector> // Include the vector header
#include <map>
#include "ParamMetadata.h"
using namespace std;

// Search for parameter in HTTP POST request
extern const char *PARAM_WIFI_SSID; // = "ssid";
extern const char *PARAM_WIFI_PASS; // = "pass";
extern const char *PARAM_LOCATION;  // = "location";
extern const char *PARAM_PIN_DHT;   // = "pinDht";
extern const char *PARAM_PIN_ACS;   // = "pinAcs";
extern const char *PARAM_MQTT_SERVER;
extern const char *PARAM_MQTT_PORT;
extern const char *PARAM_MAIN_DELAY;
extern const int W1_NUM_BYTES;
extern const char *PARAM_W1_1;
extern const char *PARAM_W1_2;
extern const char *PARAM_W1_3;
extern const char *PARAM_W1_1_NAME;
extern const char *PARAM_W1_2_NAME;
extern const char *PARAM_W1_3_NAME;
extern const char *PARAM_ENABLE_W1;   //= "enableW1";
extern const char *PARAM_ENABLE_DHT;  //= "enableDHT";
extern const char *PARAM_ENABLE_ACS;  //= "enableDHT";
extern const char *PARAM_ENABLE_MQTT; //= "enableMQTT";


// Variables to save values from HTML form
extern String ssid;
extern String pass;
extern String locationName; // used during regular operation, not only setup
extern String pinDht;
extern String pinAcs;
extern String mqttServer;
extern String mqttPort;
extern bool w1Enabled;
extern bool dhtEnabled;
extern bool mqttEnabled;
extern bool acs712Enabled;

extern String mainDelay;
extern uint8_t w1Address[3][8]; // todo:remove post refactor
extern String w1Name[3];        // todo:remove post refactor

struct W1Sensor
{
    std::string name;
    std::array<uint8_t, 8> HEX_ARRAY = {};
};

struct SensorGroupW1
{
    std::array<W1Sensor, 3> sensors;
};

extern SensorGroupW1 w1Sensors;


extern std::vector<ParamMetadata> paramList;
extern std::map<String, String *> paramToVariableMap;
extern std::map<String, bool *> paramToBoolMap;
#endif // SHARED_VARS_H