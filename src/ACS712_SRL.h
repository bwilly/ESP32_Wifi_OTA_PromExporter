#ifndef ACS712SENSOR_H
#define ACS712SENSOR_H

#include <Arduino.h>

// Nov 25 2025 calibration notes taken using multimete
// 4.56 A eng main bilge pump wet (load)
// 3.76-3.80 A eng main bilge pump dry (on but no load)

// @pattern class
// Class definition for ACS712Sensor
class ACS712Sensor {
private:
    int _pinAcs;
    
    // ACS712 Calibration Values
    // static constexpr float SENSOR_OFFSET = 2.5f;   // Midpoint of sensor output at 0A (in volts)
    
    // If using a different ACS712 variant (e.g., 20A or 30A), adjust MV_PER_AMP accordingly:
    // ACS712-20A: 100 mV/A
    // ACS712-30A: 66 mV/A
    //static constexpr float MV_PER_AMP = 185.0;    // Sensitivity for ACS712-5A: 185mV/A


     // ACS712 Calibration Values
    // static constexpr float ADC_MAX_VOLTAGE = 3.3f; // ESP32 ADC max voltage
    // static constexpr int ADC_RESOLUTION = 4095;   // ESP32 ADC resolution (12-bit)
    // static constexpr float ADC_ZERO_VOLT = 1.047f;
    // static constexpr float MV_PER_AMP      = 100.0f;   // datasheet
    // static constexpr float DIV_RATIO       = 0.5f;     // ideal 10k/10k voltage divider per amp
    // static constexpr float CAL_GAIN        = 1.52f;    // from your meter vs ADC
    // static constexpr float ADC_SENSITIVITY = (MV_PER_AMP / 1000.0f) * DIV_RATIO * CAL_GAIN;

    // ADC characteristics
    static constexpr float ADC_MAX_VOLTAGE = 3.3f;
    static constexpr int   ADC_RESOLUTION  = 4095;

    // Calibration: measured ADC zero when pump off
    static constexpr float ADC_ZERO_VOLT   = 1.047f;   // your measured zero

    // Sensor: ACS712-20A chip spec
    static constexpr float MV_PER_AMP      = 100.0f;   // mV per amp at sensor

    // Divider: ideal 10k / 10k
    static constexpr float DIV_RATIO       = 0.5f;

    // Calibration gain tuned from wet & dry readings
    static constexpr float CAL_GAIN        = 1.18125f;

    // Final V/A sensitivity at the ADC pin
    static constexpr float ADC_SENSITIVITY =
        (MV_PER_AMP / 1000.0f) * DIV_RATIO * CAL_GAIN;
    // ~0.0641 V/A

    static constexpr int   NUM_SAMPLES     = 16;  // number of samples for averaging



    
    // V per A seen at ADC pin:
    // static constexpr float ADC_SENSITIVITY = (MV_PER_AMP / 1000.0f) * (DIV_R2 / (DIV_R1 + DIV_R2));


public:
    // Constructor with pin parameter
    explicit ACS712Sensor(int pin);

    void begin(); // Initializes the sensor
    float readCurrentDC(); // Reads the DC current value
    void serialOutInfo(); // Prints current readings to Serial
};

// Function declarations for external usage
void setupACS712();
void loopACS712();
float readACS712Current();


#endif // ACS712SENSOR_H
