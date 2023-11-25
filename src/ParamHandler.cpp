// In ParamHandler.cpp
#include "ParamHandler.h"
#include "SpiffsHandler.h"
#include "SPIFFS.h"
#include "shared_vars.h"

// todo: refactor this is copy/paste from SpiffsHandler.cpp
void parseHexToArray_dup(const String &value, uint8_t array[8])
{
    int startIndex = value.indexOf('{');
    int endIndex = value.indexOf('}');

    if (startIndex != -1 && endIndex != -1)
    {
        String subValue = value.substring(startIndex + 1, endIndex);

        int byteCount = 0;
        int commaIndex;
        while ((commaIndex = subValue.indexOf(',')) != -1 && byteCount < W1_NUM_BYTES)
        {
            // String byteStr = subValue.substring(0, commaIndex).trim();
            String byteStr = subValue.substring(0, commaIndex);
            byteStr.trim();
            array[byteCount] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);

            subValue = subValue.substring(commaIndex + 1);
            byteCount++;
        }
        if (byteCount < W1_NUM_BYTES)
        { // Process the last byte
            array[byteCount] = (uint8_t)strtol(subValue.c_str(), NULL, 16);
        }
    }
}

void handlePostParameters(AsyncWebServerRequest *request)
{
    for (const auto &paramMetadata : paramList)
    {
        if (request->hasParam(paramMetadata.name.c_str(), true))
        {
            AsyncWebParameter *p = request->getParam(paramMetadata.name.c_str(), true);

            if (paramMetadata.type == ParamMetadata::STRING)
            {
                // Handle String type
                if (paramToVariableMap.find(paramMetadata.name) != paramToVariableMap.end())
                {
                    *(paramToVariableMap[paramMetadata.name]) = p->value().c_str();
                }
            }
            else if (paramMetadata.type == ParamMetadata::BOOLEAN)
            {
                // Handle Boolean type
                if (paramToBoolMap.find(paramMetadata.name) != paramToBoolMap.end())
                {
                    *(paramToBoolMap[paramMetadata.name]) = (p->value() == "true" || p->value() == "on");
                }
            }
            // else if (paramMetadata.type == ParamMetadata::NUMBER)
            // {
            //     // Handle Number type
            //     if (paramToIntMap.find(paramMetadata.name) != paramToIntMap.end())
            //     {
            //         *(paramToIntMap[paramMetadata.name]) = p->value().toInt();
            //     }
            // }

            if (paramMetadata.name == "w1-1" || paramMetadata.name == "w1-2" || paramMetadata.name == "w1-3")
            {
                // Special handling for w1Address array
                int index = (paramMetadata.name == "w1-1") ? 0 : (paramMetadata.name == "w1-2") ? 1
                                                                                                : 2;
                parseHexToArray_dup(p->value(), w1Address[index]);
            }

            // Special handling for w1Name array
            if (paramMetadata.name == PARAM_W1_1_NAME)
            {
                w1Name[0] = p->value().c_str();
            }
            else if (paramMetadata.name == PARAM_W1_2_NAME)
            {
                w1Name[1] = p->value().c_str();
            }
            else if (paramMetadata.name == PARAM_W1_3_NAME)
            {
                w1Name[2] = p->value().c_str();
            }

            writeFile(SPIFFS, paramMetadata.spiffsPath.c_str(), p->value().c_str());
        }
    }
}

// void handlePostParameters(AsyncWebServerRequest *request)
// {
//     // Default values
//     bool enableW1 = false;
//     bool enableDHT = false;
//     bool enableMQTT = false;

//     bool enableW1_checked = false;
//     bool enableDHT_checked = false;
//     bool enableMQTT_checked = false;

//     int params = request->params();
//     for (int i = 0; i < params; i++)
//     {
//         AsyncWebParameter *p = request->getParam(i);
//         // if (p->isPost())
//         if (p->isPost() && p->value().length() > 0) // This checks if the value is not empty
//         {
//             if (p->name() == PARAM_WIFI_SSID)
//             {
//                 ssid = p->value().c_str();
//                 Serial.print("SSID set to: ");
//                 Serial.println(ssid);
//                 // Write file to save value
//                 writeFile(SPIFFS, ssidPath.c_str(), ssid.c_str());
//             }
//             // HTTP POST pass value
//             if (p->name() == PARAM_WIFI_PASS)
//             {
//                 pass = p->value().c_str();
//                 Serial.print("Password set to: ");
//                 Serial.println(pass);
//                 // Write file to save value
//                 writeFile(SPIFFS, passPath.c_str(), pass.c_str());
//             }
//             // HTTP POST location value
//             if (p->name() == PARAM_LOCATION)
//             {
//                 locationName = p->value().c_str();
//                 Serial.print("Location (for mDNS Hostname and Prometheus): ");
//                 Serial.println(locationName);
//                 // Write file to save value
//                 writeFile(SPIFFS, locationNamePath.c_str(), locationName.c_str());
//             }
//             // HTTP POST location value
//             if (p->name() == PARAM_PIN_DHT)
//             {
//                 pinDht = p->value().c_str();
//                 Serial.print("Pin (for DHT-22): ");
//                 Serial.println(pinDht);
//                 // Write file to save value
//                 writeFile(SPIFFS, pinDhtPath.c_str(), pinDht.c_str());
//             }
//             // HTTP POST location value
//             if (p->name() == PARAM_MQTT_SERVER)
//             {
//                 mqttServer = p->value().c_str();
//                 Serial.print("PARAM_MQTT_SERVER: ");
//                 Serial.println(mqttServer);
//                 // Write file to save value
//                 writeFile(SPIFFS, mqttServerPath.c_str(), mqttServer.c_str());
//             }
//             // HTTP POST location value
//             if (p->name() == PARAM_MQTT_PORT)
//             {
//                 mqttPort = p->value().c_str();
//                 Serial.print("PARAM_MQTT_PORT: ");
//                 Serial.println(mqttPort);
//                 // Write file to save value
//                 writeFile(SPIFFS, mqttPortPath.c_str(), mqttPort.c_str());
//             }
//             if (p->name() == PARAM_MAIN_DELAY)
//             {
//                 mainDelay = p->value().c_str();
//                 Serial.print("PARAM_MAIN_DELAY: ");
//                 Serial.println(mainDelay);
//                 // Write file to save value
//                 writeFile(SPIFFS, mainDelayPath.c_str(), mainDelay.c_str());
//             }
//             if (p->name() == PARAM_W1_1_NAME)
//             {
//                 w1Name[0] = p->value().c_str();
//                 Serial.print("PARAM_W1_1_NAME: ");
//                 Serial.println(w1Name[0]);
//                 // Write file to save value
//                 writeFile(SPIFFS, w1_1_name_Path.c_str(), w1Name[0].c_str());
//             }
//             if (p->name() == PARAM_W1_2_NAME)
//             {
//                 w1Name[1] = p->value().c_str();
//                 Serial.print("PARAM_W1_2_NAME: ");
//                 Serial.println(w1Name[1]);
//                 // Write file to save value
//                 writeFile(SPIFFS, w1_2_name_Path.c_str(), w1Name[1].c_str());
//             }
//             if (p->name() == PARAM_W1_3_NAME)
//             {
//                 w1Name[2] = p->value().c_str();
//                 Serial.print("PARAM_W1_3_NAME: ");
//                 Serial.println(w1Name[2]);
//                 // Write file to save value
//                 writeFile(SPIFFS, w1_3_name_Path.c_str(), w1Name[2].c_str());
//             }
//             if (p->name() == PARAM_W1_1)
//             {
//                 parseAndStoreHex(p->value(), 0);
//                 writeFile(SPIFFS, w1_1Path.c_str(), p->value().c_str());
//             }
//             else if (p->name() == PARAM_W1_2)
//             {
//                 parseAndStoreHex(p->value(), 1);
//                 writeFile(SPIFFS, w1_2Path.c_str(), p->value().c_str());
//             }
//             else if (p->name() == PARAM_W1_3)
//             {
//                 parseAndStoreHex(p->value(), 2);
//                 writeFile(SPIFFS, w1_3Path.c_str(), p->value().c_str());
//             }

//             else if (p->name() == PARAM_ENABLE_W1 && p->value() == "on")
//             {
//                 enableW1 = true;
//             }
//             else if (p->name() == PARAM_ENABLE_DHT && p->value() == "on")
//             {
//                 enableDHT = true;
//             }
//             else if (p->name() == PARAM_ENABLE_MQTT && p->value() == "on")
//             {
//                 enableMQTT = true;
//             }
//         }
//     }
//     // Now, save these to SPIFFS regardless of whether they were received in the POST or not.
//     writeFile(SPIFFS, enableW1Path.c_str(), enableW1 ? "true" : "false");
//     writeFile(SPIFFS, enableDHTPath.c_str(), enableDHT ? "true" : "false");
//     writeFile(SPIFFS, enableMQTTPath.c_str(), enableMQTT ? "true" : "false");
// }
