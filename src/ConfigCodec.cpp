// ConfigCodec.cpp
#include "ConfigCodec.h"

// ---- helpers per section ----

static void identityFromJson(JsonObject obj, IdentityConfig &id) {
    if (!obj.isNull()) {
        if (obj.containsKey("site")) {
            id.site = obj["site"].as<const char*>();
        }
        if (obj.containsKey("instance")) {
            id.instance = obj["instance"].as<const char*>();
        }
        if (obj.containsKey("locationName")) {
            id.locationName = obj["locationName"].as<const char*>();
        }
    }
}

static void identityToJson(const IdentityConfig &id, JsonObject obj) {
    obj["site"]         = id.site;
    obj["instance"]     = id.instance;
    obj["locationName"] = id.locationName;
}

static void wifiFromJson(JsonObject obj, WifiConfig &wifi) {
    if (!obj.isNull()) {
        if (obj.containsKey("ssid")) {
            wifi.ssid = obj["ssid"].as<const char*>();
        }
        if (obj.containsKey("pass")) {
            wifi.pass = obj["pass"].as<const char*>();
        }
    }
}

static void wifiToJson(const WifiConfig &wifi, JsonObject obj) {
    obj["ssid"] = wifi.ssid;
    obj["pass"] = wifi.pass;
}

static void remoteFromJson(JsonObject obj, RemoteConfig &rc) {
    if (!obj.isNull()) {
        if (obj.containsKey("configBaseUrl")) {
            rc.configBaseUrl = obj["configBaseUrl"].as<const char*>();
        }
        if (obj.containsKey("otaUrl")) {
            rc.otaUrl = obj["otaUrl"].as<const char*>();
        }
    }
}

static void remoteToJson(const RemoteConfig &rc, JsonObject obj) {
    obj["configBaseUrl"] = rc.configBaseUrl;
    obj["otaUrl"]        = rc.otaUrl;
}

static void mqttFromJson(JsonObject obj, MqttConfig &mq) {
    if (!obj.isNull()) {
        if (obj.containsKey("enabled")) {
            mq.enabled = obj["enabled"].as<bool>();
        }
        if (obj.containsKey("server")) {
            mq.server = obj["server"].as<const char*>();
        }
        if (obj.containsKey("port")) {
            mq.port = obj["port"].as<int>();
        }
    }
}

static void mqttToJson(const MqttConfig &mq, JsonObject obj) {
    obj["enabled"] = mq.enabled;
    obj["server"]  = mq.server;
    obj["port"]    = mq.port;
}

static void sensorsFromJson(JsonObject obj, SensorsConfig &sc) {
    if (obj.isNull()) return;

    // DHT
    if (obj.containsKey("dht")) {
        JsonObject dht = obj["dht"].as<JsonObject>();
        if (dht.containsKey("enabled")) {
            sc.dht.enabled = dht["enabled"].as<bool>();
        }
        if (dht.containsKey("pin")) {
            sc.dht.pin = dht["pin"].as<int>();
        }
    }

    // W1
    if (obj.containsKey("w1")) {
        JsonObject w1 = obj["w1"].as<JsonObject>();
        if (w1.containsKey("enabled")) {
            sc.w1.enabled = w1["enabled"].as<bool>();
        }
        if (w1.containsKey("busPin")) {
            sc.w1.busPin = w1["busPin"].as<int>();
        }
    }

    // ACS712
    if (obj.containsKey("acs712")) {
        JsonObject a = obj["acs712"].as<JsonObject>();
        if (a.containsKey("enabled")) {
            sc.acs.enabled = a["enabled"].as<bool>();
        }
        if (a.containsKey("pin")) {
            sc.acs.pin = a["pin"].as<int>();
        }
        if (a.containsKey("onThresholdAmps")) {
            sc.acs.onThresholdAmps = a["onThresholdAmps"].as<float>();
        }
    }
}

static void sensorsToJson(const SensorsConfig &sc, JsonObject obj) {
    JsonObject dht = obj.createNestedObject("dht");
    dht["enabled"] = sc.dht.enabled;
    dht["pin"]     = sc.dht.pin;

    JsonObject w1 = obj.createNestedObject("w1");
    w1["enabled"] = sc.w1.enabled;
    w1["busPin"]  = sc.w1.busPin;

    JsonObject a  = obj.createNestedObject("acs712");
    a["enabled"]         = sc.acs.enabled;
    a["pin"]             = sc.acs.pin;
    a["onThresholdAmps"] = sc.acs.onThresholdAmps;
}

static void timingFromJson(JsonObject obj, TimingConfig &tc) {
    if (!obj.isNull()) {
        if (obj.containsKey("mainDelayMs")) {
            tc.mainDelayMs = obj["mainDelayMs"].as<int>();
        }
        if (obj.containsKey("publishIntervalMs")) {
            tc.publishIntervalMs = obj["publishIntervalMs"].as<int>();
        }
    }
}

static void timingToJson(const TimingConfig &tc, JsonObject obj) {
    obj["mainDelayMs"]       = tc.mainDelayMs;
    obj["publishIntervalMs"] = tc.publishIntervalMs;
}

// ---- top-level API ----

bool configFromJson(const JsonObject &root, AppConfig &cfg)
{
    // identity
    if (root.containsKey("identity")) {
        identityFromJson(root["identity"].as<JsonObject>(), cfg.identity);
    }

    // wifi
    if (root.containsKey("wifi")) {
        wifiFromJson(root["wifi"].as<JsonObject>(), cfg.wifi);
    }

    // remote
    if (root.containsKey("remote")) {
        remoteFromJson(root["remote"].as<JsonObject>(), cfg.remote);
    }

    // mqtt
    if (root.containsKey("mqtt")) {
        mqttFromJson(root["mqtt"].as<JsonObject>(), cfg.mqtt);
    }

    // sensors
    if (root.containsKey("sensors")) {
        sensorsFromJson(root["sensors"].as<JsonObject>(), cfg.sensors);
    }

    // timing
    if (root.containsKey("timing")) {
        timingFromJson(root["timing"].as<JsonObject>(), cfg.timing);
    }

    // You can enforce required bootstrap fields here later if you want.
    return true;
}

void configToJson(const AppConfig &cfg, JsonObject &root)
{
    JsonObject idObj = root.createNestedObject("identity");
    identityToJson(cfg.identity, idObj);

    JsonObject wifiObj = root.createNestedObject("wifi");
    wifiToJson(cfg.wifi, wifiObj);

    JsonObject remoteObj = root.createNestedObject("remote");
    remoteToJson(cfg.remote, remoteObj);

    JsonObject mqttObj = root.createNestedObject("mqtt");
    mqttToJson(cfg.mqtt, mqttObj);

    JsonObject sensorsObj = root.createNestedObject("sensors");
    sensorsToJson(cfg.sensors, sensorsObj);

    JsonObject timingObj = root.createNestedObject("timing");
    timingToJson(cfg.timing, timingObj);
}
