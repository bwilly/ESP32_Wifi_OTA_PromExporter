#include "SpiffsHandler.h"
#include "shared_vars.h"
#include "SPIFFS.h"
#include "ParamMetadata.h"

String makePath(const char *param)
{
    return String("/") + param + ".txt";
}

// // For SPIFFS
// String ssidPath = makePath(PARAM_WIFI_SSID);
// String passPath = makePath(PARAM_WIFI_PASS);
// String locationNamePath = makePath(PARAM_LOCATION);
// String pinDhtPath = makePath(PARAM_PIN_DHT);
// String mqttServerPath = makePath(PARAM_MQTT_SERVER);
// String mqttPortPath = makePath(PARAM_MQTT_PORT);
// String mainDelayPath = makePath(PARAM_MAIN_DELAY);
// String w1_1Path = makePath(PARAM_W1_1);
// String w1_2Path = makePath(PARAM_W1_2);
// String w1_3Path = makePath(PARAM_W1_3);
// String w1_1_name_Path = makePath(PARAM_W1_1_NAME);
// String w1_2_name_Path = makePath(PARAM_W1_2_NAME);
// String w1_3_name_Path = makePath(PARAM_W1_3_NAME);
// String enableW1Path = makePath(PARAM_ENABLE_W1);
// String enableDHTPath = makePath(PARAM_ENABLE_DHT);
// String enableMQTTPath = makePath(PARAM_ENABLE_MQTT);

void loadPersistedValues()
{
    for (const auto &param : paramList)
    {
        String value = readFile(SPIFFS, param.spiffsPath.c_str());

        // Assign value to the appropriate global String variable
        if (param.type == ParamMetadata::STRING)
        {
            if (paramToVariableMap.find(param.name) != paramToVariableMap.end())
            {
                *(paramToVariableMap[param.name]) = value;
            }
        }
        // // Convert value to a number and assign it to the appropriate global variable
        // else if (param.type == ParamMetadata::NUMBER) {
        //     if (paramToIntMap.find(param.name) != paramToIntMap.end()) {
        //         *(paramToIntMap[param.name]) = value.toInt();
        //     }
        // }
        // Convert value to a boolean and assign it to the appropriate global variable
        else if (param.type == ParamMetadata::BOOLEAN)
        {
            if (paramToBoolMap.find(param.name) != paramToBoolMap.end())
            {
                *(paramToBoolMap[param.name]) = (value == "true");
            }
        }
    }
}

// void loadPersistedValues()
// {
//     // Load values saved in SPIFFS
//     Serial.println("loading SPIFFS values...");
//     ssid = readFile(SPIFFS, ssidPath.c_str());
//     pass = readFile(SPIFFS, passPath.c_str());
//     locationName = readFile(SPIFFS, locationNamePath.c_str());
//     pinDht = readFile(SPIFFS, pinDhtPath.c_str());
//     mqttServer = readFile(SPIFFS, mqttServerPath.c_str());
//     mqttPort = readFile(SPIFFS, mqttPortPath.c_str());
//     mainDelay = readFile(SPIFFS, mainDelayPath.c_str()).toInt();
//     w1Enabled = (readFile(SPIFFS, enableW1Path.c_str()) == "true");
//     dhtEnabled = (readFile(SPIFFS, enableDHTPath.c_str()) == "true");
//     mqttEnabled = (readFile(SPIFFS, enableMQTTPath.c_str()) == "true");
// }

void parseHexToArray(const String &value, uint8_t array[8])
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

void loadW1AddressFromFile(fs::FS &fs, const char *path, uint8_t entryIndex)
{
    String content = readFile(fs, path);
    if (content.length() > 0)
    {
        parseHexToArray(content, w1Address[entryIndex]); // todo:refactor so w1Address is passed as an arg rather than be global
    }
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

void parseAndStoreHex(const String &value, uint8_t index)
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

            w1Address[index][byteCount] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);

            subValue = subValue.substring(commaIndex + 1);
            byteCount++;
        }
        if (byteCount < W1_NUM_BYTES)
        { // Process the last byte
            w1Address[index][byteCount] = (uint8_t)strtol(subValue.c_str(), NULL, 16);
        }
    }

    // Debug output
    for (int i = 0; i < W1_NUM_BYTES; i++)
    {
        Serial.print("w1Address[");
        Serial.print(index);
        Serial.print("][");
        Serial.print(i);
        Serial.print("] = ");
        Serial.println(w1Address[index][i], HEX);
    }
}
