#include <Arduino.h>
#include "shared_vars.h"

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
// uint8_t w1Sensors[];