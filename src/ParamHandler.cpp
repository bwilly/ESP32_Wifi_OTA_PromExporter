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
        }
    }
}
