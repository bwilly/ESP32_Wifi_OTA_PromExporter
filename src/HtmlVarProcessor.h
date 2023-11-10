#pragma once

#include <Arduino.h>

// String SendHTML(float tempSensor1, float tempSensor2, float tempSensor3);
String SendHTML(TemperatureReading *readings, int numReadings);
// Replaces placeholder with LED state value
// Only used for printing info
String processor(const String &var);