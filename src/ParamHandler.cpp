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
            if (p->name() == PARAM_INPUT_1)
            {
                ssid = p->value().c_str();
                Serial.print("SSID set to: ");
                Serial.println(ssid);
                // Write file to save value
                writeFile(SPIFFS, ssidPath, ssid.c_str());
            }
            // HTTP POST pass value
            if (p->name() == PARAM_INPUT_2)
            {
                pass = p->value().c_str();
                Serial.print("Password set to: ");
                Serial.println(pass);
                // Write file to save value
                writeFile(SPIFFS, passPath, pass.c_str());
            }
            // HTTP POST location value
            if (p->name() == PARAM_INPUT_3)
            {
                locationName = p->value().c_str();
                Serial.print("Location (for mDNS Hostname and Prometheus): ");
                Serial.println(locationName);
                // Write file to save value
                writeFile(SPIFFS, locationNamePath, locationName.c_str());
            }
            // HTTP POST location value
            if (p->name() == PARAM_INPUT_4)
            {
                pinDht = p->value().c_str();
                Serial.print("Pin (for DHT-22): ");
                Serial.println(pinDht);
                // Write file to save value
                writeFile(SPIFFS, pinDhtPath, pinDht.c_str());
            }
        }
    }
}
