#ifndef SCT_SRL_H
#define SCT_SRL_H

#include <Arduino.h>

// @pattern class (modeled after ACS712Sensor)
// Class definition for SctSensor: generic 1V-output CT (e.g. SCT-013-010/015/020)
class SctSensor {
private:
    int   _pinAdc;
    float _ratedAmps;   // e.g. 10, 15, 20 for 10A/1V, 15A/1V, 20A/1V
    float _ratedVrms;   // usually 1.0 V RMS for these CT variants
    float _calGain;     // user calibration factor (default 1.0)

    // ADC characteristics
    static constexpr float ADC_MAX_VOLTAGE = 3.3f;  // ESP32 ADC range
    static constexpr int   ADC_RESOLUTION  = 4095;  // 12-bit

    // How many samples to use for RMS measurement
    static constexpr int   NUM_SAMPLES_RMS = 2000;  // adjust if needed

public:
    // Constructor: pin, ratedAmps (10, 15, 20), ratedVrms (1.0), calGain (1.0)
    explicit SctSensor(int pinAdc,
                       float ratedAmps = 15.0f,
                       float ratedVrms = 1.0f,
                       float calGain   = 1.0f);

    void begin();                 // Initializes the sensor
    float readCurrentACRms();     // Reads AC RMS current in amps
    void serialOutInfo();         // Prints RMS current to Serial
    void serialOutAdcDebug();     // Prints raw ADC bias (counts + volts)

    // Runtime configuration
    void setRatedAmps(float ratedAmps)   { _ratedAmps = ratedAmps; }
    void setCalibration(float calGain)   { _calGain   = calGain;   }
    void setRatedVrms(float ratedVrms)   { _ratedVrms = ratedVrms; }
};



#endif // SCT_SRL_H
