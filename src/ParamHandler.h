#pragma once

#include <FS.h>
#include "ESPAsyncWebServer.h"

// Make sure you declare any constants or variables (e.g. PARAM_INPUT_1, ssidPath, etc.)
// that are needed from the main file.

void handlePostParameters(AsyncWebServerRequest *request);
