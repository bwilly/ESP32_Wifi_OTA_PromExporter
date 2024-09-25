#include <Arduino.h>
#include "DHT_SRL.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "shared_vars.h"

#define DHTTYPE DHT22 // DHT 22 (AM2302)

// Create a DHT object
DHT dht(0, DHTTYPE); // We'll set the pin in the configDHT() function

// Task and synchronization handles
TaskHandle_t sensorTaskHandle = NULL;
SemaphoreHandle_t xSensorDataMutex;

// Shared sensor data variables
float temperature = NAN;
float humidity = NAN;

// Function prototypes
void configDHT();
float roundToHundredth(float number);
void sensorTask(void *parameter);
void initSensorTask();

void configDHT()
{
    Serial.println("config DHT...");
    if (pinDht != nullptr)
    {
        Serial.println("About to convert pin to int.");
        int _pinDht = std::stoi(pinDht.c_str());
        dht = DHT(_pinDht, DHTTYPE); // Reinitialize the DHT object with the correct pin
        dht.begin();                 // Initialize the DHT sensor
    }
    else
    {
        Serial.println("Cannot configure DHT because pin not defined.");
    }
}

float roundToHundredth(float number)
{
    return round(number * 100.0) / 100.0;
}

void sensorTask(void *parameter)
{
    if (pinDht == nullptr)
    {
        Serial.println("DHT pin not configured in sensorTask!");
        vTaskDelete(NULL); // Delete the task if pin is not configured
        return;
    }

    configDHT();

    for (;;)
    {
        // Wait a few seconds between measurements
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds

        // Reading temperature and humidity
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();

        // Check if any reads failed and exit early (to try again).
        if (isnan(temp) || isnan(hum))
        {
            Serial.println("Failed to read from DHT sensor!");
            continue;
        }

        temp = roundToHundredth(temp);
        hum = roundToHundredth(hum);

        // Acquire the mutex before updating the shared variables
        if (xSemaphoreTake(xSensorDataMutex, portMAX_DELAY) == pdTRUE)
        {
            temperature = temp;
            humidity = hum;
            xSemaphoreGive(xSensorDataMutex); // Release the mutex
        }
    }
}

void initSensorTask()
{
    // Create the mutex
    xSensorDataMutex = xSemaphoreCreateMutex();
    if (xSensorDataMutex == NULL)
    {
        Serial.println("Mutex creation failed!");
    }

    // Create the task pinned to core 1
    xTaskCreatePinnedToCore(
        sensorTask,        // Task function
        "SensorTask",      // Name of the task
        4096,              // Stack size in words
        NULL,              // Task input parameter
        1,                 // Priority of the task
        &sensorTaskHandle, // Task handle
        1                  // Core where the task should run (1 or 0)
    );

    if (sensorTaskHandle == NULL)
    {
        Serial.println("Failed to create sensor task!");
    }
}

String floatToStringTwoDecimals(float value)
{
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    return String(buffer);
}

float readDHTTemperature()
{
    Serial.println("readDHTTemperature...");
    float temp = NAN;

    // Acquire the mutex before accessing the shared variable
    if (xSemaphoreTake(xSensorDataMutex, portMAX_DELAY) == pdTRUE)
    {
        temp = temperature;
        xSemaphoreGive(xSensorDataMutex); // Release the mutex
    }
    else
    {
        Serial.println("Failed to obtain mutex for temperature!");
    }

    return temp;
}

float readDHTHumidity()
{
    Serial.println("readDHTHumidity...");
    float hum = NAN;

    // Acquire the mutex before accessing the shared variable
    if (xSemaphoreTake(xSensorDataMutex, portMAX_DELAY) == pdTRUE)
    {
        hum = humidity;
        xSemaphoreGive(xSensorDataMutex); // Release the mutex
    }
    else
    {
        Serial.println("Failed to obtain mutex for humidity!");
    }

    return hum;
}
