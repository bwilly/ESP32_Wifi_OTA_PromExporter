// In ParamMetadata.cpp
#include "shared_vars.h"
#include "SpiffsHandler.h"

#include <vector>

using namespace std; // This line allows you to use 'vector' instead of 'std::vector'

std::vector<ParamMetadata> paramList = {
    {"ssid", makePath(PARAM_WIFI_SSID), ParamMetadata::STRING},
    {"pass", makePath(PARAM_WIFI_PASS), ParamMetadata::STRING},
    {"location", makePath(PARAM_LOCATION), ParamMetadata::STRING},
    {"pinDht", makePath(PARAM_PIN_DHT), ParamMetadata::STRING},
    {"mqtt-server", makePath(PARAM_MQTT_SERVER), ParamMetadata::STRING},
    {"mqtt-port", makePath(PARAM_MQTT_PORT), ParamMetadata::STRING},
    {"main-delay", makePath(PARAM_MAIN_DELAY), ParamMetadata::NUMBER}, // Assuming this is a numeric value
    {"w1-1", makePath(PARAM_W1_1), ParamMetadata::STRING},             // or NUMBER if it is numeric
    {"w1-2", makePath(PARAM_W1_2), ParamMetadata::STRING},
    {"w1-3", makePath(PARAM_W1_3), ParamMetadata::STRING},
    {"w1-1-name", makePath(PARAM_W1_1_NAME), ParamMetadata::STRING},
    {"w1-2-name", makePath(PARAM_W1_2_NAME), ParamMetadata::STRING},
    {"w1-3-name", makePath(PARAM_W1_3_NAME), ParamMetadata::STRING},
    {"enableW1", makePath(PARAM_ENABLE_W1), ParamMetadata::BOOLEAN},
    {"enableDHT", makePath(PARAM_ENABLE_DHT), ParamMetadata::BOOLEAN},
    {"enableMQTT", makePath(PARAM_ENABLE_MQTT), ParamMetadata::BOOLEAN}};
