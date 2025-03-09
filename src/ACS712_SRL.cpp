#include "ACS712_SRL.h"
#include "version.h" // Version macros
#include "shared_vars.h"

// @pattern class
// Constructor with pin parameter
ACS712Sensor::ACS712Sensor(int pin) : _pinAcs(pin) {}

void ACS712Sensor::begin() {
    Serial.begin(115200);

    // Print version info
    Serial.println("Application Version: " APP_VERSION);
    Serial.println("Commit Hash: " APP_COMMIT_HASH);

    pinMode(_pinAcs, INPUT);
    Serial.println("ACS712 Current Sensor Initialized on pin " + String(_pinAcs));
}

// Helper function to read DC current
float ACS712Sensor::readCurrentDC() {
    // Read raw ADC value
    int rawValue = analogRead(_pinAcs);

    // Convert raw ADC value to voltage
    float voltage = (rawValue * ADC_MAX_VOLTAGE) / ADC_RESOLUTION;

    // Calculate current (in Amps)
    float current = (voltage - SENSOR_OFFSET) / (MV_PER_AMP / 1000.0);

    return current; // Current in Amps
}

void ACS712Sensor::serialOutInfo() {
    // Read and print current
    float current = readCurrentDC();
    Serial.print("DC Current: ");
    Serial.print(current);
    Serial.println(" A");
}

// Retrieve pin number from shared variable map
int __pinAcs = paramToVariableMap["pinAcs"]->toInt(); // @pattern:toInt #2 todo: study these to patterns. which do i want? Mar8'25

// Instantiate the sensor using the mapped pin
ACS712Sensor sensor(__pinAcs);

// External functions for calling from another file
void setupACS712() {
    sensor.begin();
}

void loopACS712() {
    sensor.serialOutInfo();
}
