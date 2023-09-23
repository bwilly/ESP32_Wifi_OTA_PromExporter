#include "SpiffsHandler.h"
#include "shared_vars.h"

const int W1_NUM_BYTES = 8; // The expected number of bytes

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
