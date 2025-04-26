#ifndef ACS712SENSOR_H
#define ACS712SENSOR_H

#include <Arduino.h>

// @pattern class
// Class definition for ACS712Sensor
class ACS712Sensor {
private:
    int _pinAcs;
    
    // ACS712 Calibration Values
    static constexpr float SENSOR_OFFSET = 2.5f;   // Midpoint of sensor output at 0A (in volts)
    static constexpr float MV_PER_AMP = 100.0f;    // Sensitivity for ACS712-20A: 
    static constexpr float ADC_MAX_VOLTAGE = 3.3f; // ESP32 ADC max voltage
    static constexpr int ADC_RESOLUTION = 4095;   // ESP32 ADC resolution (12-bit)

    // If using a different ACS712 variant (e.g., 20A or 30A), adjust MV_PER_AMP accordingly:
    // ACS712-20A: 100 mV/A
    // ACS712-30A: 66 mV/A
    //static constexpr float MV_PER_AMP = 185.0;    // Sensitivity for ACS712-5A: 185mV/A


    // === Voltage-divider (sensor→ADC pin→GND) ===
    static constexpr float DIV_R1 = 10000.0f;  // Ω from sensor→ADC
    static constexpr float DIV_R2 = 10000.0f;  // Ω from ADC→GND

    // === Precomputed for ADC readings ===
    // voltage at ADC pin that corresponds to 0 A:
    // static constexpr float ADC_ZERO_VOLT = SENSOR_OFFSET * (DIV_R2 / (DIV_R1 + DIV_R2));
    
    // (1) use your *measured* Vdiv at zero current (pump off = ~1.02 V)
// static constexpr float ADC_ZERO_VOLT = 1.02f;
static constexpr float ADC_ZERO_VOLT = 1.047f;
    
    // V per A seen at ADC pin:
    static constexpr float ADC_SENSITIVITY = (MV_PER_AMP / 1000.0f) * (DIV_R2 / (DIV_R1 + DIV_R2));


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

#endif // ACS712SENSOR_H
