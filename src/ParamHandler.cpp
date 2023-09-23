#include "ParamHandler.h"
#include "SpiffsHandler.h"
#include "SPIFFS.h"
#include "shared_vars.h"

void handlePostParameters(AsyncWebServerRequest *request)
{
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost())
        {
            if (p->name() == PARAM_WIFI_SSID)
            {
                ssid = p->value().c_str();
                Serial.print("SSID set to: ");
                Serial.println(ssid);
                // Write file to save value
                writeFile(SPIFFS, ssidPath.c_str(), ssid.c_str());
            }
            // HTTP POST pass value
            if (p->name() == PARAM_WIFI_PASS)
            {
                pass = p->value().c_str();
                Serial.print("Password set to: ");
                Serial.println(pass);
                // Write file to save value
                writeFile(SPIFFS, passPath.c_str(), pass.c_str());
            }
            // HTTP POST location value
            if (p->name() == PARAM_LOCATION)
            {
                locationName = p->value().c_str();
                Serial.print("Location (for mDNS Hostname and Prometheus): ");
                Serial.println(locationName);
                // Write file to save value
                writeFile(SPIFFS, locationNamePath.c_str(), locationName.c_str());
            }
            // HTTP POST location value
            if (p->name() == PARAM_PIN_DHT)
            {
                pinDht = p->value().c_str();
                Serial.print("Pin (for DHT-22): ");
                Serial.println(pinDht);
                // Write file to save value
                writeFile(SPIFFS, pinDhtPath.c_str(), pinDht.c_str());
            }
            // HTTP POST location value
            if (p->name() == PARAM_MQTT_SERVER)
            {
                mqttServer = p->value().c_str();
                Serial.print("PARAM_MQTT_SERVER: ");
                Serial.println(mqttServer);
                // Write file to save value
                writeFile(SPIFFS, mqttServerPath.c_str(), mqttServer.c_str());
            }
            // HTTP POST location value
            if (p->name() == PARAM_MQTT_PORT)
            {
                mqttPort = p->value().c_str();
                Serial.print("PARAM_MQTT_PORT: ");
                Serial.println(mqttPort);
                // Write file to save value
                writeFile(SPIFFS, mqttPortPath.c_str(), mqttPort.c_str());
            }
            if (p->name() == PARAM_MAIN_DELAY)
            {
                mainDelay = p->value().c_str();
                Serial.print("PARAM_MAIN_DELAY: ");
                Serial.println(mainDelay);
                // Write file to save value
                writeFile(SPIFFS, mainDelayPath.c_str(), mainDelay.c_str());
            }
            if (p->name() == PARAM_W1_1)
            {
                parseAndStoreHex(p->value(), 0);
                writeFile(SPIFFS, w1_1Path.c_str(), p->value().c_str());
            }
            else if (p->name() == PARAM_W1_2)
            {
                parseAndStoreHex(p->value(), 1);
                writeFile(SPIFFS, w1_2Path.c_str(), p->value().c_str());
            }
            else if (p->name() == PARAM_W1_3)
            {
                parseAndStoreHex(p->value(), 2);
                writeFile(SPIFFS, w1_3Path.c_str(), p->value().c_str());
            }
            else if (p->name() == PARAM_ENABLE_W1)
            {
                String val = (p->value() == "on") ? "true" : "false";
                writeFile(SPIFFS, enableW1Path.c_str(), p->value().c_str());
            }
            else if (p->name() == PARAM_ENABLE_DHT)
            {
                String val = (p->value() == "on") ? "true" : "false";
                writeFile(SPIFFS, enableDHTPath.c_str(), p->value().c_str());
            }
            else if (p->name() == PARAM_ENABLE_MQTT)
            {
                String val = (p->value() == "on") ? "true" : "false";
                writeFile(SPIFFS, enableMQTTPath.c_str(), p->value().c_str());
            }
        }
    }
}
