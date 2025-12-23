#include "BootstrapConfig.h"

#include <ArduinoJson.h>

#include "ConfigModel.h"        // wherever AppConfig / gConfig are declared
#include "ConfigStorage.h"    // ConfigStorage::saveAppConfigToFile/load...
// #include "ConfigJson.h"       // configFromJson(...)
// #include "ConfigCaps.h"       // APP_CONFIG_JSON_CAPACITY (or where it lives)

extern AppConfig gConfig;     // or include the header that declares it

bool saveBootstrapConfigJson(const String &jsonBody, String &errOut)
{
    // 1) Parse JSON (sanity check)
    StaticJsonDocument<APP_CONFIG_JSON_CAPACITY> doc;
    DeserializationError err = deserializeJson(doc, jsonBody);
    if (err) {
        errOut = String("JSON parse error: ") + err.c_str();
        return false;
    }

    // 2) Apply into a temp AppConfig so we can validate without clobbering gConfig
    AppConfig tmp = gConfig; // start from current defaults
    JsonObject root = doc.as<JsonObject>();
    if (!configFromJson(root, tmp)) {
        errOut = "configFromJson failed";
        return false;
    }

    // 3) Minimal bootstrap validation
    if (tmp.wifi.ssid.isEmpty()) {
        errOut = "Missing required field: wifi.ssid";
        return false;
    }
    if (tmp.wifi.pass.isEmpty()) {
        errOut = "Missing required field: wifi.pass";
        return false;
    }
    if (tmp.identity.locationName.isEmpty()) {
        errOut = "Missing required field: identity.locationName";
        return false;
    }

    // Default instance to locationName if omitted
    if (tmp.identity.instance.isEmpty()) {
        tmp.identity.instance = tmp.identity.locationName;
    }

    // 4) Persist to /config.json in modular format
    if (!ConfigStorage::saveAppConfigToFile("/config.json", tmp)) {
        errOut = "Failed to write /config.json";
        return false;
    }

    // 5) Also update live gConfig
    gConfig = tmp;

    return true;
}
