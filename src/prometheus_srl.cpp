#include "DHT_SRL.h"
#include "shared_vars.h"
#include "prometheus_srl.h"

// Prometheus Exporter

char buffer[1024];

// *****************
//  Prometheus Exporter Format Dec9-2022
// *****************
// https://github.com/KireinaHoro/esp32-sensor-exporter/blob/master/src/main.cpp
//
// void send_metrics(WiFiClient &client) {
char *readAndGeneratePrometheusExport(const char *location)
{
    memset(buffer, 0, 1);

    strncat(buffer, "# HELP environ_tempt Environment temperature (in C).\n", 60);
    strncat(buffer, "# TYPE environ_tempt gauge\n", 30);

    strncat(buffer, "environ_tempt{location=\"", 50);
    strncat(buffer, location, 20);
    strncat(buffer, "\"}", 3);

    strncat(buffer, " ", 2);
    strncat(buffer, readDHTTemperature().c_str(), 6);
    strncat(buffer, "\n", 2);

    strncat(buffer, "# HELP environ_humidity Environment relative humidity (in percentage).\n", 99);
    strncat(buffer, "# TYPE environ_humidity gauge\n", 32);

    strncat(buffer, "environ_humidity{location=\"", 50);
    strncat(buffer, location, 20);
    strncat(buffer, "\"}", 3);

    strncat(buffer, " ", 2);
    strncat(buffer, readDHTHumidity().c_str(), 6);
    strncat(buffer, "\n", 3);

    return buffer;
}

char *buildPrometheusMultiTemptExport(TemperatureReading readings[MAX_READINGS])
{
    static char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    strncat(buffer, "# HELP environ_tempt Environment temperature (in C).\n", sizeof(buffer) - strlen(buffer) - 1);
    strncat(buffer, "# TYPE environ_tempt gauge\n", sizeof(buffer) - strlen(buffer) - 1);

    for (int i = 0; i < MAX_READINGS; i++)
    {
        if (readings[i].location.isEmpty())
            break;

        strncat(buffer, "environ_tempt{location=\"", sizeof(buffer) - strlen(buffer) - 1);
        strncat(buffer, readings[i].location.c_str(), sizeof(buffer) - strlen(buffer) - 1);
        strncat(buffer, "\"} ", sizeof(buffer) - strlen(buffer) - 1);

        char temperatureStr[8];
        snprintf(temperatureStr, sizeof(temperatureStr), "%.2f", readings[i].temperature);
        strncat(buffer, temperatureStr, sizeof(buffer) - strlen(buffer) - 1);
        strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
    }

    return buffer;
}