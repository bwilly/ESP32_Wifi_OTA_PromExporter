
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

#include <DHT_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <AsyncElegantOTA.h>

#include <string>

#include <iostream>
#include <sstream>

// no good reason for these to be directives
#define MDNS_DEVICE_NAME "esp32-climate-sensor-"
#define SERVICE_NAME "climate-http"
#define SERVICE_PROTOCOL "tcp"
#define SERVICE_PORT 80
// #define LOCATION "SandBBedroom"

// #define DHTPIN 23 // Digital pin connected to the DHT sensor
// const int DHTPIN;
// TODO: allow pin and sensor type to be configurable

// Building for first use by multiple DS18B20 sensors
struct TemperatureReading
{
  std::string location;
  float temperature;
};

// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

// DHT dht(DHTPIN, DHTTYPE);
DHT *dht = nullptr; // global pointer declaration

// DS18b20
// Data wire is plugged into port 15 on the ESP32
#define ONE_WIRE_BUS 23

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

uint8_t tempSensor1, tempSensor2, tempSensor3;
uint8_t sensor1[8] = {0x28, 0xa0, 0x7b, 0x49, 0xf6, 0xde, 0x3c, 0xe9};
uint8_t sensor2[8] = {0x28, 0x08, 0xd3, 0x49, 0xf6, 0x3c, 0x3c, 0xfd};
uint8_t sensor3[8] = {0x28, 0xc5, 0xe1, 0x49, 0xf6, 0x50, 0x3c, 0x38};
#define MAX_READINGS 4

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "location";
const char *PARAM_INPUT_4 = "pinDht";
// const char *PARAM_INPUT_3 = "ip";
// const char *PARAM_INPUT_4 = "gateway";

// Variables to save values from HTML form
String ssid;
String pass;
String locationName; // used during regular operation, not only setup
String pinDht;
// String ip;
// String gateway;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *locationNamePath = "/location.txt";
const char *pinDhtPath = "/pindht.txt";
// const char *ipPath = "/ip.txt";
// const char *gatewayPath = "/gateway.txt";

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

// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- frite failed");
  }
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

  // Copied from Dec'22 working at saltmeadow to connect to strongest signal of mesh network
  WiFi.mode(WIFI_STA);
  // Add list of wifi networks
  WiFiMulti wifiMulti;
  wifiMulti.addAP(ssid.c_str(), pass.c_str());
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

// Prometheus Exporter

char buffer[1024];

// *****************
//  Prometheus Exporter Format Dec9-2022
// *****************
// https://github.com/KireinaHoro/esp32-sensor-exporter/blob/master/src/main.cpp
//
// void send_metrics(WiFiClient &client) {
char *readAndGeneratePrometheusExport(const char *location)
{
  memset(buffer, 0, 1);

  strncat(buffer, "# HELP environ_tempt Environment temperature (in C).\n", 60);
  strncat(buffer, "# TYPE environ_tempt gauge\n", 30);

  strncat(buffer, "environ_tempt{location=\"", 50);
  strncat(buffer, location, 20);
  strncat(buffer, "\"}", 3);

  strncat(buffer, " ", 2);
  strncat(buffer, readDHTTemperature().c_str(), 6);
  strncat(buffer, "\n", 2);

  strncat(buffer, "# HELP environ_humidity Environment relative humidity (in percentage).\n", 99);
  strncat(buffer, "# TYPE environ_humidity gauge\n", 32);

  strncat(buffer, "environ_humidity{location=\"", 50);
  strncat(buffer, location, 20);
  strncat(buffer, "\"}", 3);

  strncat(buffer, " ", 2);
  strncat(buffer, readDHTHumidity().c_str(), 6);
  strncat(buffer, "\n", 3);

  return buffer;
}

char *buildPrometheusMultiTemptExport(TemperatureReading readings[MAX_READINGS])
{
  static char buffer[1024];
  memset(buffer, 0, sizeof(buffer));

  strncat(buffer, "# HELP environ_tempt Environment temperature (in C).\n", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, "# TYPE environ_tempt gauge\n", sizeof(buffer) - strlen(buffer) - 1);

  for (int i = 0; i < MAX_READINGS; i++)
  {
    if (readings[i].location.empty())
      break;

    strncat(buffer, "environ_tempt{location=\"", sizeof(buffer) - strlen(buffer) - 1);
    strncat(buffer, readings[i].location.c_str(), sizeof(buffer) - strlen(buffer) - 1);
    strncat(buffer, "\"} ", sizeof(buffer) - strlen(buffer) - 1);

    char temperatureStr[8];
    snprintf(temperatureStr, sizeof(temperatureStr), "%.2f", readings[i].temperature);
    strncat(buffer, temperatureStr, sizeof(buffer) - strlen(buffer) - 1);
    strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
  }

  return buffer;
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
String processor(const String &var)
{
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
  else if (var == "SSID")
  {
    String SSID = readFile(SPIFFS, ssidPath);
    return SSID;
  }
  else if (var == "PASS")
  {
    String PASS = readFile(SPIFFS, passPath);
    return PASS;
  }
  else if (var == "LOCATION")
  {
    String LOCATION = readFile(SPIFFS, locationNamePath);
    return LOCATION;
  }
  else if (var == "PIN")
  {
    String PIN = readFile(SPIFFS, pinDhtPath);
    return PIN;
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

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  initSPIFFS();

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  locationName = readFile(SPIFFS, locationNamePath);
  pinDht = readFile(SPIFFS, pinDhtPath);
  // ip = readFile(SPIFFS, ipPath);
  // gateway = readFile(SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(locationName);
  Serial.println(pinDht);
  // Serial.println(ip);
  // Serial.println(gateway);

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

  if (initWiFi())
  { // Station Mode

    initDNS();

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
              { request->send(200, "text/html", "ds18b20-Alpha1"); });

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
                tempSensor1 = sensors.getTempC(sensor1); // Gets the values of the temperature
                tempSensor2 = sensors.getTempC(sensor2); // Gets the values of the temperature
                tempSensor3 = sensors.getTempC(sensor3); // Gets the values of the temperature          
                request->send(200, "text/html", SendHTML(tempSensor1, tempSensor2, tempSensor3)); });
    // request->send(200, "text/html", SendHTMLxxx()); });

    // todo: find out why some readings provide -127
    server.on("/onewiremetrics", HTTP_GET, [](AsyncWebServerRequest *request)
              {           
                sensors.requestTemperatures();

                TemperatureReading readings[MAX_READINGS] = {
                    {"RawWaterInput", sensors.getTempC(sensor1)},
                    {"RawWaterExit", sensors.getTempC(sensor2)},
                    {"EngineCoolantReturn", sensors.getTempC(sensor3)},
                    {} // Ending marker
                };

                request->send(200, "text/html", buildPrometheusMultiTemptExport(readings)); });

    server.serveStatic("/", SPIFFS, "/");

    // note: this is for the post from /manage. whereas, in the setup mode, both form and post are root /
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST location value
          if (p->name() == PARAM_INPUT_3) {
            locationName = p->value().c_str();
            Serial.print("Location (for mDNS Hostname and Prometheus): ");
            Serial.println(locationName);
            // Write file to save value
            writeFile(SPIFFS, locationNamePath, locationName.c_str());
          }
          // HTTP POST location value
          if (p->name() == PARAM_INPUT_4) {
            pinDht = p->value().c_str();
            Serial.print("Pin (for DHT-22): ");
            Serial.println(pinDht);
            // Write file to save value
            writeFile(SPIFFS, pinDhtPath, pinDht.c_str());
          }
        }
      }
      // request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(3000);
      ESP.restart(); });

    // END copy/paste

    // uses path like server.on("/update")
    AsyncElegantOTA.begin(&server);

    server.begin();
  }
  else // SETUP
  {    // This path is meant to run only upon initial setup

    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", "saltmeadow");

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

    // Load default example sample ESP toggle page (bwilly comment)
    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST location value
          if (p->name() == PARAM_INPUT_3) {
            locationName = p->value().c_str();
            Serial.print("Location (for mDNS Hostname and Prometheus): ");
            Serial.println(locationName);
            // Write file to save value
            writeFile(SPIFFS, locationNamePath, locationName.c_str());
          }
          // HTTP POST location value
          if (p->name() == PARAM_INPUT_4) {
            pinDht = p->value().c_str();
            Serial.print("Pin (for DHT-22): ");
            Serial.println(pinDht);
            // Write file to save value
            writeFile(SPIFFS, pinDhtPath, pinDht.c_str());
          }
        }
      }
      // request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
      delay(3000);
      ESP.restart(); });
    server.begin();
  }
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
}

// DHT Temperature
String readDHTTemperature()
{
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
