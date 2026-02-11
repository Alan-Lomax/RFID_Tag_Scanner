/*
 * MQTT RFID Reader/Display - ESP32 Version (Modular)
 * 
 * ISO15693 Scanner 
 * - Looks for a Valid UID response and displays it on Local display.
 * - Also publishes to Web Page
 * - All scans published to an MQTT broker which you define.
 * 
 * Hardware:
 * - ESP32 (WROOM-32 or compatible)
 * - PN5180 NFC Reader (ISO15693)
 * - ILI9341 TFT Display (240x320)
 * 
 * See README.md for complete wiring and setup instructions
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <SPI.h>
#include "nfc_reader.h"
#include "display.h"
#include "web_server.h"
#include "mqtt_handler.h"

// Version Information
#define VERSION "1.0.15"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define HOSTNAME "ESP32-RFID-ReaderDisplay"

// Timing Configuration
// =====================
// SCAN_INTERVAL (nfc_reader.h): 250ms = 4 scans/second
//   - Controls how often PN5180 checks for tags
//   - Lower = more responsive range detection
//   - Practical minimum: ~100ms (PN5180 RF field cycling)
//   - Consecutive reads required: 2 (noise filtering)
//
// TAG_TIMEOUT (nfc_reader.cpp): 1000ms
//   - How long after last detection before tag considered "removed"
//   - Should be 3-5x SCAN_INTERVAL for reliable detection
//   - Current: 4x scan interval = very responsive removal detection
//
// DISPLAY_UPDATE_INTERVAL: 500ms = 2 updates/second  
//   - Controls how often display refreshes
//   - With selective updates, can go much faster without flicker
//   - Practical minimum: ~100ms (human perception limit)
//   - Should be >= SCAN_INTERVAL for best responsiveness
//
// MQTT_RECONNECT_INTERVAL: 5000ms
//   - How often to retry MQTT connection if disconnected
//   - No need to change this
#define DISPLAY_UPDATE_INTERVAL 500
#define MQTT_RECONNECT_INTERVAL 5000

// Configuration
Config config;
Preferences preferences;

// Objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer webServer(80);

// State
unsigned long lastDisplayUpdate = 0;
unsigned long lastMqttReconnect = 0;
uint32_t mqttPublished = 0;
String lastPublishedUID = "";
String lastPublishedEvent = "";  // Track last event type (Read, Continuing, Unread)
unsigned long lastContinuingTime = 0;
#define CONTINUING_INTERVAL 3000  // Publish continuing every 3 seconds

// Forward declarations
void tagDetected(const char* uid, bool present);
void setupWiFi();
void loadConfig();
void saveConfig();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println(F("\n================================="));
  Serial.println(F("  MQTT RFID Reader/Display"));
  Serial.print(F("  Version: ")); Serial.println(VERSION);
  Serial.print(F("  Build: ")); Serial.print(BUILD_DATE); 
  Serial.print(F(" ")); Serial.println(BUILD_TIME);
  Serial.println(F("  Platform: ESP32"));
  Serial.println(F("================================="));
  
  // Load configuration
  loadConfig();
  
  // Initialize SPI (shared between PN5180 and display)
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS
  Serial.println(F("SPI initialized"));
  
  // Initialize Display
  Serial.println(F("\n=== Initializing Display ==="));
  initDisplay();
  
  // Show welcome screen with version
  displayWelcome(VERSION, BUILD_DATE);
  delay(3000);  // Show for 3 seconds
  
  displayMessage("Initializing...");
  
  // Setup WiFi
  setupWiFi();
  
  // Setup MQTT
  initMqttHandler(&mqttClient, &config);
  
  // Pass broker config to display
  setMqttConfig(config.mqtt_broker, config.mqtt_port, config.mqtt_subscribe_topic);
  
  // Setup Web Server
  setWebServerConfig(&config);
  setWebServerMqttClient(&mqttClient);
  setWebServerMqttPublished(&mqttPublished);
  setConfigSaveCallback(saveConfig);
  initWebServer(&webServer);
  
  // Initialize PN5180 - CRITICAL: Only once!
  Serial.println(F("\n=== Initializing PN5180 ==="));
  displayStatus("Init NFC...");
  
  if (initNFCReader()) {
    Serial.println(F("PN5180 initialized successfully"));
    setTagCallback(tagDetected);  // Set callback for tag events
  } else {
    Serial.println(F("PN5180 initialization failed"));
    displayMessage("NFC Init Failed!");
    delay(2000);
  }
  
  Serial.println(F("\n=== Setup Complete ==="));
  displayStatus("Ready!");
  updateDisplay();
}

void loop() {
  // Handle web server
  webServer.handleClient();
  
  // Process NFC reader (scans for tags)
  processNFCReader();
  
  // Handle MQTT connection
  if (!mqttClient.connected()) {
    setMqttStatus(false);
    unsigned long now = millis();
    if (now - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
      lastMqttReconnect = now;
      reconnectMQTT();
    }
  } else {
    setMqttStatus(true);
    mqttClient.loop();
  }
  
  // Update display periodically
  unsigned long now = millis();
  if (now - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = now;
    updateDisplay();
  }
  
  yield();
}

// Tag detection callback
void tagDetected(const char* uid, bool present) {
  // Update display
  displayTag(uid, present);
  
  // Publish MQTT event
  if (present) {
    if (String(uid) != lastPublishedUID) {
      // New tag - publish Read
      publishTag(uid, "Read");
      lastPublishedUID = String(uid);
      lastPublishedEvent = "Read";
      lastContinuingTime = millis();
      mqttPublished++;
    } else {
      // Same tag still present - check if we should publish Continuing
      unsigned long now = millis();
      if (now - lastContinuingTime >= CONTINUING_INTERVAL) {
        // Only publish Continuing if the last event was NOT already Continuing
        if (lastPublishedEvent != "Continuing") {
          publishTag(uid, "Continuing");
          lastPublishedEvent = "Continuing";
          lastContinuingTime = now;
          mqttPublished++;
        }
        // If last event was already Continuing, do nothing (no spam)
      }
    }
  } else {
    // Tag removed - publish Unread
    publishTag(uid, "Unread");
    lastPublishedUID = "";
    lastPublishedEvent = "Unread";
    mqttPublished++;
  }
}

// Configuration functions
void loadConfig() {
  preferences.begin("rfid-reader", false);
  
  strlcpy(config.wifi_ssid, preferences.getString("wifi_ssid", "").c_str(), sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, preferences.getString("wifi_pass", "").c_str(), sizeof(config.wifi_password));
  strlcpy(config.mqtt_broker, preferences.getString("mqtt_broker", "192.168.1.100").c_str(), sizeof(config.mqtt_broker));
  config.mqtt_port = preferences.getUInt("mqtt_port", 1883);
  strlcpy(config.mqtt_base_topic, preferences.getString("mqtt_pub_topic", "rfid").c_str(), sizeof(config.mqtt_base_topic));
  strlcpy(config.mqtt_subscribe_topic, preferences.getString("mqtt_sub_topic", "rfid/#").c_str(), sizeof(config.mqtt_subscribe_topic));
  config.sensor_id = preferences.getUChar("sensor_id", 33);
  
  preferences.end();
  
  Serial.println(F("\n=== Configuration ==="));
  Serial.print(F("MQTT: ")); Serial.print(config.mqtt_broker); 
  Serial.print(F(":")); Serial.println(config.mqtt_port);
  Serial.print(F("Publish Base: ")); Serial.println(config.mqtt_base_topic);
  Serial.print(F("Subscribe: ")); Serial.println(config.mqtt_subscribe_topic);
  Serial.print(F("Sensor ID: ")); Serial.println(config.sensor_id);
}

void saveConfig() {
  preferences.begin("rfid-reader", false);
  
  preferences.putString("wifi_ssid", config.wifi_ssid);
  preferences.putString("wifi_pass", config.wifi_password);
  preferences.putString("mqtt_broker", config.mqtt_broker);
  preferences.putUInt("mqtt_port", config.mqtt_port);
  preferences.putString("mqtt_pub_topic", config.mqtt_base_topic);
  preferences.putString("mqtt_sub_topic", config.mqtt_subscribe_topic);
  preferences.putUChar("sensor_id", config.sensor_id);
  
  preferences.end();
  Serial.println(F("Configuration saved"));
}

void setupWiFi() {
  Serial.println(F("\n=== WiFi Setup ==="));
  
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  
  if (strlen(config.wifi_ssid) > 0) {
    // Show connecting status
    displayWiFiStatus(config.wifi_ssid, IPAddress(0,0,0,0), true);
    
    Serial.print(F("Connecting to: "));
    Serial.println(config.wifi_ssid);
    
    WiFi.begin(config.wifi_ssid, config.wifi_password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Starting AP mode"));
    WiFi.softAP(HOSTNAME);
    
    displayMessage("WiFi AP Mode");
    delay(1000);
    
    displayWiFiSetup();
    delay(5000);  // Show instructions for 5 seconds
    
  } else {
    Serial.println(F("WiFi connected!"));
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    
    // Update same page with connected status
    displayWiFiStatus(WiFi.SSID().c_str(), WiFi.localIP(), false);
    delay(5000);  // Show IP for 5 seconds
  }
}
