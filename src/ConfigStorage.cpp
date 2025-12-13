// ConfigStorage.cpp
#include "ConfigStorage.h"

namespace ConfigStorage {

    bool loadAppConfigFromFile(const char *path, AppConfig &cfg)
    {
        File f = SPIFFS.open(path, FILE_READ);
        if (!f) {
            // No file yet
            return false;
        }

        StaticJsonDocument<APP_CONFIG_JSON_CAPACITY> doc;
        DeserializationError err = deserializeJson(doc, f);
        f.close();

        if (err) {
            // Parse error
            return false;
        }

        JsonObject root = doc.as<JsonObject>();
        return configFromJson(root, cfg);
    }

    bool saveAppConfigToFile(const char *path, const AppConfig &cfg)
    {
        StaticJsonDocument<APP_CONFIG_JSON_CAPACITY> doc;
        JsonObject root = doc.to<JsonObject>();

        configToJson(cfg, root);

        File f = SPIFFS.open(path, FILE_WRITE);
        if (!f) {
            return false;
        }

        if (serializeJson(doc, f) == 0) {
            f.close();
            return false;
        }

        f.close();
        return true;
    }

} // namespace ConfigStorage
