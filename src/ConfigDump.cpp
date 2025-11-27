// ConfigDump.cpp
#include "ConfigDump.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "shared_vars.h"

// You can tweak this later if you add more params
static const size_t CONFIG_JSON_CAPACITY = 4096;

// Helper: convert raw bytes to uppercase hex string, e.g. "28FF3C1234567890"
static String bytesToHexString(const uint8_t *data, size_t len)
{
    char buf[2 * 8 + 1];  // W1_NUM_BYTES is 8; keep simple and fixed
    size_t outLen = len * 2;

    if (outLen > sizeof(buf) - 1) {
        outLen = sizeof(buf) - 1;
    }

    for (size_t i = 0; i < len && (i * 2 + 1) < sizeof(buf); ++i) {
        sprintf(&buf[i * 2], "%02X", data[i]);
    }
    buf[outLen] = '\0';

    return String(buf);
}

// Build a flat JSON doc from the *current in-memory values*
// (paramToVariableMap, paramToBoolMap, w1Address, w1Name)
String buildConfigJsonFlat()
{
    StaticJsonDocument<CONFIG_JSON_CAPACITY> doc;

    // 1) All String params (ssid, pass, location, pinDht, mqtt-server, etc.)
    for (auto &entry : paramToVariableMap) {
        const String &key   = entry.first;
        String       *value = entry.second;

        if (value != nullptr) {
            doc[key] = *value;
        }
    }

    // 2) All bool params (enableW1, enableDHT, enableAcs712, enableMQTT)
    for (auto &entry : paramToBoolMap) {
        const String &key   = entry.first;
        bool         *value = entry.second;

        if (value != nullptr) {
            doc[key] = *value;
        }
    }

    // 3) W1 sensor hex + names → legacy flat keys: w1-1, w1-1-name, ... w1-6
    for (int i = 0; i < 6; ++i) {
        String hexKey  = "w1-" + String(i + 1);
        String nameKey = hexKey + "-name";

        bool hasName = w1Name[i].length() > 0;
        bool hasAddr = false;

        for (int b = 0; b < W1_NUM_BYTES; ++b) {
            if (w1Address[i][b] != 0x00) {
                hasAddr = true;
                break;
            }
        }

        if (hasAddr) {
            doc[hexKey] = bytesToHexString(w1Address[i], W1_NUM_BYTES);
        }

        if (hasName) {
            doc[nameKey] = w1Name[i];
        }
    }

    String out;
    serializeJsonPretty(doc, out);  // pretty for human export
    return out;
}

// Optional: write a backup file into SPIFFS, e.g. "/config-backup.json"
bool saveConfigBackupToFile(const char *path = "/config-backup.json")
{
    String json = buildConfigJsonFlat();

    if (!SPIFFS.begin(true)) {
        Serial.println(F("saveConfigBackupToFile: SPIFFS.begin() failed"));
        return false;
    }

    File f = SPIFFS.open(path, FILE_WRITE);  // truncates or creates
    if (!f) {
        Serial.println(F("saveConfigBackupToFile: open failed"));
        return false;
    }

    size_t written = f.print(json);
    f.close();

    return written == json.length();
}
