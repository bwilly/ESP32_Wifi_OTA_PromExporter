// ConfigCodec.h
#pragma once

#include <ArduinoJson.h>
#include "ConfigModel.h"

// Capacity for config JSON documents (adjust if needed)
constexpr size_t APP_CONFIG_JSON_CAPACITY = 4096;

// Decode JSON object into AppConfig (modular / namespaced)
bool configFromJson(const JsonObject &root, AppConfig &cfg);

// Encode AppConfig into JSON object (same shape)
void configToJson(const AppConfig &cfg, JsonObject &root);
