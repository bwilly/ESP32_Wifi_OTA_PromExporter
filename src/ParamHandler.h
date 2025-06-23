#pragma once

#include <FS.h>
#include "ESPAsyncWebServer.h"
#include "shared_vars.h"

// Make sure you declare any constants or variables (e.g. PARAM_WIFI_SSID, ssidPath, etc.)
// that are needed from the main file.

void handlePostParameters(AsyncWebServerRequest *request);
