#include "shared_vars.h"
#include "SpiffsHandler.h"
#include "SPIFFS.h"

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
    ptr += "<p>";
    ptr += w1Name[0].c_str();
    ptr += ": ";
    ptr += tempSensor1;
    ptr += "&deg;C</p>";
    ptr += "<p>";
    ptr += w1Name[1].c_str();
    ptr += ": ";
    ptr += tempSensor2;
    ptr += "&deg;C</p>";
    ptr += "<p>";
    ptr += w1Name[2].c_str();
    ptr += ": ";
    ptr += tempSensor3;
    ptr += "&deg;C</p>";
    ptr += "</div>\n";
    ptr += "</body>\n";
    ptr += "</html>\n";
    return ptr;
}

// Replaces placeholder with LED state value
// Only used for printing info
String processor(const String &var)
{
    Serial.println("process template token: ");
    Serial.println(var);

    if (var == "STATE") // this is from the example sample
    {
        // if (digitalRead(ledPin))
        // {
        //     ledState = "ON";
        // }
        // else
        // {
        //     ledState = "OFF";
        // }
        // return ledState;
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
    else if (var == "w1-1-name")
    {
        return readFile(SPIFFS, w1_1_name_Path.c_str());
    }
    else if (var == "w1-2-name")
    {
        return readFile(SPIFFS, w1_2_name_Path.c_str());
    }
    else if (var == "w1-3-name")
    {
        return readFile(SPIFFS, w1_3_name_Path.c_str());
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
