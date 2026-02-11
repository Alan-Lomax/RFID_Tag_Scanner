/*
 * web_server.h
 * 
 * Web Server Interface for ESP32 RFID Reader
 * Handles HTTP requests, configuration pages, and status API
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

// Configuration structure (shared with main)
struct Config {
  char wifi_ssid[32];
  char wifi_password[64];
  char mqtt_broker[64];
  uint16_t mqtt_port;
  char mqtt_base_topic[64];
  char mqtt_subscribe_topic[64];
  uint8_t sensor_id;
};

// Initialize web server
void initWebServer(WebServer* server);

// Handler functions
void handleRoot();
void handleConfig();
void handleConfigSave();
void handleStatus();

// Set configuration pointer (so web server can access config)
void setWebServerConfig(Config* cfg);

// Set MQTT client pointer (for status reporting)
void setWebServerMqttClient(void* client);

// Set MQTT published count pointer
void setWebServerMqttPublished(uint32_t* counter);

// Set config save callback (called after config is saved via web)
void setConfigSaveCallback(void (*callback)());

#endif
