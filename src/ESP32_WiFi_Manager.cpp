
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
#include "HtmlVarProcessor.h"

// no good reason for these to be directives
#define MDNS_DEVICE_NAME "esp32-climate-sensor-"
#define SERVICE_NAME "climate-http"
#define SERVICE_PROTOCOL "tcp"
#define SERVICE_PORT 80
// #define LOCATION "SandBBedroom"

#define version "multiSensorSelection"

// MQTT Server details
const char *mqtt_server = "10.27.1.135"; // todo: change to config param
const int mqtt_port = 1883;              // todo: change to config param

WiFiClient espClient;
PubSubClient mqClient(espClient);

const float PERCENTAGE_THRESHOLD = 1.0;               // Threshold set to 1%
const unsigned long PUBLISH_INTERVAL = 5 * 60 * 1000; // Five minutes in milliseconds

float previousTemperature = NAN;
float previousHumidity = NAN;
unsigned long lastPublishTime = 0;

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

  const char *w1Paths[3] = {w1_1Path.c_str(), w1_2Path.c_str(), w1_3Path.c_str()};
  for (int i = 0; i < 3; i++)
  {
    loadW1AddressFromFile(SPIFFS, w1Paths[i], i);
  }

  w1Name[0] = readFile(SPIFFS, w1_1_name_Path.c_str());
  w1Name[1] = readFile(SPIFFS, w1_2_name_Path.c_str());
  w1Name[2] = readFile(SPIFFS, w1_3_name_Path.c_str());

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
                    {w1Name[0].c_str(), sensors.getTempC(w1Address[0])},
                    {w1Name[1].c_str(), sensors.getTempC(w1Address[1])},
                    {w1Name[2].c_str(), sensors.getTempC(w1Address[2])},
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
    Serial.println(mqtt_server);
    Serial.println(mqtt_port);
    mqClient.setServer(mqtt_server, mqtt_port);

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

/**
 * Output Message
 *
 * [{"bn":"sensor:12345"},{"n":"temperature","u":"C","v":17.60000038,"ut":1139},{"n":"humidity","u":"%","v":69.30000305,"ut":1139}]
 *
 */
void publishTemperatureHumidity(PubSubClient &_client, float _temperature, float _humidity)
{
  const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(4);
  DynamicJsonDocument doc(capacity);

  JsonObject obj1 = doc.createNestedObject();
  obj1["bn"] = "sensor:12345";

  JsonObject obj2 = doc.createNestedObject();
  obj2["n"] = "temperature";
  obj2["u"] = "C";
  obj2["v"] = _temperature;
  // obj2["v"] = static_cast<float>(_temperature);
  obj2["ut"] = (int)time(nullptr);

  JsonObject obj3 = doc.createNestedObject();
  obj3["n"] = "humidity";
  obj3["u"] = "%";
  obj3["v"] = _humidity;
  // obj3["v"] = static_cast<float>(_humidity);
  obj3["ut"] = (int)time(nullptr);

  char buffer[256];
  serializeJson(doc, buffer);
  _client.publish("ship/temperature", buffer);
  _client.publish("ship/humidity", buffer);
}

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

  // publishSimpleMessage(); // manual test
  if (mqttEnabled)
  {
    if (!mqClient.connected())
    {
      reconnectMQ();
    }
    mqClient.loop();

    float currentTemperature = readDHTTemperature();
    float currentHumidity = readDHTHumidity();

    // Calculate the percentage change from the previous values
    float tempChange = calculatePercentageChange(previousTemperature, currentTemperature);
    float humidityChange = calculatePercentageChange(previousHumidity, currentHumidity);

    unsigned long currentMillis = millis();
    unsigned long timeSinceLastPublish = currentMillis - lastPublishTime;

    bool shouldPublish = false;

        // Check temperature change criteria
    if (tempChange >= PERCENTAGE_THRESHOLD)
    {
      shouldPublish = true;
    }
    Serial.print("Temperature changed from ");
    Serial.print(previousTemperature);
    Serial.print(" to ");
    Serial.print(currentTemperature);
    Serial.print(". Percentage change: ");
    Serial.print(tempChange);

    // Check humidity change criteria
    if (humidityChange >= PERCENTAGE_THRESHOLD)
    {
      shouldPublish = true;
    }
    Serial.print("Humidity changed from ");
    Serial.print(previousHumidity);
    Serial.print(" to ");
    Serial.print(currentHumidity);
    Serial.print(". Percentage change: ");
    Serial.print(humidityChange);

    // Check time interval criteria
    if (timeSinceLastPublish >= PUBLISH_INTERVAL)
    {
      shouldPublish = true;
      Serial.println("Time interval since last publish is greater than the defined threshold. Publishing...");
    }
    else
    {
      Serial.print("Time to next threshold publish: ");
      Serial.print((PUBLISH_INTERVAL - timeSinceLastPublish) / 1000);
      Serial.println(" seconds.");
    }

    if (shouldPublish)
    {
      Serial.println("publishTemperatureHumidity then sleep...");
      publishTemperatureHumidity(mqClient, currentTemperature, currentHumidity);

      // Update the previous values after publishing
      previousTemperature = currentTemperature;
      previousHumidity = currentHumidity;
      lastPublishTime = currentMillis;
    }
    else
    {
      Serial.println("Skipping publish.");
    }

    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());

    float temperature = temperatureRead();
    Serial.println("Internal Temperature: " + String(temperature) + " C");
  }
  delay(mainDelay.toInt()); // Wait for 5 seconds before next loop
}
