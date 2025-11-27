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

    // BUG: The following approach is causing GPIO0 to be used instead of the expected pin.
    // _pinAcs = paramToVariableMap["pinAcs"]->toInt(); // Original approach (temporarily disabled)

    // Temporary fix: Hardcode the pin to GPIO32
    _pinAcs = 32; // todo: externalize

    pinMode(_pinAcs, INPUT);
    Serial.println("ACS712 Current Sensor Initialized on pin " + String(_pinAcs));
}

// Helper function to read DC current
// float ACS712Sensor::readCurrentDC() {
//     // Read raw ADC value
//     int rawValue = analogRead(_pinAcs);    

//     float Vdiv = rawValue * ADC_MAX_VOLTAGE / ADC_RESOLUTION;
//     float amps = (Vdiv - ADC_ZERO_VOLT) / ADC_SENSITIVITY;

//     return amps;
// }

float ACS712Sensor::readCurrentDC() {
    float sumAmps = 0.0f;

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        int rawValue = analogRead(_pinAcs);
        float Vdiv = rawValue * ADC_MAX_VOLTAGE / ADC_RESOLUTION;
        float amps = (Vdiv - ADC_ZERO_VOLT) / ADC_SENSITIVITY;
        sumAmps += amps;

        // optional: smooth out ADC sampling timing
        // delayMicroseconds(200);
    }

    float avgAmps = sumAmps / NUM_SAMPLES;
    return avgAmps;
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

float readACS712Current() {
  return sensor.readCurrentDC();
}

