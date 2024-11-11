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
    // {"main-delay", makePath(PARAM_MAIN_DELAY), ParamMetadata::NUMBER},
    {"main-delay", makePath(PARAM_MAIN_DELAY), ParamMetadata::STRING},
    {"w1-1", makePath(PARAM_W1_1), ParamMetadata::STRING}, // val is an array of hex and spiff is an array of w1-1 thru w1-3
    {"w1-2", makePath(PARAM_W1_2), ParamMetadata::STRING},
    {"w1-3", makePath(PARAM_W1_3), ParamMetadata::STRING},
    {"w1-1-name", makePath(PARAM_W1_1_NAME), ParamMetadata::STRING}, // val is a string and the array is the spiff set of w1-1-name thru w1-3-name
    {"w1-2-name", makePath(PARAM_W1_2_NAME), ParamMetadata::STRING},
    {"w1-3-name", makePath(PARAM_W1_3_NAME), ParamMetadata::STRING},
    {"enableW1", makePath(PARAM_ENABLE_W1), ParamMetadata::BOOLEAN},
    {"enableDHT", makePath(PARAM_ENABLE_DHT), ParamMetadata::BOOLEAN},
    {"enableMQTT", makePath(PARAM_ENABLE_MQTT), ParamMetadata::BOOLEAN}};
// {"w1NameValue", makePath("w1"), ParamMetadata::STRING}}; // this one is a work-in-progress. It doesn't
// really belong here, but I am looking for a way to combine form fields into a single w1 spiff file
// for saving the name/location of the w1 sensor and its hex address Nov9'24
