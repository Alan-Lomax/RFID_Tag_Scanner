/*
 * nfc_reader.cpp
 * 
 * PN5180 NFC Reader Implementation
 */

#include "nfc_reader.h"
#include <PN5180.h>
#include <PN5180ISO15693.h>
#include <string.h>  // For memset

// Global objects
static PN5180ISO15693* nfc = nullptr;
static bool readerInitialized = false;
static NFCStatus nfcStatus = {false, false, 0, 0, 0, 0, 0, "Not initialized"};
static TagCallback tagCallback = nullptr;

// State tracking
static String lastUID = "";
static bool tagPresent = false;
static unsigned long lastTagTime = 0;
static unsigned long lastScanTime = 0;

// Debouncing - require consecutive reads
static String pendingUID = "";
static int consecutiveReads = 0;
#define REQUIRED_CONSECUTIVE_READS 2  // Must see tag 2 times in a row

// Tag timeout (tag considered gone after this period of no detection)
#define TAG_TIMEOUT 1000  // 1 second (4 scan cycles at 250ms interval)

bool initNFCReader() {
  Serial.println(F("\n=== PN5180 Initialization ==="));
  
  // Create PN5180 object
  if (nfc == nullptr) {
    nfc = new PN5180ISO15693(NFC_NSS_PIN, NFC_BUSY_PIN, NFC_RST_PIN);
  }
  
  // Initialize PN5180
  nfc->begin();
  delay(100);
  
  nfc->reset();
  delay(100);
  
  // Read product version to verify communication
  uint8_t productVersion[2];
  nfc->readEEprom(0x12, productVersion, sizeof(productVersion));
  
  Serial.print(F("PN5180 Version: "));
  Serial.print(productVersion[1]);
  Serial.print(F("."));
  Serial.println(productVersion[0]);
  
  // Check if we got valid data
  if (productVersion[1] == 0xFF && productVersion[0] == 0xFF) {
    Serial.println(F("ERROR: Cannot communicate with PN5180!"));
    strcpy(nfcStatus.lastError, "No SPI communication");
    nfcStatus.initialized = false;
    nfcStatus.rfActive = false;
    return false;
  }
  
  nfcStatus.productVersion = productVersion[1] * 10 + productVersion[0];
  
  // Setup RF for ISO15693 - CRITICAL: Only once!
  Serial.println(F("Setting up ISO15693 protocol..."));
  if (!nfc->setupRF()) {
    Serial.println(F("ERROR: setupRF failed!"));
    strcpy(nfcStatus.lastError, "setupRF failed");
    nfcStatus.initialized = false;
    nfcStatus.rfActive = false;
    return false;
  }
  
  nfcStatus.rfActive = true;
  nfcStatus.initialized = true;
  readerInitialized = true;
  strcpy(nfcStatus.lastError, "Scanning...");
  
  Serial.println(F("PN5180 ready for ISO15693 tags"));
  
  return true;
}

void processNFCReader() {
  if (!readerInitialized || nfc == nullptr) return;
  
  unsigned long now = millis();
  
  // Check for scan interval
  if (now - lastScanTime < SCAN_INTERVAL) {
    return;
  }
  
  lastScanTime = now;
  nfcStatus.totalScans++;
  
  // CRITICAL: Just call getInventory, no reset/setupRF!
  uint8_t uid[8];
  memset(uid, 0, sizeof(uid));  // Clear buffer before reading
  ISO15693ErrorCode rc = nfc->getInventory(uid);
  
  if (rc == ISO15693_EC_OK) {
    // Check if UID is valid (not all zeros or all 0xFF)
    bool validUID = false;
    bool allZeros = true;
    bool allFF = true;
    
    for (int i = 0; i < 8; i++) {
      if (uid[i] != 0x00) allZeros = false;
      if (uid[i] != 0xFF) allFF = false;
      if (uid[i] != 0x00 && uid[i] != 0xFF) {
        validUID = true;
      }
    }
    
    if (allZeros || allFF || !validUID) {
      // Invalid UID - treat as no tag
      nfcStatus.failedReads++;
      
      // Reset consecutive read counter
      consecutiveReads = 0;
      pendingUID = "";
      
      if (tagPresent && (now - lastTagTime > TAG_TIMEOUT)) {
        Serial.println(F("Tag removed (timeout)"));
        tagPresent = false;
        strcpy(nfcStatus.lastError, "Scanning...");
        if (tagCallback) {
          tagCallback(lastUID.c_str(), false);
        }
        lastUID = "";  // Clear UID
      }
      return;
    }
    
    // Tag detected with valid UID
    nfcStatus.successfulReads++;
    nfcStatus.lastSuccessTime = now;
    
    // Format UID as hex string (MSB first)
    String uidStr = "";
    for (int i = 7; i >= 0; i--) {
      if (uid[i] < 0x10) uidStr += "0";
      uidStr += String(uid[i], HEX);
    }
    uidStr.toUpperCase();
    
    // Check if this is a consecutive read of the same UID
    if (uidStr == pendingUID) {
      consecutiveReads++;
    } else {
      // Different UID or first read
      pendingUID = uidStr;
      consecutiveReads = 1;
    }
    
    // Only process tag if we've seen it REQUIRED_CONSECUTIVE_READS times
    if (consecutiveReads < REQUIRED_CONSECUTIVE_READS) {
      Serial.print(F("Pending read ("));
      Serial.print(consecutiveReads);
      Serial.print(F("/")); 
      Serial.print(REQUIRED_CONSECUTIVE_READS);
      Serial.print(F("): "));
      Serial.println(uidStr);
      return;  // Wait for more consecutive reads
    }
    
    if (uidStr != lastUID) {
      // New tag (confirmed by consecutive reads)
      Serial.print(F("Tag detected: "));
      Serial.println(uidStr);
      
      lastUID = uidStr;
      tagPresent = true;
      lastTagTime = now;
      
      strcpy(nfcStatus.lastError, "Tag present");
      
      // Trigger callback
      if (tagCallback) {
        tagCallback(uidStr.c_str(), true);
      }
      
    } else if (!tagPresent) {
      // Same tag returning after timeout (confirmed by consecutive reads)
      Serial.print(F("Tag returned: "));
      Serial.println(uidStr);
      
      tagPresent = true;
      lastTagTime = now;
      
      if (tagCallback) {
        tagCallback(uidStr.c_str(), true);
      }
      
    } else {
      // Same tag still present - update time and trigger callback
      lastTagTime = now;
      
      // Trigger callback to allow Continuing messages to be published
      if (tagCallback) {
        tagCallback(uidStr.c_str(), true);
      }
    }
    
  } else if (rc != 0x01) {
    // Error (0x01 is normal "no tag")
    nfcStatus.failedReads++;
    
    // Reset consecutive read counter
    consecutiveReads = 0;
    pendingUID = "";
    
    if (nfcStatus.successfulReads > 0 || nfcStatus.totalScans > 10) {
      strcpy(nfcStatus.lastError, "No tag in range");
    }
  }
  
  // Check for tag removal
  if (tagPresent && (now - lastTagTime > TAG_TIMEOUT)) {
    Serial.print(F("Tag removed: "));
    Serial.println(lastUID);
    
    tagPresent = false;
    strcpy(nfcStatus.lastError, "Scanning...");
    
    // Trigger callback
    if (tagCallback) {
      tagCallback(lastUID.c_str(), false);
    }
    
    // CRITICAL: Clear lastUID to prevent spurious re-detection
    lastUID = "";
  }
}

void setTagCallback(TagCallback callback) {
  tagCallback = callback;
}

NFCStatus getNFCStatus() {
  return nfcStatus;
}
