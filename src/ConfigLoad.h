// ConfigLoad.h
#pragma once

#include <Arduino.h>

// Load/apply config from a JSON string (already in memory)
bool loadConfigFromJsonString(const String &json);

// Load/apply config from a JSON file on SPIFFS (e.g. "/config.json")
bool loadConfigFromJsonFile(const char *path = "/config.json");
