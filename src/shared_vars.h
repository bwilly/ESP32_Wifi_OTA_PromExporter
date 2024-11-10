// In shared_vars.h
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
extern const char *PARAM_ENABLE_MQTT; //= "enableMQTT";

// extern String ssidPath;
// extern String passPath;
// extern String locationNamePath;
// extern String pinDhtPath;
// extern String mqttServerPath;
// extern String mqttPortPath;
// extern String mainDelayPath;
// extern String w1_1Path;
// extern String w1_2Path;
// extern String w1_3Path;
// extern String w1_1_name_Path;
// extern String w1_2_name_Path;
// extern String w1_3_name_Path;
// extern String enableW1Path;
// extern String enableDHTPath;
// extern String enableMQTTPath;

// Variables to save values from HTML form
extern String ssid;
extern String pass;
extern String locationName; // used during regular operation, not only setup
extern String pinDht;
extern String mqttServer;
extern String mqttPort;
extern bool w1Enabled;
extern bool dhtEnabled;
extern bool mqttEnabled;

extern String mainDelay;
extern uint8_t w1Address[3][8]; // todo:remove post refactor
extern String w1Name[3];        // todo:remove post refactor

struct W1Sensor
{
    std::string name;
    std::array<uint8_t, 8> HEX_ARRAY;
};

struct SensorGroupW1
{
    std::array<W1Sensor, 3> sensors;
};

extern SensorGroupW1 w1Sensors;

// extern uint8_t w1Sensor[3][8];
// Building for first use by multiple DS18B20 sensors
// struct TemperatureReading
// {
//     String location; // or std::string location;
//     float temperature;
// };

// // In ParamMetadata.h
// struct ParamMetadata
// {
//     String name;
//     String spiffsPath;
//     enum Type
//     {
//         STRING,
//         NUMBER,
//         BOOLEAN
//     } type;
// };
extern std::vector<ParamMetadata> paramList;
extern std::map<String, String *> paramToVariableMap;
extern std::map<String, bool *> paramToBoolMap;
