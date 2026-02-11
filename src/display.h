/*
 * display.h
 * 
 * ILI9341 Display Module for ESP32
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Display pin definitions
#define TFT_CS   15  // GPIO15
#define TFT_RST  4   // GPIO4
#define TFT_DC   2   // GPIO2

// Color definitions (RGB565 - adjusted for BGR displays)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0x001F  // Swapped from 0xF800 for BGR display
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0xF800  // Swapped from 0x001F for BGR display
#define COLOR_CYAN    0xFFE0  // Swapped
#define COLOR_YELLOW  0x07FF  // Swapped
#define COLOR_ORANGE  0x051F  // Adjusted for BGR

// Initialize display
void initDisplay();

// MQTT message structure
struct MqttMessage {
  String uid;
  uint8_t sensor;
  char direction;  // R, C, or U
  unsigned long timestamp;
};

// Display welcome screen with version
void displayWelcome(const char* version, const char* buildDate);

// Display WiFi setup instructions
void displayWiFiSetup();

// Display IP address after WiFi connection
void displayIPAddress(const char* ssid, IPAddress ip);

// Display unified WiFi connection status (streamlined boot sequence)
void displayWiFiStatus(const char* ssid, IPAddress ip, bool connecting);

// Update display with current status
void updateDisplay();

// Display simple message
void displayMessage(const char* msg);

// Display tag information
void displayTag(const char* uid, bool present);

// Set MQTT connection status
void setMqttStatus(bool connected);

// Set MQTT broker configuration
void setMqttConfig(const char* broker, uint16_t port, const char* topic);

// Add MQTT message to display history
void addMqttMessage(const char* uid, uint8_t sensor, char direction);

// Get current tag info for web display
String getCurrentUID();
bool getCurrentTagPresent();
int getMqttHistoryCount();
MqttMessage getMqttHistoryItem(int index);

// Display status message
void displayStatus(const char* status);

#endif
