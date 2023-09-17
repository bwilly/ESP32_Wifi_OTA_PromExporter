#pragma once
#include "shared_vars.h"

#define MAX_READINGS 4
char *readAndGeneratePrometheusExport(const char *location);
char *buildPrometheusMultiTemptExport(TemperatureReading readings[MAX_READINGS]);