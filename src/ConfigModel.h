// ConfigModel.h
#pragma once

#include <Arduino.h>

// High-level identity of the device and its role
struct IdentityConfig {
    String site;          // e.g. "salt"
    String instance;      // e.g. "stove"
    String locationName;  // used in metrics, mDNS suffix, etc.
};

// Wi-Fi configuration
struct WifiConfig {
    String ssid;
    String pass;
};

// Remote config / OTA endpoints
struct RemoteConfig {
    // Base URL for remote config (global/instance JSON)
    // e.g. "http://salt-r420:9080/esp-config/salt"
    String configBaseUrl;

    // OTA firmware URL
    // e.g. "http://salt-r420:9080/esp-config/salt/firmware.bin"
    String otaUrl;
};

// MQTT configuration
struct MqttConfig {
    bool   enabled = false;
    String server;        // e.g. "pi5d1"
    int    port   = 1883;
};

// DHT sensor configuration
struct DhtConfig {
    bool enabled = false;
    int  pin     = 23;    // default GPIO
};

struct Cht832xConfig {
    bool enabled = false;
    // int  pin     = 23;    // using 32 and 33 but #todo need to externalize
};


// DS18B20 / 1-Wire configuration
struct W1Config {
    bool enabled = false;
    int  busPin  = 23;    // default 1-Wire bus pin
};

// ACS712 current sensor configuration
struct Acs712Config {
    bool  enabled         = false;
    int   pin             = 36;     // ADC pin
    float onThresholdAmps = 1.75f;  // amps for "pump ON"
};

struct Sct013Config {
    bool  enabled         = false;
    // int   pin             = 36;     // ADC pin
    // float onThresholdAmps = 1.75f;  // amps for "pump ON"
};

// Aggregated sensors config
struct SensorsConfig {
    DhtConfig    dht;
    W1Config     w1;
    Acs712Config acs;
    Cht832xConfig cht;
    Sct013Config sct;
};

// Timing and intervals
struct TimingConfig {
    int mainDelayMs       = 3000;              // loop delay
    int publishIntervalMs = 5 * 60 * 1000;     // 5 minutes
};

// Top-level application configuration
struct AppConfig {
    IdentityConfig identity;
    WifiConfig     wifi;
    RemoteConfig   remote;
    MqttConfig     mqtt;
    SensorsConfig  sensors;
    TimingConfig   timing;
};

// Global config instance (defined in ConfigModel.cpp)
extern AppConfig gConfig;
