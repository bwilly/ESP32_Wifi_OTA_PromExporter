#pragma once
#include <FS.h>

String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void loadW1AddressFromFile(fs::FS &fs, const char *path, uint8_t entryIndex);
void parseAndStoreHex(const String &value, uint8_t index);
