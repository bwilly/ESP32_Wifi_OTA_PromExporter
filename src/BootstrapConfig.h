#pragma once
#include <Arduino.h>

// Saves validated bootstrap config JSON into /config.json (modular)
// errOut is set on failure.
bool saveBootstrapConfigJson(const String &jsonBody, String &errOut);
