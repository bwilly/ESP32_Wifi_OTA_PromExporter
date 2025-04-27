#include "MessagePublisher.h"
#include <Arduino.h>
#include <time.h>

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

    client.publish("TEMPERATURE_TOPIC", buffer);
}

void MessagePublisher::publishHumidity(PubSubClient &client, float humidity, const String &location) {
    const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(4);
    DynamicJsonDocument doc(capacity);

    JsonObject j = doc.createNestedObject();
    j["bn"] = location;
    j["n"] = "humidity";
    j["u"] = "%";
    j["v"] = humidity;
    j["ut"] = (int)time(nullptr);

    char buffer[256];
    serializeJson(doc, buffer);

    Serial.print("Publishing the following to msg broker: ");
    Serial.println(buffer);

    client.publish("HUMIDITY_TOPIC", buffer);
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

