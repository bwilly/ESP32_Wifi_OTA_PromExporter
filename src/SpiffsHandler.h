#pragma once
#include <FS.h>

// For SPIFFS
extern String ssidPath;
extern String passPath;
extern String locationNamePath;
extern String pinDhtPath;
extern String mqttServerPath;
extern String mqttPortPath;
extern String mainDelayPath;
extern String w1_1Path;
extern String w1_2Path;
extern String w1_3Path;
extern String w1_1_name_Path;
extern String w1_2_name_Path;
extern String w1_3_name_Path;
extern String enableW1Path;
extern String enableDHTPath;
extern String enableMQTTPath;

String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void loadW1AddressFromFile(fs::FS &fs, const char *path, uint8_t entryIndex);
void parseAndStoreHex(const String &value, uint8_t index);
void loadPersistedValues();
