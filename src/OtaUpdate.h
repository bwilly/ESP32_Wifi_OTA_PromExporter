// OtaUpdate.h
#pragma once

#include <Arduino.h>

// Download a firmware image from the given URL and flash it via OTA.
// Returns true if the update was applied and the device will reboot.
bool performHttpOta(const String &url);
