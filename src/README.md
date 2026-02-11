# MQTT RFID Reader/Display - ESP32 Version

Complete MQTT-enabled RFID reader with TFT display for ESP32.

## Version 1.0.15 - Production Ready

Optimized for RFID tag range testing with fast, responsive scanning and flicker-free display updates.

**Key Features:**
- **Fast scanning**: 250ms scan interval (4 scans/second)
- **Responsive display**: 500ms update rate with selective region updates (no flicker)
- **Quick detection**: Tag presence/removal detected within 500ms-1000ms
- **Modular architecture**: Clean separation of concerns across focused modules
- **Web configuration**: Easy setup via browser interface
- **MQTT integration**: Real-time tag event publishing
- **Security**: WiFi password protection, documented security considerations

**Core Modules:**
- `MQTTTagReaderDisplay_ESP32.ino` - Main application & coordination
- `nfc_reader.cpp/h` - PN5180 NFC interface (ISO15693 only)
- `display.cpp/h` - ILI9341 TFT display management (flicker-free updates)
- `web_server.cpp/h` - HTTP interface & configuration pages
- `mqtt_handler.cpp/h` - MQTT publishing & subscription
- `ILI9341_Landscape.h` - Custom landscape display class

## Hardware Requirements

- **ESP32** (WROOM-32 or compatible DevKit board)
- **PN5180 NFC Reader** (ISO15693/SLIX2 compatible)
- **ILI9341 TFT Display** (240x320, 2.8" or 3.2")
- Jumper wires
- USB cable for programming
- 3.3V and 5V power (from ESP32)


## Wiring

### PN5180 NFC Reader
```
PN5180 Pin    ESP32 Pin      Wire Color
-------------------------------------------
NSS (CS)   →  GPIO 5  (D5)   Orange
BUSY       →  GPIO 21 (D21)  Blue
RST        →  GPIO 22 (D22)  White
MOSI       →  GPIO 23 (D23)  Green
MISO       →  GPIO 19 (D19)  Purple
SCK        →  GPIO 18 (D18)  Brown
3.3V       →  3V3            Red
5V         →  VIN            ** CRITICAL! **
GND        →  GND            Black
```

**IMPORTANT:** The PN5180 needs **5V on the TVDD pin** (via the 5V/VIN connection) for the RF transmitter to work properly. The 3.3V connection powers the digital section.


For more information see: https://easyelecmodule.com/pn5180-explained-the-ultimate-nfc-module-with-full-protocol-compatibility/




### ILI9341 TFT Display
```
Display Pin   ESP32 Pin
---------------------------
CS         →  GPIO 15 (D15)
RST        →  GPIO 4  (D4)
DC         →  GPIO 2  (D2)
MOSI       →  GPIO 23 (D23) - shared with PN5180
MISO       →  GPIO 19 (D19) - shared with PN5180
SCK        →  GPIO 18 (D18) - shared with PN5180
VCC        →  3V3
GND        →  GND
LED        →  3V3 (or PWM pin for brightness control)
```




## Library Requirements

Install these libraries via Arduino Library Manager:

1. **PN5180 Library** by tueddy
   - GitHub: https://github.com/tueddy/PN5180-Library
   - Download ZIP and install via: Sketch → Include Library → Add .ZIP Library

2. **Adafruit GFX Library**
   - Tools → Manage Libraries → Search "Adafruit GFX"

3. **Adafruit ILI9341**
   - Tools → Manage Libraries → Search "Adafruit ILI9341"

4. **PubSubClient** (MQTT)
   - Tools → Manage Libraries → Search "PubSubClient"

5. **ArduinoJson**
   - Tools → Manage Libraries → Search "ArduinoJson"

## Arduino IDE Setup

1. **Install ESP32 Board Support:**
   - File → Preferences
   - Additional Board Manager URLs: 
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "ESP32" → Install "ESP32 by Espressif Systems"

2. **Select Board:**
   - Tools → Board → ESP32 Arduino → "ESP32 Dev Module"

3. **Configure Upload Settings:**
   - Upload Speed: 921600
   - Flash Frequency: 80MHz
   - Partition Scheme: "Default 4MB with spiffs"

## Configuration

### First Boot

1. Power on the ESP32
2. Look for WiFi network named `ESP32-RFID-ReaderDisplay`
3. Connect to it with your phone/computer
4. Browser will open automatically, or go to: `192.168.4.1`
5. Enter your WiFi credentials and MQTT broker details

### Web Interface

After WiFi is configured, access the web interface at: `http://[device-ip]/`

**Pages:**
- `/` - Main status page (tag detection, statistics, MQTT history)
- `/config` - Configuration page (WiFi password not exposed)
- `/status` - JSON API endpoint

### Configuration Options

**WiFi Settings:**
- SSID: Your WiFi network name
- Password: WiFi password (stored securely, not displayed in web interface)

**MQTT Settings:**
- Broker: IP address of your MQTT broker
- Port: Usually 1883
- Base Topic: Base topic for all messages (default: `rfid`)
- Subscribe Topic: Topic pattern to receive messages (default: `rfid/#`)
- Sensor ID: Unique ID for this reader (1-255)

**Topics Published:**
- `[base]/Read` - When tag is first detected
- `[base]/Unread` - When tag is removed
- `[base]/Continuing` - Published once when tag remains present (after 3 seconds)

**Message Format (JSON - shortened field names):**
```json
{
  "u": "E004010918485391",
  "s": 33,
  "R": "R"
}
```
- `u` = UID
- `s` = sensor_id
- `R` = direction (R=Read, C=Continuing, U=Unread)

## Usage

### Normal Operation

1. Power on device
2. Display shows "Scanning..."
3. Place ISO15693 tag on PN5180 antenna
4. Display shows "TAG PRESENT" with UID
5. MQTT message published to configured topics
6. Remove tag to generate "Unread" event

### Display Information

**Upper Section - Local Tag Scanner:**
- Current tag status (Scanning... or Local Tag Read)
- Tag UID when present (16 characters)

**Middle Section - MQTT Broker Messages:**
- Last 4 MQTT messages from any sensor
- Color-coded by event type: Green (Read), Yellow (Continuing), Red (Unread)
- Shows sensor ID, UID, and direction

**Lower Section - Status:**
- Configuration URL
- PN5180 status, version, and protocol
- Scan statistics (Scans, OK, Fail counts with real-time updates)
- MQTT connection status and broker info
- Topic configuration

**Display Features:**
- Flicker-free updates (selective region redrawing)
- Fast refresh rate (500ms)
- Left-justified welcome screen
- Streamlined WiFi connection status

## Troubleshooting

### PN5180 Not Detecting Tags

**Check wiring:**
- Verify all SPI connections (MOSI, MISO, SCK)
- **Ensure 5V is connected to VIN** (CRITICAL!)
- NSS on GPIO 5
- BUSY on GPIO 21
- RST on GPIO 22

**Verify power:**
- Measure 3.3V on PN5180 3.3V pin
- Measure 5V on PN5180 5V pin
- Both should be present simultaneously

**Test tags:**
- Use ISO15693 compatible tags (SLIX, SLIX2, I-CODE, etc.)
- Some generic tags may not work
- Try multiple tags to rule out tag issues

### Display Not Working

- Check CS pin (GPIO 15)
- Verify DC pin (GPIO 2)
- Check RST pin (GPIO 4)
- Ensure shared SPI pins are correct
- Try different display rotation: `tft.setRotation(1)` or `tft.setRotation(3)`

### WiFi Connection Issues

- Device creates AP named `ESP32-RFID-ReaderDisplay`
- Connect and configure via 192.168.4.1
- Check credentials carefully
- 2.4GHz WiFi only (ESP32 doesn't support 5GHz)

### MQTT Not Connecting

- Verify broker IP address is correct
- Check port (default 1883)
- Ensure broker accepts connections from this IP
- Check firewall settings
- Monitor MQTT broker logs

### Compilation Errors

**"PN5180.h: No such file"**
- Install tueddy's PN5180 library from GitHub

**"Adafruit_ILI9341.h: No such file"**
- Install Adafruit ILI9341 library via Library Manager

**"PubSubClient.h: No such file"**
- Install PubSubClient library via Library Manager

## Serial Monitor Debug

Connect via USB and open Serial Monitor (115200 baud) to see:
- Initialization status
- Tag detection events
- MQTT connection status
- Configuration details
- Error messages

## Performance

- **Scan Rate:** 250ms per scan (4 scans/second)
- **Tag Detection:** ~500ms (requires 2 consecutive reads for noise filtering)
- **Tag Removal:** ~1000ms timeout (4 missed scans)
- **Display Update:** Every 500ms (flicker-free selective updates)
- **MQTT Publish:** <100ms per message

**Optimized for Range Testing:**
The fast scan and display rates make this ideal for sliding RFID tags on a jig to determine detection range boundaries with precision.

## Known Issues & Limitations

1. **PN5180 requires 5V on TVDD** - Without this, RF transmitter won't work
2. **ISO15693 only** - ISO14443 (MIFARE) tags not currently supported
3. **Single tag detection** - Anti-collision not implemented
4. **No MQTT encryption** - Messages sent in plain text (see SECURITY.md)
5. **No web authentication** - Web interface accessible to anyone on network (see SECURITY.md)

**See SECURITY.md for security considerations and mitigation strategies.**

## Technical Details

### Critical PN5180 Setup

The PN5180 initialization sequence is critical:

```cpp
nfc.begin();        // Initialize SPI
nfc.reset();        // Reset chip
nfc.setupRF();      // Configure for ISO15693

// Then in loop:
nfc.getInventory(uid);  // Just scan, no reset/setupRF!
```

**DO NOT** call `reset()` or `setupRF()` in the loop - only in setup()!

### Power Requirements

- ESP32: ~240mA typical
- PN5180: ~150mA during RF transmission (from 5V rail)
- Display: ~100mA with backlight
- **Total:** ~500mA minimum

Use a quality USB power supply capable of delivering at least 1A.

## Important Files

- **README.md** - This file (setup, wiring, troubleshooting)
- **SECURITY.md** - Security considerations and best practices
- **REFACTORING_SUMMARY.md** - Details on modular architecture

## Support

For issues, check:
1. **README.md** - Wiring diagram and basic troubleshooting
2. **SECURITY.md** - Security considerations for your deployment
3. Serial Monitor output (115200 baud) for error messages
4. Web interface `/status` endpoint for diagnostics

**Common Issues:**
- Tags not detected → Check 5V connection to PN5180 VIN
- Display flickering → Already fixed in v1.0.5+
- Slow response → Update to v1.0.8+ for fast scanning
- WiFi password exposed → Fixed in v1.0.15

## License

MIT License - Free to use and modify

## Version History

### 1.0.15 - Security & Final Polish (Current)
- **Security Fix:** WiFi password no longer exposed in web interface HTML
- Added SECURITY.md with vulnerability assessment and mitigation strategies
- Password field now uses placeholder instead of displaying actual password

### 1.0.14 - MQTT Optimization
- Fixed redundant "Continuing" messages - only published once per continuous detection
- Added last event tracking to prevent MQTT spam

### 1.0.13 - Continuing Messages
- Fixed callback logic to enable "Continuing" message publishing
- Callback now triggered on every scan when tag is present

### 1.0.12 - MQTT Display Updates
- Fixed MQTT history display not updating when buffer full
- Implemented sequence counter for reliable change detection
- MQTT messages now update display within 500ms

### 1.0.11 - Display Spacing
- Adjusted scan statistics number positioning (+6 pixels right)
- Improved visual spacing between colons and values

### 1.0.10 - Tag Removal Responsiveness
- Reduced TAG_TIMEOUT from 3000ms to 1000ms
- Tag removal now detected within 1 second (4 missed scans)
- Added comprehensive timing documentation

### 1.0.9 - Display Responsiveness
- Reduced display update interval from 2000ms to 500ms (4x faster)
- Display now updates every 500ms for immediate visual feedback
- Added timing configuration documentation

### 1.0.8 - Scan Speed Optimization
- Reduced scan interval from 500ms to 250ms (2x faster)
- Now scans 4 times per second for precise range testing

### 1.0.7 - Display Polish
- Streamlined WiFi boot sequence (single page, updates in place)
- Left-justified welcome screen for cleaner appearance
- Improved label positioning on scan statistics line

### 1.0.6 - Granular Display Updates
- Ultra-smooth scan statistics updates (numbers only, no flicker)
- Only redraws changed number fields, not entire status line

### 1.0.5 - Flicker-Free Display
- Implemented selective region updates (no more full screen redraws)
- Smart change detection for each display section
- Dramatically improved visual experience

### 1.0.4 - Spurious Detection Filter
- Implemented consecutive read requirement (2 reads minimum)
- Added robust filtering for false positive detections
- Eliminated ghost tag detection issues

### 1.0.3 - Ghost Tag Fix
- Fixed spurious tag readings when no tag present
- Enhanced UID validation (reject all-zeros and all-FF patterns)
- Added debounce logic for tag return detection

### 1.0.2 - Boot Stability
- Fixed spurious tag detection on boot with no tag present
- Added UID validation before processing

### 1.0.1 - Modular Architecture
- Separated web server functionality into web_server module
- Separated MQTT handling into mqtt_handler module
- Simplified main INO file for better maintainability
- ISO15693 single-protocol focus

### 1.0.0 - Initial Baseline
- ISO15693 scanning with PN5180
- Web interface with status and configuration
- MQTT publishing: Read, Continuing, Unread events
- Stable tag detection

