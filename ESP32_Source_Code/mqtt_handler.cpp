/*
 * mqtt_handler.cpp
 * 
 * MQTT Handler Implementation
 */

#include "mqtt_handler.h"
#include "display.h"
#include <ArduinoJson.h>

// Forward declare the Config structure
struct Config {
  char wifi_ssid[32];
  char wifi_password[64];
  char mqtt_broker[64];
  uint16_t mqtt_port;
  char mqtt_base_topic[64];
  char mqtt_subscribe_topic[64];
  uint8_t sensor_id;
};

// Module-level pointers
static PubSubClient* mqttClient = nullptr;
static Config* config = nullptr;
static uint32_t mqttPublished = 0;

void initMqttHandler(PubSubClient* client, Config* cfg) {
  mqttClient = client;
  config = cfg;
  
  if (mqttClient && config) {
    mqttClient->setServer(config->mqtt_broker, config->mqtt_port);
    mqttClient->setCallback(mqttCallback);
    Serial.println(F("MQTT handler initialized"));
  }
}

void reconnectMQTT() {
  if (!mqttClient || !config) return;
  
  if (mqttClient->connected()) return;
  
  Serial.print(F("Attempting MQTT connection to "));
  Serial.print(config->mqtt_broker);
  Serial.print(F(":"));
  Serial.print(config->mqtt_port);
  Serial.print(F("..."));
  
  // Create client ID
  String clientId = "ESP32-RFID-";
  clientId += String(config->sensor_id);
  
  if (mqttClient->connect(clientId.c_str())) {
    Serial.println(F("connected"));
    
    // Subscribe to configured topic
    if (strlen(config->mqtt_subscribe_topic) > 0) {
      mqttClient->subscribe(config->mqtt_subscribe_topic);
      Serial.print(F("Subscribed to: "));
      Serial.println(config->mqtt_subscribe_topic);
    }
    
    setMqttStatus(true);
  } else {
    Serial.print(F("failed, rc="));
    Serial.println(mqttClient->state());
    setMqttStatus(false);
  }
}

void publishTag(const char* uid, const char* event) {
  if (!mqttClient || !config) return;
  if (!mqttClient->connected()) return;
  
  String topic = String(config->mqtt_base_topic) + "/" + event;
  
  StaticJsonDocument<200> doc;
  doc["u"] = uid;          // UID (shortened)
  doc["s"] = config->sensor_id;  // Sensor ID (shortened)
  
  // Read direction: R=Read, C=Continuing, U=Unread
  if (strcmp(event, "Read") == 0) {
    doc["R"] = "R";
  } else if (strcmp(event, "Continuing") == 0) {
    doc["R"] = "C";
  } else if (strcmp(event, "Unread") == 0) {
    doc["R"] = "U";
  }
  
  String payload;
  serializeJson(doc, payload);
  
  if (mqttClient->publish(topic.c_str(), payload.c_str())) {
    mqttPublished++;
    Serial.print(F("MQTT: "));
    Serial.print(topic);
    Serial.print(F(" -> "));
    Serial.println(payload);
    
    // Don't add here - we'll receive it back via mqttCallback which avoids duplicates
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("MQTT received: "));
  Serial.print(topic);
  Serial.print(F(" -> "));
  
  // Parse JSON payload
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.println(F("JSON parse failed"));
    return;
  }
  
  // Extract fields - using shortened field names
  const char* uid = doc["u"];  // UID
  uint8_t sensor = doc["s"];   // Sensor ID
  const char* dirStr = doc["R"];  // Direction (R/C/U)
  
  if (!uid || !dirStr) {
    Serial.println(F("Missing required fields"));
    return;
  }
  
  char direction = dirStr[0];  // Get first character
  
  Serial.print(F("UID: "));
  Serial.print(uid);
  Serial.print(F(", Sensor: "));
  Serial.print(sensor);
  Serial.print(F(", Direction: "));
  Serial.println(direction);
  
  // Add to display history
  addMqttMessage(uid, sensor, direction);
}

uint32_t getMqttPublishCount() {
  return mqttPublished;
}
