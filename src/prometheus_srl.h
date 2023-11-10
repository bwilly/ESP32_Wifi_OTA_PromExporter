#pragma once
#include "shared_vars.h"
#include "Config.h"
#include "TemperatureReading.h"
// #define MAX_READINGS 4
char *readAndGeneratePrometheusExport(const char *location);
char *buildPrometheusMultiTemptExport(TemperatureReading *readings);