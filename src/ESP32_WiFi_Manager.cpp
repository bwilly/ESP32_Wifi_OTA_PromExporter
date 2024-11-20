
/*********
// Brian Willy
// Climate Sensor
// Prometheus Exporter
// Copywrite 2022-2023
// Copyright 2022-2023, 2024

// Unstable Wifi post network/wifi failure
      - plan for fix here is to switch base wifi platform code away from Rui Santos to

// Only supports DHT22 Sensor

Immediate need for DSB18b20 Sensor support and multi names for multi sensor.
I guess that will be accomplished via the hard coded promexporter names.
Must be configurable.
With ability to map DSB ID to a name, such as raw water in, post air cooler, post heat exchanger, etc

// Configurable:
// Location as mDNS Name Suffix
// Wifi
// ESP Pin
*/

/*********
 *
 * Thanks to
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

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
#include "prometheus_srl.h"
#include "HtmlVarProcessor.h"
#include "TemperatureSensor.h"
#include "Config.h"

#include "version.h"

// no good reason for these to be directives
#define MDNS_DEVICE_NAME "sesp-"
#define SERVICE_NAME "climate-http"
#define SERVICE_PROTOCOL "tcp"
#define SERVICE_PORT 80
// #define LOCATION "SandBBedroom"

#define AP_REBOOT_TIMEOUT 600000 // 10 minutes in milliseconds
unsigned long apStartTime = 0;   // Variable to track the start time in AP mode

const std::string version = std::string(APP_VERSION) + "::" + APP_COMMIT_HASH + ":: esp32 : single - task - longer - wait : Nov - 2024";
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
IPAddress primaryDNS(10, 27, 1, 30); // Your Raspberry Pi's IP (DNS server)
IPAddress secondaryDNS(8, 8, 8, 8);  // Optional: Google DNS

const float THRESHOLD_TEMPERATURE_PERCENTAGE = 3.0;
const float THRESHOLD_HUMIDITY_PERCENTAGE = 4.0;
const unsigned long PUBLISH_INTERVAL = 5 * 60 * 1000; // Five minutes in milliseconds

float previousTemperature = NAN;
float previousHumidity = NAN;
unsigned long lastPublishTime_tempt = 0;
unsigned long lastPublishTime_humidity = 0;

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

// DS18b20
// Data wire is plugged into port 15 on the ESP32
#define ONE_WIRE_BUS 23

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

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;

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
  request->send(200, "text/plain", version.c_str());
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
  Serial.println("Zabbix agent server started on port 10050");
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
void populateW1Addresses(uint8_t w1Address[3][8], String w1Name[3], const SensorGroupW1 &w1Sensors)
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

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Enable verbose logging for the WiFi component
  esp_log_level_set("wifi", ESP_LOG_VERBOSE);

  Serial.println("initSpiffs...");
  initSPIFFS();

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
    if (paramMetadata.name.startsWith("w1-"))
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

  bool dhtEnabledValue = *(paramToBoolMap["enableDHT"]);
  if (dhtEnabledValue)
  {
    initSensorTask(); // dht
  }

  if (initWiFi())
  { // Station Mode
    Serial.println("initDNS...");
    initDNS();

    // Initialize Zabbix agent server
    initZabbixServer();

    Serial.println("set web root /index.html...");
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

    // copy/paste from setup section for AP -- changing URL path
    // todo: consolidate this copied code
    server.on("/manage", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor); });

    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", version.c_str()); });

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

    // server.serveStatic("/", SPIFFS, "/");

    // note: this is for the post from /manage. whereas, in the setup mode, both form and post are root
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      handlePostParameters(request); 
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(mainDelay.toInt()); // delay(3000);
      ESP.restart(); });

    // Correct the onEnd function signature
    // AsyncElegantOTA.onEnd([](bool success)
    //                       {
    // if (success) {
    //     Serial.println("OTA End - restarting device.");
    //     ESP.restart();  // Restart the ESP32 after OTA is completed.
    // } else {
    //     Serial.println("OTA failed.");
    // } });

    // Correct the onProgress function signature (optional, depending on version)
    // AsyncElegantOTA.onProgress([](unsigned int progress, unsigned int total)
    //                            { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

    // uses path like server.on("/update")
    // AsyncElegantOTA.begin(&server);
    ElegantOTA.begin(&server);

    configTime(0, 0, "pool.ntp.org"); // Set timezone offset and daylight offset to 0 for simplicity
    time_t now;
    while (!(time(&now)))
    { // Wait for time to be set
      Serial.println("Waiting for time...");
      delay(1000);
    }

    server.begin();

    // Set MQTT server
    Serial.println("Setting MQTT server and port...");
    Serial.println(*paramToVariableMap["mqtt-server"]);
    Serial.println(*paramToVariableMap["mqtt-port"]);

    if (paramToVariableMap.find("mqtt-server") != paramToVariableMap.end() &&
        paramToVariableMap.find("mqtt-port") != paramToVariableMap.end())
    {

      const char *server = paramToVariableMap["mqtt-server"]->c_str();
      int port = paramToVariableMap["mqtt-port"]->toInt();

      mqClient.setServer(server, port);
    }
    else
    {
      Serial.println("Error setting MQTT from params.");
    }
    Serial.println("Entry setup loop complete.");
  }
  else
  // SETUP
  { // This path is meant to run only upon initial setup

    apStartTime = millis(); // Record the start time in AP mode

    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("srlESP-WIFI-MANAGER", "saltmeadow"); // name and password

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    if (!SPIFFS.begin(true))
    {
      Serial.println("SPIFFS is out of scope per bwilly!");
    }
    // Web Server Root URL
    Serial.print("Setting web root path to /wifimanager.html... ");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor); });

    server.serveStatic("/", SPIFFS, "/"); // for things such as CSS

    Serial.print("Setting root POST and delegating to handlePostParameters...");
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      handlePostParameters(request);
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(3000);
      ESP.restart(); });

    Serial.print("Starting web server...");
    server.begin();
  }
}

// void publishTemperature(PubSubClient &_client, float _temperature, String location)
// {
//   const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(4);
//   DynamicJsonDocument doc(capacity);

//   JsonObject j = doc.createNestedObject();
//   j["bn"] = location;
//   j["n"] = "temperature";
//   j["u"] = "C";
//   j["v"] = _temperature;
//   j["ut"] = (int)time(nullptr);

//   char buffer[256];
//   serializeJson(doc, buffer);

//   _client.publish("ship/temperature", buffer);
// }

void publishTemperature(PubSubClient &_client, float _temperature, String location)
{
  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonDocument doc(capacity);

  doc["bn"] = location;
  doc["n"] = "temperature";
  doc["u"] = "C";
  doc["v"] = _temperature;
  doc["ut"] = (int)time(nullptr);

  char buffer[512];
  serializeJson(doc, buffer);

  Serial.print("Publishing the following to msg broker: ");
  Serial.println(buffer);

  _client.publish("ship/temperature", buffer); // todo: externalize
}

void publishHumidity(PubSubClient &_client, float _humidity, String location)
{
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(4);
  DynamicJsonDocument doc(capacity);

  JsonObject j = doc.createNestedObject();
  j["bn"] = location;
  j["n"] = "humidity";
  j["u"] = "%";
  j["v"] = _humidity;
  j["ut"] = (int)time(nullptr);

  char buffer[256];
  serializeJson(doc, buffer);

  Serial.print("Publishing the following to msg broker: ");
  Serial.println(buffer);

  _client.publish("ship/humidity", buffer); // todo: externalize
}

// Oct7, '23
// Telegraf doesn't seem to be able to parse the senml with multiple measures
/**
 * Output Message
 *
 * [{"bn":"sensor:12345"},{"n":"temperature","u":"C","v":17.60000038,"ut":1139},{"n":"humidity","u":"%","v":69.30000305,"ut":1139}]
 *
 */
// void publishTemperatureHumidity(PubSubClient &_client, float _temperature, float _humidity)
// {
//   const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(4);
//   DynamicJsonDocument doc(capacity);

//   JsonObject obj1 = doc.createNestedObject();
//   obj1["bn"] = "sensor:12345";

//   JsonObject obj2 = doc.createNestedObject();
//   obj2["n"] = "temperature";
//   obj2["u"] = "C";
//   obj2["v"] = _temperature;
//   // obj2["v"] = static_cast<float>(_temperature);
//   obj2["ut"] = (int)time(nullptr);

//   JsonObject obj3 = doc.createNestedObject();
//   obj3["n"] = "humidity";
//   obj3["u"] = "%";
//   obj3["v"] = _humidity;
//   // obj3["v"] = static_cast<float>(_humidity);
//   obj3["ut"] = (int)time(nullptr);

//   char buffer[256];
//   serializeJson(doc, buffer);

//   // _client.publish("ship/temperature", buffer);
//   // _client.publish("ship/humidity", buffer);

//   _client.publish("ship/climate", buffer);
// }

// void publishTemperatureHumidity(PubSubClient &_client, String _temperature, String _humidity)
// {
//   const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(4);
//   DynamicJsonDocument doc(capacity);

//   JsonObject obj1 = doc.createNestedObject();
//   obj1["bn"] = "sensor:12345";

//   JsonObject obj2 = doc.createNestedObject();
//   obj2["n"] = "temperature";
//   obj2["u"] = "C";
//   obj2["v"] = _temperature;
//   obj2["ut"] = (int)time(nullptr);

//   JsonObject obj3 = doc.createNestedObject();
//   obj3["n"] = "humidity";
//   obj3["u"] = "%";
//   obj3["v"] = _humidity;
//   obj3["ut"] = (int)time(nullptr);

//   char buffer[256];
//   serializeJson(doc, buffer);

//   Serial.print("Completed json serialization for queue publish. Now publishing...");
//   _client.publish("ship/temperature", buffer);
//   _client.publish("ship/humidity", buffer);
// }

void reconnectMQ()
{
  // while (!mqClient.connected() && (WiFi.getMode() == WIFI_AP))
  while (!mqClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqClient.connect(MakeMine(MDNS_DEVICE_NAME)))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqClient.state());
      Serial.println(" try again in 5 seconds");
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
      Serial.println("Message published successfully.");
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

void loop()
{
  unsigned long currentMillis = millis();

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
    publishSimpleMessage(); // manual test

    if (!mqClient.connected())
    {
      reconnectMQ();
    }
    mqClient.loop();

    if (dhtEnabled)
    {
      float currentTemperature = readDHTTemperature();
      float currentHumidity = readDHTHumidity();

      // Calculate the percentage change from the previous values
      float tempChange = calculatePercentageChange(previousTemperature, currentTemperature);
      float humidityChange = calculatePercentageChange(previousHumidity, currentHumidity);

      // Checking current time
      // unsigned long currentMillis = millis();

      unsigned long timeSinceLastPublishTempt = currentMillis - lastPublishTime_tempt;
      unsigned long timeSinceLastPublishHumidity = currentMillis - lastPublishTime_humidity;

      bool shouldPublishTempt = true;    // false;
      bool shouldPublishHumidity = true; // false;

      // Check temperature change criteria
      if (tempChange >= THRESHOLD_TEMPERATURE_PERCENTAGE)
      {
        shouldPublishTempt = true;
      }
      Serial.print("Temperature changed from ");
      Serial.print(previousTemperature);
      Serial.print(" to ");
      Serial.print(currentTemperature);
      Serial.print(". Percentage change: ");
      Serial.println(tempChange);

      // Check humidity change criteria
      if (humidityChange >= THRESHOLD_HUMIDITY_PERCENTAGE)
      {
        shouldPublishHumidity = true;
      }
      Serial.print("Humidity changed from ");
      Serial.print(previousHumidity);
      Serial.print(" to ");
      Serial.print(currentHumidity);
      Serial.print(". Percentage change: ");
      Serial.println(humidityChange);

      // Check time interval criteria for temperature
      if (timeSinceLastPublishTempt >= PUBLISH_INTERVAL)
      {
        shouldPublishTempt = true;
        Serial.println("Time interval since last temperature publish is greater than the defined threshold. Publishing...");
      }
      else
      {
        Serial.print("Time to next threshold publish for temperature: ");
        Serial.print((PUBLISH_INTERVAL - timeSinceLastPublishTempt) / 1000);
        Serial.println(" seconds.");
      }

      // Check time interval criteria for humidity
      if (timeSinceLastPublishHumidity >= PUBLISH_INTERVAL)
      {
        shouldPublishHumidity = true;
        Serial.println("Time interval since last humidity publish is greater than the defined threshold. Publishing...");
      }
      else
      {
        Serial.print("Time to next threshold publish for humidity: ");
        Serial.print((PUBLISH_INTERVAL - timeSinceLastPublishHumidity) / 1000);
        Serial.println(" seconds.");
      }

      // Publishing based on the individual checks
      if (shouldPublishTempt)
      {
        Serial.println("publishTemperature...");
        publishTemperature(mqClient, currentTemperature, locationName);

        // Update the previous values after publishing
        previousTemperature = currentTemperature;
        lastPublishTime_tempt = currentMillis;
      }
      else
      {
        Serial.println("Skipping publish temperature.");
      }

      if (shouldPublishHumidity)
      {
        Serial.println("publishHumidity...");
        publishHumidity(mqClient, currentHumidity, locationName);

        // Update the previous values after publishing
        previousHumidity = currentHumidity;
        lastPublishTime_humidity = currentMillis;
      }
      else
      {
        Serial.println("Skipping publish humidity.");
      }
      Serial.println("");
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
        publishTemperature(mqClient, readings[i].value, readings[i].name);
      }
    }
  }

  delay(mainDelay.toInt()); // Wait for 5 seconds before next loop
}
