#include "WebRoutes.h"

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>

// Your project headers / globals
#include "shared_vars.h"
#include "HtmlVarProcessor.h"
#include "ConfigDump.h"
#include "ConfigLoad.h"
#include "TemperatureReading.h"
#include "TemperatureSensor.h"
#include "BootstrapConfig.h"
#include "ConfigLoad.h"


// Things your routes reference (functions/globals defined elsewhere)
extern String version;

// From your main file / other modules:
// extern char *MakeMine(const char *NameTemplate);
extern String printDS18b20(void);
extern String SendHTML(TemperatureReading *readings, int maxReadings);
extern String buildPrometheusMultiTemptExport(TemperatureReading *readings);
extern const char *readAndGeneratePrometheusExport(const char *deviceName);

extern float readDHTTemperature();

// CHT sensor global you referenced (envSensor.read)
extern decltype(envSensor) envSensor;

// W1 / temp sensor globals referenced by routes
extern TemperatureSensor temptSensor;


// POST handler you’re retiring later (we’ll keep it for now unless you say remove)
extern void handlePostParameters(AsyncWebServerRequest *request);

// Bootstrapping JSON save function you already have
extern bool saveBootstrapConfigJson(const String &jsonBody, String &err);

// Cache helper you already have
extern bool clearConfigJsonCache(fs::FS &fs);

// OTA scheduling globals you already added
extern String g_otaUrl;
extern bool g_otaRequested;

// Legacy map still used by /ota/run currently
extern std::map<String, String *> paramToVariableMap;

// Your logger
// extern BufferedLogger logger;

// Your “processor” callback
extern String processor(const String &var);

// Your config cache path
// extern const char *EFFECTIVE_CACHE_PATH;


// ---- Internal helpers ----

static void registerBootstrapEndpoints(AsyncWebServer &server)
{
    // POST raw JSON bootstrap config (curl-friendly)
    server.on(
        "/config/bootstrap",
        HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Body handler responds; empty finalizer OK
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String body;


if (total > 4096) { request->send(413, "text/plain", "Too large"); return; }


            if (index == 0) {
  g_bootstrapBody = "";
  g_bootstrapBody.reserve(total);
  g_bootstrapErr = "";
}

g_bootstrapBody += String((char*)data, len);

if (index + len == total) {
  g_bootstrapPending = true;
  request->send(200, "text/plain", "Bootstrap received; applying shortly...");
}

        }
    );

    // Optional: View the effective cache file
    server.on("/config/effective-cache", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char *path = EFFECTIVE_CACHE_PATH;

        if (!SPIFFS.exists(path)) {
            request->send(404, "text/plain", "No effective cache file stored");
            return;
        }

        request->send(SPIFFS, path, "application/json");
    });

    // View the last downloaded remote snapshot: /config-remote.json
    server.on("/config/remote", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char *path = "/config-remote.json";

        if (!SPIFFS.exists(path)) {
            request->send(404, "text/plain", "No /config-remote.json stored");
            return;
        }

        request->send(SPIFFS, path, "application/json");
    });

    // Clear cache helper
    server.on("/config/cache/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool ok = clearConfigJsonCache(SPIFFS);
        if (ok) {
            request->send(200, "text/plain", "Config JSON cache cleared. It will not be used until remote config repopulates it.");
        } else {
            request->send(500, "text/plain", "Failed to clear config JSON cache.");
        }
    });

    // Device restart
    server.on("/device/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Restarting...");
        delay(300);
        ESP.restart();
    });
}

static void registerOtaEndpoint(AsyncWebServer &server)
{
    server.on("/ota/run", HTTP_GET, [](AsyncWebServerRequest *request) {

        auto it = paramToVariableMap.find("ota-url");
        if (it == paramToVariableMap.end() || it->second == nullptr) {
            request->send(400, "text/plain", "Missing or null 'ota-url' param in config");
            return;
        }

        String fwUrl = *(it->second);
        fwUrl.trim();

        if (fwUrl.length() == 0) {
            request->send(400, "text/plain", "Empty 'ota-url' value in config");
            return;
        }

        logger.log("OTA: requested via /ota/run, URL = " + fwUrl + "\n");

        // schedule OTA (run in loop() to avoid async_tcp WDT)
        g_otaUrl = fwUrl;
        g_otaRequested = true;

        request->send(200, "text/plain",
                      "OTA scheduled from " + fwUrl +
                      "\nDevice will reboot if update succeeds.");
    });
}


// ---- Public API ----

void registerWebRoutesStation(AsyncWebServer &server)
{
    logger.log("set web root /index.html...\n");

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");


    // Route to Prometheus Metrics Exporter
    server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", readAndGeneratePrometheusExport(locationName.c_str()));
    });

    server.on("/locationname", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", locationName);
    });

    server.on("/bssid", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", WiFi.BSSIDstr());
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[32];  // Buffer to hold the string representation of the temperature
        float temperature = readDHTTemperature();
        snprintf(buffer, sizeof(buffer), "%.2f", temperature);
        request->send(200, "text/html", buffer);
    });

    server.on("/cht/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[32];

        float chtTemp = NAN;
        float chtHum  = NAN;

        if (envSensor.read(chtTemp, chtHum)) {
            snprintf(buffer, sizeof(buffer), "%.2f", chtTemp);
            request->send(200, "text/html", buffer);
        } else {
            request->send(500, "text/plain", "CHT read failed");
        }
    });

    server.on("/cht/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[32];

        float chtTemp = NAN;
        float chtHum  = NAN;

        if (envSensor.read(chtTemp, chtHum)) {
            snprintf(buffer, sizeof(buffer), "%.2f", chtHum);
            request->send(200, "text/html", buffer);
        } else {
            request->send(500, "text/plain", "CHT read failed");
        }
    });

    // copy/paste from setup section for AP -- changing URL path
    // todo: consolidate this copied code
    server.on("/manage", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor);
    });

    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", version);
    });

    server.on("/onewire", HTTP_GET, [](AsyncWebServerRequest *request) {
        String result = printDS18b20();
        request->send(200, "text/html", result);
    });

    // todo: find out why some readings provide 129 now, and on prev commit, they returned -127 for same bad reading. Now, the method below return -127, but this one is now 129. Odd. Aug19 '23
    server.on("/onewiretempt", HTTP_GET, [](AsyncWebServerRequest *request) {
        temptSensor.requestTemperatures();
        TemperatureReading *readings = temptSensor.getTemperatureReadings();

        // Use the readings to send a response, assume SendHTML can handle TemperatureReading array
        request->send(200, "text/html", SendHTML(readings, MAX_READINGS));

        // sensors.requestTemperatures();
        // request->send(200, "text/html", SendHTML(sensors.getTempC(w1Address[0]), sensors.getTempC(w1Address[1]), sensors.getTempC(w1Address[2]))); });
        // request->send(200, "text/html", SendHTMLxxx());
    });

    // todo: find out why some readings provide -127
    server.on("/onewiremetrics", HTTP_GET, [](AsyncWebServerRequest *request) {
        temptSensor.requestTemperatures();
        TemperatureReading *readings = temptSensor.getTemperatureReadings();
        request->send(200, "text/html", buildPrometheusMultiTemptExport(readings));
    });

    // i believe this is the in-mem representation
    // server.on("/exportconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     String json = buildConfigJsonFlat();
    //     request->send(200, "application/json", json);
    // });

    // View the locally stored working config: /config.json
    server.on("/config/local", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char *path = "/config.json";

        if (!SPIFFS.exists(path)) {
            request->send(404, "text/plain", "No /config.json stored");
            return;
        }

        request->send(SPIFFS, path, "application/json");
    });

    // Bootstrap / cache endpoints
    registerBootstrapEndpoints(server);

    // OTA endpoint
    registerOtaEndpoint(server);

    // NOTE: legacy HTML POST handler remains for now.
    // You said you want to retire it; we’ll remove this later after bootstrap.json migration is in.
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        handlePostParameters(request);
        request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
        delay(mainDelay.toInt()); // delay(3000);
        ESP.restart();
    });
}

void registerWebRoutesAp(AsyncWebServer &server)
{
    // AP-mode helpers (cache + bootstrap + restart)
    registerBootstrapEndpoints(server);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor);
    });

    server.serveStatic("/", SPIFFS, "/"); // for things such as CSS

    // NOTE: legacy HTML POST handler remains for now.
    // You said you want JSON-only; we’ll remove this after bootstrap.json migration is solid.
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        handlePostParameters(request);
        request->send(200, "text/plain", "Done. ESP will restart, connect to your AP");
        delay(3000);
        logger.log("Updated. Now restarting...\n");
        ESP.restart();
    });
}
