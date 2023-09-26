#include <Arduino.h>
#include "shared_vars.h"

// Search for parameter in HTTP POST request
// This value must match the name or ID of the HTML input control and the process() value for displaying the value in the manage page
const char *PARAM_WIFI_SSID = "ssid";
const char *PARAM_WIFI_PASS = "pass";
const char *PARAM_LOCATION = "location";
const char *PARAM_PIN_DHT = "pinDht"; // wish i could change name convention. see above comment
const char *PARAM_MQTT_SERVER = "mqtt-server";
const char *PARAM_MQTT_PORT = "mqtt-port";
const char *PARAM_MAIN_DELAY = "main-delay";
const char *PARAM_W1_1 = "w1-1";
const char *PARAM_W1_2 = "w1-2";
const char *PARAM_W1_3 = "w1-3";
const char *PARAM_W1_1_NAME = "w1-1-name";
const char *PARAM_W1_2_NAME = "w1-2-name";
const char *PARAM_W1_3_NAME = "w1-3-name";
const char *PARAM_ENABLE_W1 = "enableW1";
const char *PARAM_ENABLE_DHT = "enableDHT";
const char *PARAM_ENABLE_MQTT = "enableMQTT";

// Variables to save values from HTML form
String ssid;
String pass;
String locationName; // used during regular operation, not only setup
String pinDht;
String mqttServer;
String mqttPort;
bool w1Enabled;
bool dhtEnabled;
bool mqttEnabled;
String mainDelay;
uint8_t w1Address[3][8];
String w1Name[3];
// uint8_t w1Sensors[];
