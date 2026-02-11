/*
 * nfc_reader.h
 * 
 * PN5180 NFC Reader Interface for ESP32
 * Handles ISO15693 tag detection
 */

#ifndef NFC_READER_H
#define NFC_READER_H

#include <Arduino.h>

// PN5180 Pin Definitions (NFC_ prefix to avoid library conflicts)
#define NFC_NSS_PIN  5   // GPIO5 - Chip Select
#define NFC_BUSY_PIN 21  // GPIO21 - Busy signal
#define NFC_RST_PIN  22  // GPIO22 - Reset

// Timing
#define SCAN_INTERVAL 250  // Scan every 250ms (optimized for range testing)

// NFC Status Structure
struct NFCStatus {
  bool initialized;
  bool rfActive;
  uint8_t productVersion;
  uint32_t totalScans;
  uint32_t successfulReads;
  uint32_t failedReads;
  unsigned long lastSuccessTime;
  char lastError[40];
};

// Callback function type for tag events
typedef void (*TagCallback)(const char* uid, bool present);

// Initialize NFC reader
bool initNFCReader();

// Process NFC reader (call in loop)
void processNFCReader();

// Set callback for tag events
void setTagCallback(TagCallback callback);

// Get NFC status
NFCStatus getNFCStatus();

#endif
