
/*********
// Brian Willy
// Climate Sensor
// Prometheus Exporter
// Copywrite 2022-2023
// Copyright 2022-2023, 2024, 2025

// Unstable Wifi post network/wifi failure
      - plan for fix here is to switch base wifi platform code away from Rui Santos to

// Only supports DHT22 Sensor [obsolete]]

Immediate need for DSB18b20 Sensor support and multi names for multi sensor. [done]
I guess that will be accomplished via the hard coded promexporter names.
Must be configurable.
With ability to map DSB ID to a name, such as raw water in, post air cooler, post heat exchanger, etc

// Configurable:
// Location as mDNS Name Suffix
// Wifi
// ESP Pin
*/

/** some params can be set via UI. Anything params loaded later as the json web call will supersede UI params [Nov28'25] */

/*********
 *
 * Thanks to
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// todo Nov25'25
// why is volt-test trying to send dht?
// readDHTTemperature...
// ⚠️ sensor mutex not initialized in readDHTTemperature!

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

#include "ConfigLoad.h"
#include "ConfigFetch.h"

#include "esp_log.h"
#include <WiFiMulti.h>
#include "ESPmDNS.h"

// #include <DHT_U.h>
// #include <dht.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// #include <AsyncElegantOTA.h>
#include <ElegantOTA.h>

#include <string>

#include <iostream>
#include <sstream>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "shared_vars.h"
#include "ParamHandler.h"
#include "SpiffsHandler.h"
#include "DHT_SRL.h"
#include "ACS712_SRL.h"
#include "prometheus_srl.h"
#include "HtmlVarProcessor.h"
#include "TemperatureSensor.h"
#include "Config.h"
#include "MessagePublisher.h"
#include "ConfigDump.h"
#include "OtaUpdate.h"
#include "CHT832xSensor.h"
#include "SCT_SRL.h"

#include "version.h"
// #include <TelnetStream.h>

// #include <AsyncTelnetSerial.h>
#include <AsyncTCP.h> // dependency for the async streams

#include <BufferedLogger.h>
#include "TelnetBridge.h"

// extern AsyncTelnetSerial telnetSerial;
// static AsyncTelnetSerial telnetSerial(&Serial);

// no good reason for these to be directives
#define MDNS_DEVICE_NAME "sesp-"
#define SERVICE_NAME "climate-http"
#define SERVICE_PROTOCOL "tcp"
#define SERVICE_PORT 80
// #define LOCATION "SandBBedroom"

#define AP_REBOOT_TIMEOUT 600000 // 10 minutes in milliseconds
unsigned long apStartTime = 0;   // Variable to track the start time in AP mode

volatile bool g_otaRequested = false;
String g_otaUrl;

// Remote config JSON merge docs (kept off stack)
static const size_t REMOTE_JSON_CAPACITY = 4096;
static StaticJsonDocument<REMOTE_JSON_CAPACITY> g_remoteMergedDoc;
static StaticJsonDocument<REMOTE_JSON_CAPACITY> g_remoteTmpDoc;

// BEFORE (something like this)
// const char* version = APP_VERSION "::" APP_COMMIT_HASH ":: TelnetBridge";

// AFTER
// String version = String(APP_VERSION) + "::" + APP_COMMIT_HASH + ":: TelnetBridge-removed";
String version = String(APP_VERSION) + "::" +
                 APP_COMMIT_HASH + "::" +
                 APP_BUILD_DATE + ":: TelnetBridge-removed";

// trying to identify cause of unreliable dht22 readings

// Serial.println("Application Version: " APP_VERSION);
// Serial.println("Commit Hash: " APP_COMMIT_HASH);

// MQTT Server details
// const char *mqtt_server = "192.168.68.120"; // todo: change to config param
// const int mqtt_port = 1883;                 // todo: change to config param

WiFiClient espClient;
PubSubClient mqClient(espClient);
WiFiMulti wifiMulti;

// DNS settings
IPAddress primaryDNS(10, 27, 1, 30); // Your Raspberry Pi's IP (DNS server) mar'25: why is this here? is it doing anything
IPAddress secondaryDNS(8, 8, 8, 8);  // Optional: Google DNS

const float THRESHOLD_TEMPERATURE_PERCENTAGE = 3.0;
const float THRESHOLD_HUMIDITY_PERCENTAGE = 4.0;
const unsigned long PUBLISH_INTERVAL = 5 * 60 * 1000; // Five minutes in milliseconds

float previousTemperature = NAN;
float previousHumidity = NAN;
unsigned long lastPublishTime_tempt = 0;
unsigned long lastPublishTime_humidity = 0;

// For CHT832x
float previousCHTTemperature = NAN;
float previousCHTHumidity = NAN;
unsigned long lastPublishTime_temptCHT = 0;
unsigned long lastPublishTime_humidityCHT = 0;

// Local pump config (Nov 25, 2025)
namespace
{
  constexpr float PUMP_ON_THRESHOLD_AMPS = 1.75f; // was hardcoded in loop
}

// Globals to store the last published values
// float lastPublishedTemperature = NAN;
// float lastPublishedHumidity = NAN;

// #define DHTPIN 23 // Digital pin connected to the DHT sensor
// const int DHTPIN;
// TODO: allow pin and sensor type to be configurable

// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
// #define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

// DHT dht(DHTPIN, DHTTYPE);
// DHT *dht = nullptr; // global pointer declaration

// CHT832x I2C temperature/humidity sensor (full OO)
CHT832xSensor envSensor(0x44); // default address; todo: externalize Dec3'25

// SctSensor sctSensor(32, 15.0f);  // default 15A;
// choose the ADC1 pin you're using (example)
static const int PIN_SCT = 32;
// choose the rating (10, 15, or 20A version)
static const float SCT_RATED_AMPS = 15.0f;
// your OO instance (just like envSensor)
SctSensor sctSensor(PIN_SCT, SCT_RATED_AMPS);

// DS18b20
// Data wire is plugged into port 15 on the ESP32
#define ONE_WIRE_BUS 23 // todo: externalize Nov20-24

// // Setup a oneWire instance to communicate with any OneWire devices
// OneWire oneWire(ONE_WIRE_BUS);

// // Pass our oneWire reference to Dallas Temperature.
// DallasTemperature sensors(&oneWire);

OneWire oneWire(ONE_WIRE_BUS); // todo:externalize I/O port nov'24
// Create a TemperatureSensor instance
TemperatureSensor temptSensor(&oneWire); // Dallas

// uint8_t w1[3][8] = {
//     {0x28, 0xa0, 0x7b, 0x49, 0xf6, 0xde, 0x3c, 0xe9},
//     {0x28, 0x08, 0xd3, 0x49, 0xf6, 0x3c, 0x3c, 0xfd},
//     {0x28, 0xc5, 0xe1, 0x49, 0xf6, 0x50, 0x3c, 0x38}};

// #define MAX_READINGS 4

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// New Zabbix agent server on port 10050
AsyncWebServer zabbixServer(10050);

// IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
// IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
// IPAddress subnet(255, 255, 0, 0);

// Timer variables
// todo: can combine interval and delay. each is from copy/paste. one solves initial connect, the other is for lost connection retry
unsigned long previousMillis = 0;
const long interval = 40000; // interval to wait for Wi-Fi connection (milliseconds)
unsigned long previous_time = 0;
unsigned long reconnect_delay = 180000; // 3-min delay

static bool lastPumpState = false; // Assume OFF at startup
static bool firstRun = true;       // New flag to force first publish

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;

// Forward declarations for setup refactor
void setupSpiffsAndConfig();
void setupStationMode();
void setupAccessPointMode();

void onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success)
  {
    Serial.println("OTA update finished successfully! Restarting...");
    ESP.restart();
  }
  else
  {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}
void onOTAStart()
{
  // Log when OTA has started
  Serial.println("OTA update started!");
}

/* Append a semi-unique id to the name template */
char *MakeMine(const char *NameTemplate)
{
  // uint16_t uChipId = GetDeviceId();
  // String Result = String(NameTemplate) + String(uChipId, HEX);
  String Result = String(NameTemplate) + String(locationName);

  char *tab2 = new char[Result.length() + 1];
  strcpy(tab2, Result.c_str());

  return tab2;
}

// Function to handle Zabbix agent.ping
void handleZabbixPing(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", "1");
}

// Function to handle Zabbix agent.version
void handleZabbixVersion(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", version);
}

// Function to handle system.uptime
void handleSystemUptime(AsyncWebServerRequest *request)
{
  unsigned long uptimeMillis = millis();
  unsigned long uptimeSeconds = uptimeMillis / 1000;
  request->send(200, "text/plain", String(uptimeSeconds));
}

// // Function to handle vm.memory.size[available]
// void handleAvailableMemory(AsyncWebServerRequest *request)
// {
//   // Placeholder value for available memory
//   // Implement actual memory reading logic if possible
//   int availableMemory = 1024; // example value in bytes
//   request->send(200, "text/plain", String(availableMemory));
// }

// Function to handle vm.memory.size[available]
void handleAvailableMemory(AsyncWebServerRequest *request)
{
  // Get available memory
  int availableMemory = ESP.getFreeHeap(); // Get free heap memory in bytes
  request->send(200, "text/plain", String(availableMemory));
}

// Function to handle agent.hostname
void handleHostName(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", MakeMine(MDNS_DEVICE_NAME));
}

// Function to handle zabbix.agent.availability
void handleAvailability(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", "1");
}

void handleWiFiBSSID(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", WiFi.BSSIDstr());
}

void handleWiFiSignalStrength(AsyncWebServerRequest *request)
{
  String rssiString = String(WiFi.RSSI());
  const char *rssiCharArray = rssiString.c_str();
  request->send(200, "text/plain", rssiCharArray);
}

static bool readFileToString(const char *path, String &out)
{
  File f = SPIFFS.open(path, FILE_READ);
  if (!f)
  {
    return false;
  }
  out = f.readString();
  f.close();
  return true;
}

static bool writeStringToFile(const char *path, const String &data)
{
  File f = SPIFFS.open(path, FILE_WRITE); // truncates or creates
  if (!f)
  {
    Serial.print(F("writeStringToFile: failed to open "));
    Serial.print(path);
    Serial.println(F(" for writing"));
    return false;
  }

  size_t written = f.print(data);
  f.close();

  if (written != data.length())
  {
    Serial.print(F("writeStringToFile: short write to "));
    Serial.print(path);
    Serial.print(F(" (expected "));
    Serial.print(data.length());
    Serial.print(F(" bytes, wrote "));
    Serial.print(written);
    Serial.println(F(")"));
    return false;
  }

  return true;
}

void tryFetchAndApplyRemoteConfig()
{
  const String baseUrl = configUrl;

  if (baseUrl.length() == 0)
  {
    logger.log("ConfigFetch: base configUrl is empty; cannot fetch remote config\n");
    return;
  }

  // Build GLOBAL URL: <base>/global.json
  String globalUrl = baseUrl;
  if (!globalUrl.endsWith("/"))
  {
    globalUrl += "/";
  }
  globalUrl += "global.json"; // <-- make sure this says "global.json", not "global."

  // Build INSTANCE URL: <base>/<locationName>.json
  String instanceUrl;
  if (locationName.length() > 0)
  {
    instanceUrl = baseUrl;
    if (!instanceUrl.endsWith("/"))
    {
      instanceUrl += "/";
    }
    instanceUrl += locationName + ".json";
  }
  else
  {
    logger.log("ConfigFetch: locationName is empty; will only attempt GLOBAL config\n");
  }

  // Load previous remote snapshot
  String previousRemoteJson;
  bool hasPreviousRemote = readFileToString("/config-remote.json", previousRemoteJson);
  if (hasPreviousRemote)
  {
    logger.log("ConfigFetch: found previous remote snapshot /config-remote.json\n");
  }
  else
  {
    logger.log("ConfigFetch: no previous /config-remote.json (first run or missing)\n");
  }

  // Clear and prepare merged remote doc
  g_remoteMergedDoc.clear();
  JsonObject mergedRoot = g_remoteMergedDoc.to<JsonObject>();

  bool anyRemoteApplied = false;
  String json;

  auto fetchApplyAndMerge = [&](const String &url, const char *label)
  {
    if (url.length() == 0)
    {
      return;
    }

    if (!downloadConfigJson(url, json))
    {
      logger.log(String("ConfigFetch: ") + label +
                 " config fetch FAILED or not found at " + url + "\n");
      return;
    }

    logger.log(String("ConfigFetch: downloaded ") + label + " config from " + url +
               " (" + String(json.length()) + " bytes)\n");

    // Apply to in-memory params
    if (!loadConfigFromJsonString(json))
    {
      logger.log(String("ConfigFetch: ") + label + " JSON parse/apply FAILED\n");
      return;
    }

    logger.log(String("ConfigFetch: ") + label + " JSON applied OK\n");

    // Merge into remote snapshot doc
    g_remoteTmpDoc.clear();
    DeserializationError err = deserializeJson(g_remoteTmpDoc, json);
    if (err)
    {
      logger.log(String("ConfigFetch: ") + label +
                 " JSON re-parse FAILED for snapshot merge\n");
      return;
    }

    JsonObject src = g_remoteTmpDoc.as<JsonObject>();
    for (JsonPair kv : src)
    {
      mergedRoot[kv.key()] = kv.value(); // instance overrides global on same key
    }

    anyRemoteApplied = true;
  };

  // GLOBAL then INSTANCE
  fetchApplyAndMerge(globalUrl, "GLOBAL");
  if (instanceUrl.length() > 0)
  {
    fetchApplyAndMerge(instanceUrl, "INSTANCE");
  }

  if (!anyRemoteApplied)
  {
    logger.log("ConfigFetch: no remote configs applied; leaving local config as-is; no reboot\n");
    return;
  }

  // Serialize new remote snapshot
  String newRemoteJson;
  serializeJson(g_remoteMergedDoc, newRemoteJson);

  // Compare snapshots *only on remote config*
  if (hasPreviousRemote && (newRemoteJson == previousRemoteJson))
  {
    logger.log("ConfigFetch: remote config unchanged vs /config-remote.json; no persist, no reboot\n");
    return;
  }

  logger.log("ConfigFetch: remote config changed; persisting and rebooting\n");

  // Persist remote snapshot
  if (writeStringToFile("/config-remote.json", newRemoteJson))
  {
    logger.log("ConfigFetch: wrote new remote snapshot to /config-remote.json\n");
  }
  else
  {
    logger.log("ConfigFetch: FAILED to write /config-remote.json\n");
  }

  // Persist full effective config
  if (saveEffectiveCacheToFile(EFFECTIVE_CACHE_PATH))
  {
    logger.log("ConfigFetch: persisted merged effective config EFFECTIVE_CACHE_PATH; rebooting\n");
  }
  else
  {
    logger.log("ConfigFetch: FAILED to persist merged config to EFFECTIVE_CACHE_PATH; rebooting anyway\n");
  }

  ESP.restart();
}

// Function to initialize the Zabbix agent server
void initZabbixServer()
{
  zabbixServer.on("/", HTTP_GET, handleZabbixPing);
  zabbixServer.on("/agent.version", HTTP_GET, handleZabbixVersion);
  zabbixServer.on("/system.uptime", HTTP_GET, handleSystemUptime);
  zabbixServer.on("/vm.memory.size[available]", HTTP_GET, handleAvailableMemory);
  zabbixServer.on("/hostname", HTTP_GET, handleHostName);
  zabbixServer.on("/bssid", HTTP_GET, handleWiFiBSSID);
  zabbixServer.on("/signal", HTTP_GET, handleWiFiSignalStrength);
  zabbixServer.on("/availability", HTTP_GET, handleAvailability);
  zabbixServer.begin();
  logger.log("Zabbix agent server started on port 10050\n");
}

String printAddressAsString(DeviceAddress deviceAddress)
{
  String addressString = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    addressString += "0x";
    if (deviceAddress[i] < 0x10)
      addressString += "0";
    addressString += String(deviceAddress[i], HEX);
    if (i < 7)
      addressString += ", ";
  }
  addressString += "\n";
  return addressString;
}

// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");

  loadPersistedValues();
}

bool initWiFi()
{
  if (ssid == "")
  {
    Serial.println("Undefined SSID.");
    return false;
  }

  Serial.println("Setting WiFi to WIFI_STA...");

  WiFi.disconnect(true);
  // delay(180);
  // Set custom hostname
  if (!WiFi.setHostname(MakeMine(MDNS_DEVICE_NAME)))
  {
    Serial.println("Error setting hostname");
  }
  else
  {
    Serial.print("Setting DNS hostname to: ");
    Serial.println(MakeMine(MDNS_DEVICE_NAME));
  }
  WiFi.mode(WIFI_STA);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);

  // WiFi.begin(ssid.c_str(), pass.c_str());

  int n = WiFi.scanNetworks();
  Serial.println("Scan complete");
  for (int i = 0; i < n; ++i)
  {
    Serial.print("SSID: ");
    Serial.print(WiFi.SSID(i));
    Serial.print(", RSSI: ");
    Serial.println(WiFi.RSSI(i));
  }

  // Add Wi-Fi networks to WiFiMulti
  wifiMulti.addAP(ssid.c_str(), pass.c_str());

  Serial.println("Connecting to Wi-Fi; looking for the strongest mesh node...");

  // Start the connection attempt
  unsigned long startAttemptTime = millis();
  while (wifiMulti.run() != WL_CONNECTED)
  {
    // Additional diagnostics
    Serial.print("Last SSID attempted: ");
    Serial.println(WiFi.SSID());
    Serial.print("Last BSSID attempted: ");
    Serial.println(WiFi.BSSIDstr());
    Serial.print("Signal strength (RSSI): ");
    Serial.println(WiFi.RSSI());
    Serial.print("Connection status: ");
    Serial.println(WiFi.status());

    unsigned long currentMillis = millis();
    if (currentMillis - startAttemptTime >= interval)
    {
      Serial.println("Failed to connect after interval timeout.");

      // Note: WiFi.reasonCode() can provide additional reason if available
      // if (WiFi.status() != WL_CONNECTED)
      // {
      //   Serial.println("WiFi did not connect.");
      // }

      return false;
    }
    Serial.print(".");
    // WiFi.disconnect(true);
    delay(500); // Shorter delay to show progress

    // WiFi.reconnect();
  }

  // Successful connection
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Connected to BSSID: ");
  Serial.println(WiFi.BSSIDstr());

  return true;
}

void AdvertiseServices()
{
  Serial.println("AdvertiseServices on mDNS...");
  char *MyName = MakeMine(MDNS_DEVICE_NAME);
  if (MDNS.begin(MyName))
  {
    Serial.println(F("mDNS responder started"));
    Serial.print(F("I am: "));
    Serial.println(MyName);

    // Add service to MDNS-SD
    MDNS.addService(SERVICE_NAME, SERVICE_PROTOCOL, SERVICE_PORT);
  }
  else
  {
    while (1)
    {
      Serial.println(F("Error setting up MDNS responder"));
      delay(1000);
    }
  }
}

bool initDNS()
{
  // if (!MDNS.begin(MDNS_DEVICE_NAME))
  // {
  //   Serial.println("Error starting mDNS");
  //   return false;
  // }

  AdvertiseServices();
  return true;
}

// Replaces placeholder with DHT values
// String processor(const String& var){
//   //Serial.println(var);
//   if(var == "TEMPERATURE"){
//     return readDHTTemperature();
//   }
//   else if(var == "HUMIDITY"){
//     return readDHTHumidity();
//   }
//   return String();
// }

// variable to hold device addresses
DeviceAddress Thermometer;

int deviceCount = 0;

String printDS18b20(void)
{
  String output = "";

  // sensors.begin();
  temptSensor.sensors.begin();

  output += "Locating devices...\n";
  output += "Found ";
  deviceCount = temptSensor.sensors.getDeviceCount();
  output += String(deviceCount, DEC);
  output += " devices.\n\n";
  output += "Printing addresses...\n";

  for (int i = 0; i < deviceCount; i++)
  {
    output += "Sensor ";
    output += String(i + 1);
    output += " : ";
    temptSensor.sensors.getAddress(Thermometer, i);
    // Assuming printAddress() returns a formatted string
    output += printAddressAsString(Thermometer);
  }

  return output;
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7)
      Serial.print(", ");
  }
  Serial.println("");
}

void setupDS18b20(void)
{
  // start serial port
  // Serial.begin(115200);

  // Start up the library
  // sensors.begin();
  temptSensor.sensors.begin();

  // locate devices on the bus
  Serial.println("Locating devices...");
  Serial.print("Found ");
  deviceCount = temptSensor.sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

  Serial.println("Printing addresses...");
  for (int i = 0; i < deviceCount; i++)
  {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" : ");
    temptSensor.sensors.getAddress(Thermometer, i);
    printAddress(Thermometer);
  }

  // Output
  // Printing addresses...
  // Sensor 1 : 0x28, 0xA0, 0x7B, 0x49, 0xF6, 0xDE, 0x3C, 0xE9
  // Sensor 2 : 0x28, 0x08, 0xD3, 0x49, 0xF6, 0x3C, 0x3C, 0xFD
  // Sensor 3 : 0x28, 0xC5, 0xE1, 0x49, 0xF6, 0x50, 0x3C, 0x38
}

// todo:extract to another file
// Define function to populate w1Address and w1Name from w1Sensors
void populateW1Addresses(uint8_t w1Address[6][8], String w1Name[6], const SensorGroupW1 &w1Sensors)
{
  for (size_t i = 0; i < w1Sensors.sensors.size(); ++i)
  {
    // Populate w1Name
    w1Name[i] = String(w1Sensors.sensors[i].name.c_str()); // Convert std::string to Arduino String

    // Populate w1Address
    for (size_t j = 0; j < w1Sensors.sensors[i].HEX_ARRAY.size(); ++j)
    {
      w1Address[i][j] = w1Sensors.sensors[i].HEX_ARRAY[j];
    }
  }
}

// Entry point
// Entry point
void setup()
{
  // Serial port for debugging purposes
  delay(500);
  Serial.begin(115200);
  delay(500);

  // Enable verbose logging for the WiFi component
  esp_log_level_set("wifi", ESP_LOG_VERBOSE);

  // SPIFFS, legacy params, /config.json, W1 sensors, etc.
  setupSpiffsAndConfig();

  // Decide which path: Station vs AP
  if (initWiFi()) // Station Mode
  {
    setupStationMode();
  }
  else
  {
    setupAccessPointMode();
  }
}

void setupSpiffsAndConfig()
{
  Serial.println("initSpiffs...");
  initSPIFFS();

  // Try to load a flat JSON config from /config.json.
  // If present and valid, it will override the legacy per-param SPIFFS values
  // that loadPersistedValues() just populated.
  if (loadEffectiveCacheFromFile(EFFECTIVE_CACHE_PATH))
  {
    Serial.println("ConfigLoad: EFFECTIVE_CACHE_PATH = '/config.effective.cache.json'loaded and applied (overrides per-param SPIFFS files).");
  }
  else
  {
    Serial.println("ConfigLoad: no EFFECTIVE_CACHE_PATH = '/config.effective.cache.json' (or parse error); using values from loadPersistedValues() which are the params from UI.");
  }

  // i am commenting out below b/c i don't believe it has purpose (bwilly)
  // Set GPIO 2 as an OUTPUT
  // Serial.println("ledPin mode and digitalWrite...");
  // pinMode(ledPin, OUTPUT);
  // digitalWrite(ledPin, LOW);

  // const char *w1Paths[3] = {w1_1Path.c_str(), w1_2Path.c_str(), w1_3Path.c_str()};
  // for (int i = 0; i < 3; i++)
  // {
  //   loadW1AddressFromFile(SPIFFS, w1Paths[i], i);
  // }

  // w1Name[0] = readFile(SPIFFS, w1_1_name_Path.c_str());
  // w1Name[1] = readFile(SPIFFS, w1_2_name_Path.c_str());
  // w1Name[2] = readFile(SPIFFS, w1_3_name_Path.c_str());

  // Load parameters from SPIFFS using paramList
  // replaces commented out code directly above
  for (const auto &paramMetadata : paramList)
  {
    if (paramMetadata.name.startsWith(PARAM_W1_1_NAME)) // just match the first w1 as the delegated loaders will load them all
    {
      // loadW1SensorConfigFromFile(SPIFFS, paramMetadata.spiffsPath.c_str(), w1Sensors.sensors);
      loadW1SensorConfigFromFile(SPIFFS, "/w1Json", w1Sensors); // moved away from coupleing file name of storage to teh paramMetaData. maybe i should use it. nov'24

      // populate the arrays that TemperatureSensor consumes
      populateW1Addresses(w1Address, w1Name, w1Sensors);
    }
    // Add else if blocks here for loading other specific parameter types if needed
  }

  // ip = readFile(SPIFFS, ipPath);
  // gateway = readFile(SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(locationName);
  // Serial.println(ip);
  // Serial.println(gateway);
}

void setupStationMode()
{
  // setup: path1 (Station Mode)

  tryFetchAndApplyRemoteConfig();

  // Start Telnet stream
  // TelnetStream.begin();            // start Telnet server on port 23
  // initTelnet();

  // (B) Start async Telnet-to-Serial forwarding:
  //     - 115200 is the baudrate to mirror (must match Serial.begin)
  //     - true  = link Telnet <-> Serial
  //     - false = don’t publish mDNS name

  // telnetSerial.begin(115200, true, false);

  // start our async Telnet server
  initTelnetServer();
  logger.logf("Boot: IP=%s\n", WiFi.localIP().toString().c_str());

  logger.log("initDNS...\n");
  initDNS();

  // Initialize Zabbix agent server
  initZabbixServer();

  // @pattern
  bool dhtEnabledValue = *(paramToBoolMap["enableDHT"]);
  if (dhtEnabledValue)
  {
    initSensorTask(); // dht
  }
  if (acs712Enabled)
  {
    setupACS712();
  }

  // I2C pins for CHT832x
  Wire.begin(32, 33); // why is this here instead of inside the instantiation of the CHT sensor? Dec6'25
  if (cht832xEnabled)
  {
    envSensor.begin();
  }

  
  if(sctEnabled) {
    sctSensor.begin();
  }

  logger.log("set web root /index.html...\n");
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html", false, processor); });
  server.serveStatic("/", SPIFFS, "/");

  // Route to set GPIO state to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      digitalWrite(ledPin, HIGH);
      request->send(SPIFFS, "/index.html", "text/html", false, processor); });

  // Route to set GPIO state to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      digitalWrite(ledPin, LOW);
      request->send(SPIFFS, "/index.html", "text/html", false, processor); });

  // Route to Prometheus Metrics Exporter
  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", readAndGeneratePrometheusExport(locationName.c_str())); });

  server.on("/devicename", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", MakeMine(MDNS_DEVICE_NAME)); });

  server.on("/bssid", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", WiFi.BSSIDstr()); });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                  char buffer[32];  // Buffer to hold the string representation of the temperature
                  float temperature = readDHTTemperature();
                  snprintf(buffer, sizeof(buffer), "%.2f", temperature);
                  request->send(200, "text/html", buffer); });

  server.on("/cht/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    char buffer[32];

    float chtTemp = NAN;
    float chtHum  = NAN;

    if (envSensor.read(chtTemp, chtHum)) {
        snprintf(buffer, sizeof(buffer), "%.2f", chtTemp);
        request->send(200, "text/html", buffer);
    } else {
        request->send(500, "text/plain", "CHT read failed");
    } });

  server.on("/cht/humidity", HTTP_GET, [](AsyncWebServerRequest *request)
{
    char buffer[32];

    float chtTemp = NAN;
    float chtHum  = NAN;

    if (envSensor.read(chtTemp, chtHum)) {
        snprintf(buffer, sizeof(buffer), "%.2f", chtHum);
        request->send(200, "text/html", buffer);
    } else {
        request->send(500, "text/plain", "CHT read failed");
    }
});


  // copy/paste from setup section for AP -- changing URL path
  // todo: consolidate this copied code
  server.on("/manage", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor); });

  server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", version); });

  server.on("/pins", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", pinDht); });

  server.on("/onewire", HTTP_GET, [](AsyncWebServerRequest *request)
            {           
                String result = printDS18b20();                     
                request->send(200, "text/html", result); });

  // todo: find out why some readings provide 129 now, and on prev commit, they returned -127 for same bad reading. Now, the method below return -127, but this one is now 129. Odd. Aug19 '23
  server.on("/onewiretempt", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              temptSensor.requestTemperatures();
              TemperatureReading *readings = temptSensor.getTemperatureReadings();

              // Use the readings to send a response, assume SendHTML can handle TemperatureReading array
              request->send(200, "text/html", SendHTML(readings, MAX_READINGS));

              // sensors.requestTemperatures();
              // request->send(200, "text/html", SendHTML(sensors.getTempC(w1Address[0]), sensors.getTempC(w1Address[1]), sensors.getTempC(w1Address[2]))); });
              // request->send(200, "text/html", SendHTMLxxx());
            });

  // todo: find out why some readings provide -127
  server.on("/onewiremetrics", HTTP_GET, [](AsyncWebServerRequest *request)
            {           
                // sensors.requestTemperatures();
                temptSensor.requestTemperatures();
                TemperatureReading *readings = temptSensor.getTemperatureReadings();

                // TemperatureReading readings[MAX_READINGS] = {
                //     {w1Name[0].c_str(), sensors.getTempC(w1Address[0])},
                //     {w1Name[1].c_str(), sensors.getTempC(w1Address[1])},
                //     {w1Name[2].c_str(), sensors.getTempC(w1Address[2])},
                //     {} // Ending marker
                // };

                request->send(200, "text/html", buildPrometheusMultiTemptExport(readings)); });


  // View the locally stored working config
  server.on("/config/effective-cache", HTTP_GET, [](AsyncWebServerRequest *request) {
    const char *path = EFFECTIVE_CACHE_PATH;

    if (!SPIFFS.exists(path)) {
        request->send(404, "text/plain", "No effective cache file stored");
        return;   
    }

    request->send(SPIFFS, path, "application/json");
});

  // View the last downloaded remote snapshot: /config-remote.json
  server.on("/config/remote", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      const char *path = "/config-remote.json";

      if (!SPIFFS.exists(path)) {
          request->send(404, "text/plain", "No /config-remote.json stored");
          return;
      }

      request->send(SPIFFS, path, "application/json"); });

  server.on("/ota/run", HTTP_GET, [](AsyncWebServerRequest *request)
            {

      auto it = paramToVariableMap.find("ota-url");
      if (it == paramToVariableMap.end() || it->second == nullptr) {
          request->send(400, "text/plain",
                        "Missing or null 'ota-url' param in config");
          return;
      }

      String fwUrl = *(it->second);
      fwUrl.trim();

      if (fwUrl.length() == 0) {
          request->send(400, "text/plain",
                        "Empty 'ota-url' value in config");
          return;
      }

      logger.log("OTA: requested via /ota/run, URL = " + fwUrl + "\n");

      // schedule OTA (run in loop() to avoid async_tcp WDT)
      g_otaUrl = fwUrl;
      g_otaRequested = true;

      request->send(200, "text/plain",
                    "OTA scheduled from " + fwUrl +
                    "\nDevice will reboot if update succeeds."); });

  server.on("/config/cache/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool ok = clearConfigJsonCache(SPIFFS);
    if (ok) {
        request->send(200, "text/plain", "Config JSON cache cleared. It will not be used until remote config repopulates it.");
    } else {
        request->send(500, "text/plain", "Failed to clear config JSON cache.");
    }
});

server.on("/device/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    delay(300);   // allow response to flush
    ESP.restart();
});


  
                    // note: this is for the post from /manage. whereas, in the setup mode, both form and post are root
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      handlePostParameters(request);
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(mainDelay.toInt()); // delay(3000);
      ESP.restart(); });

  // uses path like server.on("/update")
  // AsyncElegantOTA.begin(&server);
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  // ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  configTime(0, 0, "pool.ntp.org"); // Set timezone offset and daylight offset to 0 for simplicity
  time_t now;
  while (!(time(&now)))
  { // Wait for time to be set
    logger.log("Waiting for time...\n");
    delay(1000);
  }

  server.begin();

  // Set MQTT server
  logger.log("Setting MQTT server and port...\n");
  logger.log(*paramToVariableMap["mqtt-server"]);
  logger.log(*paramToVariableMap["mqtt-port"]);
  logger.log("\n");

  if (paramToVariableMap.find("mqtt-server") != paramToVariableMap.end() &&
      paramToVariableMap.find("mqtt-port") != paramToVariableMap.end())
  {

    const char *serverName = paramToVariableMap["mqtt-server"]->c_str();
    int port = paramToVariableMap["mqtt-port"]->toInt();

    mqClient.setServer(serverName, port);
  }
  else
  {
    logger.log("Error setting MQTT from params.\n");
  }
  logger.log("\nEntry setup loop complete.");
}

void setupAccessPointMode()
{
  // SETUP : Path2
  // This path is meant to run only upon initial one-time setup

  apStartTime = millis(); // Record the start time in AP mode

  Serial.println("Setting AP (Access Point)");

  // Build base SSID (without prefix)
  String base;

  if (locationName.length() > 0)
  {
    // Use configured location name
    base = locationName;
  }
  else
  {
    // Build fallback with unique suffix from MAC
    uint64_t mac = ESP.getEfuseMac();
    uint32_t suffix = (uint32_t)(mac & 0xFFFFFF); // last 3 bytes

    char buf[32];
    snprintf(buf, sizeof(buf), "%06X", suffix);
    base = buf;
  }

  // Final SSID: ALWAYS prefixed with "sesp-"
  String apSsid = "srl-sesp-" + base;

  // Guarantee SSID ≤ 31 chars (Wi-Fi limit)
  if (apSsid.length() > 31)
  {
    apSsid = apSsid.substring(0, 31);
  }

  WiFi.softAP(apSsid.c_str(), "saltmeadow");

  Serial.print("AP SSID: ");
  Serial.println(apSsid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS is out of scope per bwilly!");
  }
  // Web Server Root URL
  Serial.print("Setting web root path to /wifimanager.html...\n");

    server.on("/config/cache/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool ok = clearConfigJsonCache(SPIFFS);
    if (ok) {
        request->send(200, "text/plain", "Config JSON cache cleared. It will not be used until remote config repopulates it.");
    } else {
        request->send(500, "text/plain", "Failed to clear config JSON cache.");
    }
});

server.on("/device/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    delay(300);   // allow response to flush
    ESP.restart();
});


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor); });

  server.serveStatic("/", SPIFFS, "/"); // for things such as CSS

  Serial.print("Setting root POST and delegating to handlePostParameters...\n");
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      handlePostParameters(request);
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(3000);
      logger.log("Updated. Now restarting...\n");
      ESP.restart(); });

  logger.log("Starting web server...\n");
  server.begin();
}

void reconnectMQ()
{
  // while (!mqClient.connected() && (WiFi.getMode() == WIFI_AP))
  while (!mqClient.connected())
  {
    logger.log("Attempting MQTT connection...");
    if (mqClient.connect(MakeMine(MDNS_DEVICE_NAME)))
    {
      logger.log("connected\n");
    }
    else
    {
      logger.log("failed, rc=");
      logger.log(mqClient.state());
      logger.log(" try again in 5 seconds\n");
      delay(5000);
    }
  }
}

void publishSimpleMessage()
{
  // Define the message and topic
  const char *topic = "test/topic";
  const char *message = "Hello World!";

  // Publish the message
  if (mqClient.connected())
  {
    if (mqClient.publish(topic, message))
    {
      Serial.print("Message published successfully: ");
      Serial.println(message);
    }
    else
    {
      Serial.println("Message publishing failed.");
    }
  }
  else
  {
    Serial.println("Not connected to MQTT broker.");
    Serial.println(mqClient.state());

    reconnectMQ();
  }
}

// Helper function to calculate percentage change
float calculatePercentageChange(float oldValue, float newValue)
{
  if (isnan(oldValue) || oldValue == 0.0f)
  {
    return 100.0f; // Return 100% if old value is NaN or 0 to force an update
  }
  return abs((newValue - oldValue) / oldValue) * 100.0f;
}

void maybePublishEnvToMqtt(
    const char *sourceName,
    float currentTemperature,
    float currentHumidity,
    float &previousTemperatureRef,
    float &previousHumidityRef,
    unsigned long &lastPublishTimeTempRef,
    unsigned long &lastPublishTimeHumRef)
{
  unsigned long now = millis();

  float tempChange = calculatePercentageChange(previousTemperatureRef, currentTemperature);
  float humidityChange = calculatePercentageChange(previousHumidityRef, currentHumidity);

  unsigned long timeSinceLastPublishTemp = now - lastPublishTimeTempRef;
  unsigned long timeSinceLastPublishHum = now - lastPublishTimeHumRef;

  bool shouldPublishTemp = false;
  bool shouldPublishHum = false;

  if (tempChange >= THRESHOLD_TEMPERATURE_PERCENTAGE)
  {
    shouldPublishTemp = true;
  }
  if (humidityChange >= THRESHOLD_HUMIDITY_PERCENTAGE)
  {
    shouldPublishHum = true;
  }

  logger.log(sourceName);
  logger.log(" Temperature changed from ");
  logger.log(previousTemperatureRef);
  logger.log(" to ");
  logger.log(currentTemperature);
  logger.log(". Percentage change: ");
  logger.log(tempChange);
  logger.log("\n");

  logger.log(sourceName);
  logger.log(" Humidity changed from ");
  logger.log(previousHumidityRef);
  logger.log(" to ");
  logger.log(currentHumidity);
  logger.log(". Percentage change: ");
  logger.log(humidityChange);
  logger.log("\n");

  if (timeSinceLastPublishTemp >= PUBLISH_INTERVAL)
  {
    shouldPublishTemp = true;
    logger.log(sourceName);
    logger.log(": time interval since last temp publish exceeded; publishing.\n");
  }
  else
  {
    logger.log(sourceName);
    logger.log(": time to next temp publish: ");
    logger.log((PUBLISH_INTERVAL - timeSinceLastPublishTemp) / 1000);
    logger.log(" seconds.\n");
  }

  if (timeSinceLastPublishHum >= PUBLISH_INTERVAL)
  {
    shouldPublishHum = true;
    logger.log(sourceName);
    logger.log(": time interval since last humidity publish exceeded; publishing.\n");
  }
  else
  {
    logger.log(sourceName);
    logger.log(": time to next humidity publish: ");
    logger.log((PUBLISH_INTERVAL - timeSinceLastPublishHum) / 1000);
    logger.log(" seconds.\n");
  }

  if (shouldPublishTemp)
  {
    logger.log(sourceName);
    logger.log(": publishTemperature...\n");
    MessagePublisher::publishTemperature(mqClient, currentTemperature, locationName);
    previousTemperatureRef = currentTemperature;
    lastPublishTimeTempRef = now;
  }
  else
  {
    logger.log(sourceName);
    logger.log(": skipping publish temperature.\n");
  }

  if (shouldPublishHum)
  {
    logger.log(sourceName);
    logger.log(": publishHumidity...\n");
    MessagePublisher::publishHumidity(mqClient, currentHumidity, locationName);
    previousHumidityRef = currentHumidity;
    lastPublishTimeHumRef = now;
  }
  else
  {
    logger.log(sourceName);
    logger.log(": skipping publish humidity.\n");
  }

  logger.log("\n");
}

void loop()
{
  unsigned long currentMillis = millis();

  // Defered OTA execution from main loop (not async_tcp task)
  if (g_otaRequested)
  {
    g_otaRequested = false; // clear first to avoid reentry if OTA fails

    logger.log("OTA: executing deferred OTA from loop using URL = " + g_otaUrl + "\n");

    bool ok = performHttpOta(g_otaUrl);
    if (!ok)
    {
      logger.log("OTA: performHttpOta() failed; no reboot will occur\n");
    }

    // If performHttpOta succeeds, it calls ESP.restart() and we never reach here.
  }

  // Check if we are in AP mode and if so, if it's time to reboot
  if (WiFi.getMode() == WIFI_AP) // && WiFi.softAPIP().toString() == "192.168.4.1")
  {
    if (currentMillis - apStartTime >= AP_REBOOT_TIMEOUT)
    {
      Serial.println("Rebooting due to extended time in AP mode...");
      ESP.restart();
    }
  }

  // does this need to be here? I didn't use it, here, on the new master project. maybe it was only needed for the non-async web server? bwilly Feb26'23
  // server.handleClient();

  unsigned long current_time = millis(); // number of milliseconds since the upload

  // checking for WIFI connection
  if ((WiFi.status() != WL_CONNECTED) && (current_time - previous_time >= reconnect_delay))
  {
    Serial.print("millis: ");
    Serial.println(millis());
    Serial.print("previous_time: ");
    Serial.println(previous_time);
    Serial.print("current_time: ");
    Serial.println(current_time);
    Serial.print("reconnect_delay: ");
    Serial.println(reconnect_delay);
    Serial.println("Reconnecting to WIFI network by restarting to leverage best AP algorithm");
    // WiFi.disconnect();
    // WiFi.reconnect();
    ESP.restart();
    previous_time = current_time;
  }

  if (mqttEnabled)
  {

    reconnectMQ();
    // publishSimpleMessage(); // manual test

    if (!mqClient.connected())
    {
      reconnectMQ();
    }
    mqClient.loop();

    // @anti-pattern as compared to dhtEnabledValue? Maybe this is the bettr way and the dhtEnabledValue was mean for checkbox population? Mar4'25
    if (dhtEnabled)
    {
      float currentTemperature = readDHTTemperature();
      float currentHumidity = readDHTHumidity();

      maybePublishEnvToMqtt(
          "DHT",
          currentTemperature,
          currentHumidity,
          previousTemperature,
          previousHumidity,
          lastPublishTime_tempt,
          lastPublishTime_humidity);
    }

    // Dec3'25
    if (cht832xEnabled)
    {
      float chtTemp = NAN;
      float chtHum = NAN;

      if (envSensor.read(chtTemp, chtHum))
      {
        maybePublishEnvToMqtt(
            "CHT832x",
            chtTemp,
            chtHum,
            previousCHTTemperature,
            previousCHTHumidity,
            lastPublishTime_temptCHT,
            lastPublishTime_humidityCHT);
      }
      else
      {
        logger.log("CHT832xSensor: read failed; skipping publish this cycle\n");
      }
    } else {
      logger.log("CHT832xSensor not enabled. \n");
    }

    if(sctEnabled) {
          float amps = sctSensor.readCurrentACRms();
          logger.logf("iot.sct.current %.3fA pin=%d rated=%.0f\n",
            amps, PIN_SCT, SCT_RATED_AMPS);

          
    }
  }

  if (w1Enabled)
  {
    temptSensor.requestTemperatures();
    TemperatureReading *readings = temptSensor.getTemperatureReadings(); // todo:performance: move declaration outside of the esp loop

    for (int i = 0; i < MAX_READINGS; i++)
    {
      // Check if the reading is valid, e.g., by checking if the name is not empty
      if (!readings[i].name.isEmpty())
      {
        MessagePublisher::publishTemperature(mqClient, readings[i].value, readings[i].name);
      }
    }
  }

  if (acs712Enabled)
  {
    float amps = fabs(readACS712Current());
    logger.log(amps);
    logger.log(" amps\n");

    bool pumpState = (amps > PUMP_ON_THRESHOLD_AMPS);

    if (pumpState)
    {
      logger.log("Pump ON\n");
    }
    else
    {
      logger.log("Pump OFF\n");
    }

    if (firstRun || pumpState != lastPumpState || pumpState)
    {
      // Only publish if the pump state changed ...i don't like hiding the visibility of being OFF, but too much data
      MessagePublisher::publishPumpState(mqClient, pumpState, amps, "engineRoomPump");
      lastPumpState = pumpState; // Update the last known state
      firstRun = false;
    }
  }

  logger.log("loop finished. sleep/delay...\n");
  // logger.flushTelnet();
  //  TelnetStream.println("loop finished. sleep/delay...");

  // (1) Read-and-parse the configured delay (in ms)
  int requestedDelay = mainDelay.toInt();

  // (2) Enforce a minimum of 500 ms—if below, log + bump up to 3000 ms
  int actualDelay = requestedDelay < 500
                        ? 3000
                        : requestedDelay;
  if (requestedDelay < 500)
  {
    logger.logf(
        "Requested delay (%d ms) < 500 ms; overriding to %d ms\n",
        requestedDelay, actualDelay);
  }

  delay(actualDelay); // Wait for 5 seconds before next loop
}