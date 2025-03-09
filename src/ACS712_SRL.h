#ifndef ACS712SENSOR_H
#define ACS712SENSOR_H

#include <Arduino.h>

// @pattern class
// Class definition for ACS712Sensor
class ACS712Sensor {
private:
    int _pinAcs;
    
    // ACS712 Calibration Values
    static constexpr float SENSOR_OFFSET = 2.5;   // Midpoint of sensor output at 0A (in volts)
    static constexpr float MV_PER_AMP = 185.0;    // Sensitivity for ACS712-5A: 185mV/A
    static constexpr float ADC_MAX_VOLTAGE = 3.3; // ESP32 ADC max voltage
    static constexpr int ADC_RESOLUTION = 4095;   // ESP32 ADC resolution (12-bit)

    // If using a different ACS712 variant (e.g., 20A or 30A), adjust MV_PER_AMP accordingly:
    // ACS712-20A: 100 mV/A
    // ACS712-30A: 66 mV/A

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
