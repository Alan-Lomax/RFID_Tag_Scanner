/*
 * mqtt_handler.h
 * 
 * MQTT Interface for ESP32 RFID Reader
 * Handles MQTT connection, publishing, and subscriptions
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>

// Configuration structure (shared with main)
struct Config;

// Initialize MQTT handler
void initMqttHandler(PubSubClient* client, Config* cfg);

// MQTT reconnection
void reconnectMQTT();

// Publish tag event
void publishTag(const char* uid, const char* event);

// MQTT callback (internal)
void mqttCallback(char* topic, byte* payload, unsigned int length);

// Get MQTT publish counter
uint32_t getMqttPublishCount();

#endif
