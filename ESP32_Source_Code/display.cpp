/*
 * display.cpp
 * 
 * ILI9341 Display Implementation
 */

#include "display.h"
#include "ILI9341_Landscape.h"
#include "nfc_reader.h"
#include <Adafruit_GFX.h>
#include <WiFi.h>

// Global display object - using custom landscape class
static ILI9341_Landscape tft = ILI9341_Landscape(TFT_CS, TFT_DC, TFT_RST);

// Display state
static String currentUID = "";
static bool currentTagPresent = false;
static bool mqttConnected = false;

// Previous state for change detection (flicker reduction)
static String prevUID = "";
static bool prevTagPresent = false;
static bool prevMqttConnected = false;
static uint32_t prevTotalScans = 0;
static uint32_t prevSuccessfulReads = 0;
static uint32_t prevFailedReads = 0;
static bool prevNfcInitialized = false;
static bool displayInitialized = false;

// MQTT message history (up to 4 messages)
static MqttMessage mqttHistory[4];  // Struct defined in display.h
static int mqttHistoryCount = 0;
static uint32_t mqttSequence = 0;  // Sequence number - increments on every new message
static uint32_t prevMqttSequence = 0;

// MQTT broker config
static String mqttBroker = "";
static uint16_t mqttPort = 1883;
static String mqttTopic = "";

void initDisplay() {
  Serial.println(F("Initializing ILI9341 display..."));
  
  tft.begin();
  tft.setRotation(0);  // Portrait - but custom class swaps to landscape
  tft.fillScreen(COLOR_BLACK);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  
  Serial.print(F("Display dimensions: "));
  Serial.print(tft.width());
  Serial.print(F(" x "));
  Serial.println(tft.height());
  
  Serial.println(F("Display initialized"));
}

void displayWelcome(const char* version, const char* buildDate) {
  tft.fillScreen(COLOR_BLACK);
  
  // Title - left justified
  tft.setTextSize(3);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(10, 40);
  tft.println("RFID");
  tft.setCursor(10, 75);
  tft.println("Reader");
  
  // Version - left justified
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 125);
  tft.print("Ver ");
  tft.println(version);
  
  // Build date - left justified
  tft.setTextSize(1);
  tft.setCursor(10, 150);
  tft.println(buildDate);
  
  // Platform - left justified
  tft.setCursor(10, 165);
  tft.setTextColor(COLOR_YELLOW);
  tft.println("ESP32 + PN5180");
}

void displayWiFiStatus(const char* ssid, IPAddress ip, bool connecting) {
  // Single page display for WiFi status - updates in place
  tft.fillScreen(COLOR_BLACK);
  
  if (connecting) {
    // Connecting state
    tft.setTextSize(2);
    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(10, 60);
    tft.println("WiFi Connecting...");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10, 100);
    tft.print("SSID: ");
    tft.println(ssid);
  } else {
    // Connected state - replace text on same page
    tft.setTextSize(2);
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(10, 60);
    tft.println("WiFi Connected!");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10, 100);
    tft.print("SSID: ");
    tft.println(ssid);
    
    tft.setCursor(10, 120);
    tft.setTextColor(COLOR_CYAN);
    tft.println("Configure via web browser:");
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(10, 145);
    tft.print("http://");
    tft.println(ip);
  }
}

void displayWiFiSetup() {
  tft.fillScreen(COLOR_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(10, 40);
  tft.println("WiFi Setup");
  tft.println("Required");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 100);
  tft.println("1. Connect phone/PC to:");
  tft.setTextColor(COLOR_CYAN);
  tft.println("   ESP32-RFID-ReaderDisplay");
  
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 140);
  tft.println("2. Browser opens automatically");
  tft.println("   or go to:");
  tft.setTextColor(COLOR_CYAN);
  tft.println("   http://192.168.4.1");
  
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 195);
  tft.println("3. Enter WiFi credentials");
}

void displayIPAddress(const char* ssid, IPAddress ip) {
  tft.fillScreen(COLOR_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(10, 60);
  tft.println("WiFi Connected!");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 100);
  tft.print("SSID: ");
  tft.println(ssid);
  
  tft.setCursor(10, 120);
  tft.setTextColor(COLOR_CYAN);
  tft.println("Configure via web browser:");
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(10, 145);
  tft.print("http://");
  tft.println(ip);
}

void displayMessage(const char* msg) {
  tft.fillScreen(COLOR_BLACK);
  tft.setCursor(10, 100);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE);
  tft.println(msg);
}

void displayStatus(const char* status) {
  // Large centered status message for initialization steps
  tft.fillScreen(COLOR_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_CYAN);
  
  // Simple centering
  int y = 100;
  tft.setCursor(10, y);
  tft.println(status);
}

void displayTag(const char* uid, bool present) {
  currentUID = String(uid);
  currentTagPresent = present;
  updateDisplay();
}

void setMqttStatus(bool connected) {
  mqttConnected = connected;
}

void setMqttConfig(const char* broker, uint16_t port, const char* topic) {
  mqttBroker = String(broker);
  mqttPort = port;
  mqttTopic = String(topic);
}

void addMqttMessage(const char* uid, uint8_t sensor, char direction) {
  // Shift history down (for 4 elements: shift 3->2, 2->1, 1->0)
  for (int i = 3; i > 0; i--) {
    mqttHistory[i] = mqttHistory[i-1];
  }
  
  // Add new message at top
  mqttHistory[0].uid = String(uid);
  mqttHistory[0].sensor = sensor;
  mqttHistory[0].direction = direction;
  mqttHistory[0].timestamp = millis();
  
  if (mqttHistoryCount < 4) mqttHistoryCount++;
  
  // Always increment sequence to trigger display update
  mqttSequence++;
  
  Serial.print(F("MQTT history count: "));
  Serial.print(mqttHistoryCount);
  Serial.print(F(", sequence: "));
  Serial.println(mqttSequence);
}

// Getter functions for web display
String getCurrentUID() {
  return currentUID;
}

bool getCurrentTagPresent() {
  return currentTagPresent;
}

int getMqttHistoryCount() {
  return mqttHistoryCount;
}

MqttMessage getMqttHistoryItem(int index) {
  if (index >= 0 && index < mqttHistoryCount) {
    return mqttHistory[index];
  }
  return MqttMessage{"", 0, ' ', 0};
}

// Helper function to clear a specific region (reduces flicker)
void clearRegion(int x, int y, int width, int height) {
  tft.fillRect(x, y, width, height, COLOR_BLACK);
}

// Helper function to update local tag area only
void updateLocalTagArea() {
  clearRegion(0, 0, 320, 60);  // Clear upper section
  
  tft.setCursor(0, 2);
  tft.setTextSize(2);
  
  if (currentTagPresent && currentUID.length() > 0 && currentUID != "0000000000000000") {
    tft.setTextColor(COLOR_GREEN);
    tft.println("Local Tag Read:");
    tft.setCursor(0, 22);
    tft.setTextColor(COLOR_CYAN);
    tft.setTextSize(2);
    tft.println(currentUID.substring(0, 16));
  } else {
    tft.setTextColor(COLOR_ORANGE);
    tft.println("Scanning...");
  }
  
  // Redraw separator
  tft.drawLine(0, 60, 320, 60, COLOR_WHITE);
}

// Helper function to update MQTT history area only
void updateMqttHistoryArea() {
  clearRegion(0, 65, 320, 115);  // Clear MQTT section (65 to 180)
  
  tft.setCursor(0, 65);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.println("MQTT Broker:");
  
  int y = 90;
  int displayCount = mqttHistoryCount;
  if (displayCount > 4) displayCount = 4;
  
  for (int i = 0; i < displayCount; i++) {
    tft.setCursor(0, y);
    tft.setTextSize(2);
    
    if (mqttHistory[i].direction == 'R') {
      tft.setTextColor(COLOR_GREEN);
    } else if (mqttHistory[i].direction == 'C') {
      tft.setTextColor(COLOR_YELLOW);
    } else if (mqttHistory[i].direction == 'U') {
      tft.setTextColor(COLOR_RED);
    } else {
      tft.setTextColor(COLOR_WHITE);
    }
    
    char line[25];
    snprintf(line, sizeof(line), "s:%d %s %c", 
             mqttHistory[i].sensor, 
             mqttHistory[i].uid.c_str(), 
             mqttHistory[i].direction);
    tft.print(line);
    y += 22;
  }
  
  // Redraw separator
  tft.drawLine(0, 180, 320, 180, COLOR_WHITE);
}

// Helper function to update only scan statistics numbers (no flicker)
void updateScanStats() {
  NFCStatus status = getNFCStatus();
  int statusY = 205;  // Y position of scan stats line
  
  // Clear only the number regions, not the labels
  // Scans number area (after ": " at x=44, number starts at x=50)
  tft.fillRect(50, statusY, 40, 8, COLOR_BLACK);
  // OK number area (after ": " at x=114, number starts at x=120)
  tft.fillRect(120, statusY, 60, 8, COLOR_BLACK);
  // Fail number area (after ": " at x=216, number starts at x=222)
  tft.fillRect(222, statusY, 100, 8, COLOR_BLACK);
  
  // Redraw just the numbers
  tft.setTextSize(1);
  
  // Scans value
  tft.setCursor(50, statusY);  // Moved from 44 to 50
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.totalScans);
  
  // OK value
  tft.setCursor(120, statusY);  // Moved from 114 to 120
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.successfulReads);
  
  // Fail value
  tft.setCursor(222, statusY);  // Moved from 216 to 222
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.failedReads);
}

// Helper function to update status area only
void updateStatusArea() {
  NFCStatus status = getNFCStatus();
  
  clearRegion(0, 185, 320, 55);  // Clear status section (185 to 240)
  
  tft.setTextSize(1);
  int statusY = 185;
  
  // Config line
  tft.setCursor(0, statusY);
  tft.setTextColor(COLOR_WHITE);
  tft.print("Config");
  tft.setCursor(38, statusY);
  tft.print(": ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(COLOR_YELLOW);
    tft.print("http://");
    tft.print(WiFi.localIP());
  } else {
    tft.setTextColor(COLOR_RED);
    tft.print("WiFi not connected");
  }
  statusY += 10;
  
  // PN5180 status
  tft.setCursor(0, statusY);
  tft.setTextColor(COLOR_WHITE);
  tft.print("PN5180");
  tft.setCursor(38, statusY);
  tft.print(": ");
  if (status.initialized) {
    tft.setTextColor(COLOR_GREEN);
    tft.print("OK");
  } else {
    tft.setTextColor(COLOR_RED);
    tft.print("FAIL");
  }
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(90, statusY);
  tft.print("Ver");
  tft.setCursor(108, statusY);
  tft.print(": ");
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.productVersion / 10.0, 1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(180, statusY);
  tft.print("Protocol");
  tft.setCursor(228, statusY);
  tft.print(": ");
  tft.setTextColor(COLOR_GREEN);
  tft.print("ISO15693");
  statusY += 10;
  
  // Scan statistics
  tft.setCursor(0, statusY);
  tft.setTextColor(COLOR_WHITE);
  tft.print("Scans");
  tft.setCursor(38, statusY);
  tft.print(": ");
  tft.setCursor(50, statusY);  // Position number 1 char right of natural position
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.totalScans);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(96, statusY);
  tft.print("OK");
  tft.setCursor(108, statusY);
  tft.print(": ");
  tft.setCursor(120, statusY);  // Position number 1 char right of natural position
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.successfulReads);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(186, statusY);
  tft.print("Fail");
  tft.setCursor(210, statusY);
  tft.print(": ");
  tft.setCursor(222, statusY);  // Position number 1 char right of natural position
  tft.setTextColor(COLOR_GREEN);
  tft.print(status.failedReads);
  statusY += 10;
  
  // MQTT status
  tft.setCursor(0, statusY);
  tft.setTextColor(COLOR_WHITE);
  tft.print("MQTT");
  tft.setCursor(38, statusY);
  tft.print(": ");
  if (mqttConnected) {
    tft.setTextColor(COLOR_GREEN);
    tft.print("Connected");
  } else {
    tft.setTextColor(COLOR_RED);
    tft.print("Disconnected");
  }
  if (mqttBroker.length() > 0) {
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(150, statusY);
    tft.print("URL");
    tft.setCursor(168, statusY);
    tft.print(": ");
    tft.setTextColor(COLOR_GREEN);
    tft.print(mqttBroker);
    tft.print(":");
    tft.print(mqttPort);
  }
  statusY += 10;
  
  // Topic line
  if (mqttTopic.length() > 0) {
    tft.setCursor(0, statusY);
    tft.setTextColor(COLOR_WHITE);
    tft.print("Topic");
    tft.setCursor(38, statusY);
    tft.print(": ");
    tft.setTextColor(COLOR_GREEN);
    tft.print(mqttTopic);
  }
}

void updateDisplay() {
  // Get NFC status
  NFCStatus status = getNFCStatus();
  
  // First time initialization - draw everything
  if (!displayInitialized) {
    tft.fillScreen(COLOR_BLACK);
    updateLocalTagArea();
    updateMqttHistoryArea();
    updateStatusArea();
    
    displayInitialized = true;
    prevUID = currentUID;
    prevTagPresent = currentTagPresent;
    prevMqttConnected = mqttConnected;
    prevMqttSequence = mqttSequence;
    prevTotalScans = status.totalScans;
    prevSuccessfulReads = status.successfulReads;
    prevFailedReads = status.failedReads;
    prevNfcInitialized = status.initialized;
    return;
  }
  
  // Check what changed and update only those regions
  bool localTagChanged = (currentUID != prevUID) || (currentTagPresent != prevTagPresent);
  bool mqttHistoryChanged = (mqttSequence != prevMqttSequence);  // Use sequence instead of count
  bool statsChanged = (status.totalScans != prevTotalScans) || 
                      (status.successfulReads != prevSuccessfulReads) ||
                      (status.failedReads != prevFailedReads);
  bool mqttStatusChanged = (mqttConnected != prevMqttConnected);
  bool nfcStatusChanged = (status.initialized != prevNfcInitialized);
  
  // Update only changed sections
  if (localTagChanged) {
    updateLocalTagArea();
    prevUID = currentUID;
    prevTagPresent = currentTagPresent;
  }
  
  if (mqttHistoryChanged) {
    updateMqttHistoryArea();
    prevMqttSequence = mqttSequence;  // Update sequence tracking
  }
  
  // Granular update for scan statistics
  if (statsChanged && !mqttStatusChanged && !nfcStatusChanged) {
    // Only stats changed - update just the numbers
    updateScanStats();
    prevTotalScans = status.totalScans;
    prevSuccessfulReads = status.successfulReads;
    prevFailedReads = status.failedReads;
  } else if (statsChanged || mqttStatusChanged || nfcStatusChanged) {
    // Other status items changed - full status area update
    updateStatusArea();
    prevTotalScans = status.totalScans;
    prevSuccessfulReads = status.successfulReads;
    prevFailedReads = status.failedReads;
    prevMqttConnected = mqttConnected;
    prevNfcInitialized = status.initialized;
  }
}
