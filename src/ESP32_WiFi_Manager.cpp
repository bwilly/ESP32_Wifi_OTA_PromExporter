
/*********
// Brian Willy
// Climate Sensor
// Prometheus Exporter
// Copywrite 2022-2023
// Copyright 2022-2023

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

#include <WiFiMulti.h>
#include "ESPmDNS.h"

// #include <DHT_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <AsyncElegantOTA.h>

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

// no good reason for these to be directives
#define MDNS_DEVICE_NAME "esp32-climate-sensor-"
#define SERVICE_NAME "climate-http"
#define SERVICE_PROTOCOL "tcp"
#define SERVICE_PORT 80
// #define LOCATION "SandBBedroom"

#define version "multiSensorSelection"

// MQTT Server details
const char *mqtt_server = "pi4-2.local"; // todo: change to config param
const int mqtt_port = 1883;              // todo: change to config param

WiFiClient espClient;
PubSubClient client(espClient);

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

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// uint8_t w1[3][8] = {
//     {0x28, 0xa0, 0x7b, 0x49, 0xf6, 0xde, 0x3c, 0xe9},
//     {0x28, 0x08, 0xd3, 0x49, 0xf6, 0x3c, 0x3c, 0xfd},
//     {0x28, 0xc5, 0xe1, 0x49, 0xf6, 0x50, 0x3c, 0x38}};

// #define MAX_READINGS 4

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

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
const char *PARAM_ENABLE_W1 = "enableW1";
const char *PARAM_ENABLE_DHT = "enableDHT";
const char *PARAM_ENABLE_MQTT = "enableMQTT";

String makePath(const char *param)
{
  return String("/") + param + ".txt";
}

// For SPIFFS
String ssidPath = makePath(PARAM_WIFI_SSID);
String passPath = makePath(PARAM_WIFI_PASS);
String locationNamePath = makePath(PARAM_LOCATION);
String pinDhtPath = makePath(PARAM_PIN_DHT);
String mqttServerPath = makePath(PARAM_MQTT_SERVER);
String mqttPortPath = makePath(PARAM_MQTT_PORT);
String mainDelayPath = makePath(PARAM_MAIN_DELAY);
String w1_1Path = makePath(PARAM_W1_1);
String w1_2Path = makePath(PARAM_W1_2);
String w1_3Path = makePath(PARAM_W1_3);
String enableW1Path = makePath(PARAM_ENABLE_W1);
String enableDHTPath = makePath(PARAM_ENABLE_DHT);
String enableMQTTPath = makePath(PARAM_ENABLE_MQTT);

// IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
// IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
// IPAddress subnet(255, 255, 0, 0);

// Timer variables
// todo: can combine interval and delay. each is from copy/paste. one solves initial connect, the other is for lost connection retry
unsigned long previousMillis = 0;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)
unsigned long previous_time = 0;
unsigned long reconnect_delay = 180000; // 3-min delay

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;

// PoC method.
String SendHTML(float tempSensor1, float tempSensor2, float tempSensor3)
{
  Serial.println(tempSensor1);

  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP32 Temperature Monitor</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP32 Temperature Monitor</h1>\n";
  ptr += "<p>Living Room: ";
  ptr += tempSensor1;
  ptr += "&deg;C</p>";
  ptr += "<p>Bedroom: ";
  ptr += tempSensor2;
  ptr += "&deg;C</p>";
  ptr += "<p>Kitchen: ";
  ptr += tempSensor3;
  ptr += "&deg;C</p>";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
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
}

// Initialize WiFi
bool initWiFi()
{
  // if (ssid == "" || ip == "")
  if (ssid == "")
  {
    // Serial.println("Undefined SSID or IP address.");
    Serial.println("Undefined SSID.");
    return false;
  }

  Serial.println("Setting WiFi to WIFI_STA...");
  // Copied from Dec'22 working at saltmeadow to connect to strongest signal of mesh network
  WiFi.mode(WIFI_STA);
  // Add list of wifi networks
  WiFiMulti wifiMulti;
  wifiMulti.addAP(ssid.c_str(), pass.c_str());
  Serial.println("WiFi scanNetworks...");
  WiFi.scanNetworks();
  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi; looking for strongest mesh node...");
  if (wifiMulti.run() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Connected to BSSID: ");
    Serial.println(WiFi.BSSIDstr());
  }

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      Serial.println("Failed to connect.");
      return false;
    }
  }
  return true;
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
// Replaces placeholder with LED state value
// Only used for printing info
String processor(const String &var)
{
  Serial.println("process template token: ");
  Serial.println(var);

  if (var == "STATE") // this is from the example sample
  {
    if (digitalRead(ledPin))
    {
      ledState = "ON";
    }
    else
    {
      ledState = "OFF";
    }
    return ledState;
  }
  else if (var == "ssid")
  {
    return readFile(SPIFFS, ssidPath.c_str());
  }
  else if (var == "pass")
  {
    return readFile(SPIFFS, passPath.c_str());
  }
  else if (var == "location")
  {
    return readFile(SPIFFS, locationNamePath.c_str());
  }
  else if (var == "pinDht")
  {
    return readFile(SPIFFS, pinDhtPath.c_str());
  }
  else if (var == "mqtt-server")
  {
    return readFile(SPIFFS, mqttServerPath.c_str());
  }
  else if (var == "mqtt-port")
  {
    return readFile(SPIFFS, mqttPortPath.c_str());
  }
  else if (var == "main-delay")
  {
    return readFile(SPIFFS, mainDelayPath.c_str());
  }
  else if (var == "w1-1")
  {
    return readFile(SPIFFS, w1_1Path.c_str());
  }
  else if (var == "w1-2")
  {
    return readFile(SPIFFS, w1_2Path.c_str());
  }
  else if (var == "w1-3")
  {
    return readFile(SPIFFS, w1_3Path.c_str());
  }
  else if (var == "enableW1")
  {
    return readFile(SPIFFS, enableW1Path.c_str());
  }
  else if (var == "enableDHT")
  {
    return readFile(SPIFFS, enableDHTPath.c_str());
  }
  else if (var == "enableMQTT")
  {
    return readFile(SPIFFS, enableMQTTPath.c_str());
  }
  else if (var == "enableW1_checked")
  {
    String fileValue = readFile(SPIFFS, enableW1Path.c_str());
    if (fileValue == "true")
    {
      return "checked";
    }
    else if (fileValue == "false" || fileValue == "")
    {
      return "";
    }
  }
  else if (var == "enableW1_checked")
  {
    String fileValue = readFile(SPIFFS, enableW1Path.c_str());
    if (fileValue == "true")
    {
      return "checked";
    }
    else if (fileValue == "false" || fileValue == "")
    {
      return "";
    }
  }
  else if (var == "enableDHT_checked")
  {
    String fileValue = readFile(SPIFFS, enableDHTPath.c_str());
    if (fileValue == "true")
    {
      return "checked";
    }
    else if (fileValue == "false" || fileValue == "")
    {
      return "";
    }
  }
  else if (var == "enableMQTT_checked")
  {
    String fileValue = readFile(SPIFFS, enableMQTTPath.c_str());
    if (fileValue == "true")
    {
      return "checked";
    }
    else if (fileValue == "false" || fileValue == "")
    {
      return "";
    }
  }
  else
    return String();

  // ssid = readFile(SPIFFS, ssidPath);
  // pass = readFile(SPIFFS, passPath);
  // locationName = readFile(SPIFFS, locationNamePath);
  // pinDht = readFile(SPIFFS, pinDhtPath);
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

  sensors.begin();

  output += "Locating devices...\n";
  output += "Found ";
  deviceCount = sensors.getDeviceCount();
  output += String(deviceCount, DEC);
  output += " devices.\n\n";
  output += "Printing addresses...\n";

  for (int i = 0; i < deviceCount; i++)
  {
    output += "Sensor ";
    output += String(i + 1);
    output += " : ";
    sensors.getAddress(Thermometer, i);
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
  sensors.begin();

  // locate devices on the bus
  Serial.println("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

  Serial.println("Printing addresses...");
  for (int i = 0; i < deviceCount; i++)
  {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" : ");
    sensors.getAddress(Thermometer, i);
    printAddress(Thermometer);
  }

  // Output
  // Printing addresses...
  // Sensor 1 : 0x28, 0xA0, 0x7B, 0x49, 0xF6, 0xDE, 0x3C, 0xE9
  // Sensor 2 : 0x28, 0x08, 0xD3, 0x49, 0xF6, 0x3C, 0x3C, 0xFD
  // Sensor 3 : 0x28, 0xC5, 0xE1, 0x49, 0xF6, 0x50, 0x3C, 0x38
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  Serial.println("initSpiffs...");
  initSPIFFS();

  // i am commenting out below b/c i don't believe it has purpose (bwilly)
  // Set GPIO 2 as an OUTPUT
  // Serial.println("ledPin mode and digitalWrite...");
  // pinMode(ledPin, OUTPUT);
  // digitalWrite(ledPin, LOW);

  // Load values saved in SPIFFS
  Serial.println("loading SPIFFS values...");
  ssid = readFile(SPIFFS, ssidPath.c_str());
  pass = readFile(SPIFFS, passPath.c_str());
  locationName = readFile(SPIFFS, locationNamePath.c_str());
  pinDht = readFile(SPIFFS, pinDhtPath.c_str());
  mqttServer = readFile(SPIFFS, mqttServerPath.c_str());
  mqttPort = readFile(SPIFFS, mqttPortPath.c_str());
  mainDelay = readFile(SPIFFS, mainDelayPath.c_str()).toInt();
  bool w1Enabled = (readFile(SPIFFS, enableW1Path.c_str()) == "true");
  bool dhtEnabled = (readFile(SPIFFS, enableDHTPath.c_str()) == "true");
  bool mqttEnabled = (readFile(SPIFFS, enableMQTTPath.c_str()) == "true");

  const char *w1Paths[3] = {w1_1Path.c_str(), w1_2Path.c_str(), w1_3Path.c_str()};
  for (int i = 0; i < 3; i++)
  {
    loadW1AddressFromFile(SPIFFS, w1Paths[i], i);
  }

  // ip = readFile(SPIFFS, ipPath);
  // gateway = readFile(SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(locationName);
  Serial.println(pinDht);
  // Serial.println(ip);
  // Serial.println(gateway);

  if (initWiFi())
  { // Station Mode
    Serial.println("initDNS...");
    initDNS();

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
              { request->send(200, "text/html", readDHTTemperature().c_str()); });

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
                sensors.requestTemperatures();                
                request->send(200, "text/html", SendHTML(sensors.getTempC(w1Address[0]), sensors.getTempC(w1Address[1]), sensors.getTempC(w1Address[2]))); });
    // request->send(200, "text/html", SendHTMLxxx()); });

    // todo: find out why some readings provide -127
    server.on("/onewiremetrics", HTTP_GET, [](AsyncWebServerRequest *request)
              {           
                sensors.requestTemperatures();

                TemperatureReading readings[MAX_READINGS] = {
                    {"RawWaterInput", sensors.getTempC(w1Address[0])},
                    {"RawWaterExit", sensors.getTempC(w1Address[1])},
                    {"EngineCoolantReturn", sensors.getTempC(w1Address[2])},
                    {} // Ending marker
                };

                request->send(200, "text/html", buildPrometheusMultiTemptExport(readings)); });

    // server.serveStatic("/", SPIFFS, "/");

    // note: this is for the post from /manage. whereas, in the setup mode, both form and post are root
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      handlePostParameters(request); 
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(mainDelay.toInt()); // delay(3000);
      ESP.restart(); });

    // uses path like server.on("/update")
    AsyncElegantOTA.begin(&server);

    server.begin();

    // todo: removeMe: reinstate
    // Set MQTT server
    // Serial.println("Setting MQTT server...");
    // client.setServer(mqtt_server, mqtt_port);

    Serial.println("Entry setup loop complete.");
  }
  else // SETUP
  {    // This path is meant to run only upon initial setup

    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", "saltmeadow"); // name and password

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
              { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

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

// probably working this one to convert to prometheus output
String SendHTMLxxx(void)
{

  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP32 Temperature Monitor</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP32 Temperature Monitor</h1>\n";
  ptr += "<p>Living Room: ";

  ptr += "&deg;C</p>";
  ptr += "<p>Bedroom: ";

  ptr += "&deg;C</p>";
  ptr += "<p>Kitchen: ";

  ptr += "&deg;C</p>";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void publishTemperatureHumidity(PubSubClient _client, float _temperature, float _humidity)
{
  const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(4);
  DynamicJsonDocument doc(capacity);

  JsonObject obj1 = doc.createNestedObject();
  obj1["bn"] = "sensor:12345";

  JsonObject obj2 = doc.createNestedObject();
  obj2["n"] = "temperature";
  obj2["u"] = "C";
  obj2["v"] = _temperature;
  obj2["ut"] = (int)time(nullptr);

  JsonObject obj3 = doc.createNestedObject();
  obj3["n"] = "humidity";
  obj3["u"] = "%";
  obj3["v"] = _humidity;
  obj3["ut"] = (int)time(nullptr);

  char buffer[256];
  serializeJson(doc, buffer);
  _client.publish("ship/temperature", buffer);
  _client.publish("ship/humidity", buffer);
}

void loop()
{
  // does this need to be here? I didn't use it, here, on the new master project. maybe it was only needed for the non-async web server? bwilly Feb26'23
  // server.handleClient();

  unsigned long current_time = millis(); // number of milliseconds since the upload

  // checking for WIFI connection
  if ((WiFi.status() != WL_CONNECTED) && (current_time - previous_time >= reconnect_delay))
  {
    Serial.print(millis());
    Serial.println("Reconnecting to WIFI network by restarting to leverage best AP algorithm");
    // WiFi.disconnect();
    // WiFi.reconnect();
    ESP.restart();
    previous_time = current_time;
  }

  if (mqttEnabled)
  {
    Serial.println("publishTemperatureHumidity then sleep...");
    publishTemperatureHumidity(client, readDHTTemperature().toFloat(), readDHTHumidity().toFloat());
  }
  delay(mainDelay.toInt()); // Wait for 5 seconds before next loop
}
