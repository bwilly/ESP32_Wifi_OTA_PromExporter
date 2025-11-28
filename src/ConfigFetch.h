// ConfigFetch.h
#pragma once

#include <Arduino.h>

// Download a JSON config from the given URL into outJson.
// Returns true on success (HTTP 200 and non-empty body).
bool downloadConfigJson(const String &url, String &outJson);
