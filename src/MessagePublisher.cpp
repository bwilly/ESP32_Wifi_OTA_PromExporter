#define MQTT_MAX_PACKET_SIZE 512
#define MQTT_DEBUG

#include "MessagePublisher.h"
#include <Arduino.h>
#include <time.h>
// #include <BufferedLogger.h>
#include <shared_vars.h>

namespace {
    constexpr char TEMPERATURE_TOPIC[] = "ship/temperature";
    constexpr char HUMIDITY_TOPIC[] = "ship/humidity";
    constexpr char PUMP_STATE_TOPIC[] = "ship/pump";
}


void MessagePublisher::publishTemperature(PubSubClient &client, float temperature, const String &location) {
    const size_t capacity = JSON_OBJECT_SIZE(10);
    DynamicJsonDocument doc(capacity);

    doc["bn"] = location;
    doc["n"] = "temperature";
    doc["u"] = "C";
    doc["v"] = temperature;
    doc["ut"] = (int)time(nullptr);

    char buffer[512];
    serializeJson(doc, buffer);

    Serial.print("Publishing the following to msg broker: ");
    Serial.println(buffer);

    if (!client.connected()) {
        logger.log("MQTT client disconnected before publish!");
  
    }

    bool ok = client.publish(TEMPERATURE_TOPIC, buffer);
    if (ok) {
        logger.log("Msg pub ok \n");
    } else {
        logger.log("Msg pub FAIL \n");
    }
}

void MessagePublisher::publishHumidity(PubSubClient &client, float humidity, const String &location) {
    // Same style/capacity as temperature
    const size_t capacity = JSON_OBJECT_SIZE(10);
    DynamicJsonDocument doc(capacity);

    doc["bn"] = location;
    doc["n"]  = "humidity";
    doc["u"]  = "%";
    doc["v"]  = humidity;
    doc["ut"] = (int)time(nullptr);

    char buffer[512];
    serializeJson(doc, buffer);

    Serial.print("Publishing the following to msg broker: ");
    Serial.println(buffer);

    if (!client.connected()) {
        logger.log("MQTT client disconnected before publish!");
        // you might want to return here, but I’ll leave behavior same as temp()
    }

    bool ok = client.publish(HUMIDITY_TOPIC, buffer);
    if (ok) {
        logger.log("Humidity msg pub ok \n");
    } else {
        logger.log("Humidity msg pub FAIL \n");
    }
}


// void MessagePublisher::publishPumpState(PubSubClient &client, bool isOn, const String &location) {
//     const size_t capacity = JSON_OBJECT_SIZE(10);
//     DynamicJsonDocument doc(capacity);

//     doc["bn"] = location;
//     doc["n"] = "pumpState";
//     doc["u"] = "bool";
//     doc["v"] = isOn ? 1 : 0;
//     doc["ut"] = (int)time(nullptr);

//     char buffer[256];
//     serializeJson(doc, buffer);

//     Serial.print("Publishing the following to msg broker: ");
//     Serial.println(buffer);

//     client.publish(PUMP_STATE_TOPIC, buffer);
// }

void MessagePublisher::publishPumpState(PubSubClient &client, bool isOn, float amps, const String &location) {
    const size_t capacity = JSON_OBJECT_SIZE(12); // Increased slightly to allow amps
    DynamicJsonDocument doc(capacity);

    doc["bn"] = location;
    doc["n"] = "pumpState";
    doc["u"] = "bool";
    doc["v"] = isOn ? 1 : 0;
    doc["ut"] = (int)time(nullptr);
    doc["amps"] = amps;  // <-- New field!

    char buffer[256];
    serializeJson(doc, buffer);

    Serial.print("Publishing the following to msg broker: ");
    Serial.println(buffer);

    client.publish(PUMP_STATE_TOPIC, buffer);
}

