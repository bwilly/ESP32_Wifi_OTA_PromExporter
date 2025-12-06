#include "SCT_SRL.h"
#include "version.h"     // Version macros (same pattern as ACS)
#include "shared_vars.h" // paramToVariableMap etc. (same pattern as ACS)

// ------------------------
// SctSensor implementation
// ------------------------

SctSensor::SctSensor(int pinAdc,
                     float ratedAmps,
                     float ratedVrms,
                     float calGain)
    : _pinAdc(pinAdc),
      _ratedAmps(ratedAmps),
      _ratedVrms(ratedVrms),
      _calGain(calGain)
{
}

void SctSensor::begin() {
    Serial.begin(115200);

    // Print version info (same style as ACS712_SRL)
    Serial.println("Application Version: " APP_VERSION);
    Serial.println("Commit Hash: " APP_COMMIT_HASH);

    // BUG NOTE (paralleling ACS): ultimately you may want to externalize pin from params
    // For now, trust the constructor pin and just set the mode.
    pinMode(_pinAdc, INPUT);

    Serial.print("SCT Current Sensor Initialized on ADC pin ");
    Serial.println(_pinAdc);
}

/**
 * Read AC RMS current from a 1V-output CT (e.g. SCT-013-010/015/020)
 *
 * Hardware assumption:
 *   - CT output is biased around ~1.65 V via R1/R2 + C1 (+ optional C2),
 *     and fed into this ADC pin.
 *
 * Procedure:
 *   1. Measure DC offset (bias) from a set of samples.
 *   2. Take a bunch of samples, subtract offset → pure AC component.
 *   3. Compute RMS of that AC in ADC counts.
 *   4. Convert counts → volts.
 *   5. Convert volts → amps using ratedAmps/ratedVrms and calGain.
 */
float SctSensor::readCurrentACRms() {
    // 1) Estimate the actual DC offset at the ADC pin
    const int offsetSamples = 200;
    long offsetSum = 0;
    for (int i = 0; i < offsetSamples; ++i) {
        int raw = analogRead(_pinAdc);
        offsetSum += raw;
        // tiny delay to avoid hammering ADC too hard
        delayMicroseconds(100);
    }
    float adcOffset = static_cast<float>(offsetSum) / offsetSamples;

    // 2) Collect AC samples and compute sum of squares after removing offset
    double sumSq = 0.0;
    for (int i = 0; i < NUM_SAMPLES_RMS; ++i) {
        int raw = analogRead(_pinAdc);
        float centered = static_cast<float>(raw) - adcOffset; // remove DC bias
        sumSq += static_cast<double>(centered) * static_cast<double>(centered);

        // Adjust this for your CPU/time budget; this value is "safe" for 50/60 Hz
        delayMicroseconds(200);
    }

    // 3) RMS in ADC counts
    float adcRmsCounts = sqrt(sumSq / NUM_SAMPLES_RMS);

    // 4) Convert ADC counts -> volts (AC component only)
    float voltsPerCount = ADC_MAX_VOLTAGE / ADC_RESOLUTION;
    float vRms = adcRmsCounts * voltsPerCount;

    // 5) Convert volts -> amps using CT rating and calibration
    //    e.g., SCT-013-015: 1.0 V RMS -> 15 A RMS
    float ampsRms = (vRms / _ratedVrms) * _ratedAmps * _calGain;

    return ampsRms;
}

void SctSensor::serialOutInfo() {
    float amps = readCurrentACRms();
    Serial.print("AC Current (SCT): ");
    Serial.print(amps, 3);
    Serial.println(" A RMS");
}



