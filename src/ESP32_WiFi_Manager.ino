/*********
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

#include <AsyncElegantOTA.h>

#include <string>

// no good reason for these to be directives
#define MDNS_DEVICE_NAME "esp32-climate-sensor-"
#define SERVICE_NAME "climate-http"
#define SERVICE_PROTOCOL "tcp"
#define SERVICE_PORT 80
// #define LOCATION "SandBBedroom"

// #define DHTPIN 23 // Digital pin connected to the DHT sensor
// const int DHTPIN;
// TODO: allow pin and sensor type to be configurable

// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

// DHT dht(DHTPIN, DHTTYPE);
DHT *dht = nullptr; // global pointer declaration

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
  if (var == "STATE")
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
  return String();
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
              { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "preAlpha2"); });

    server.on("/pins", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", pinDht); });

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
