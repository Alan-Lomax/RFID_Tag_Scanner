/*
 * web_server.cpp
 * 
 * Web Server Implementation
 */

#include "web_server.h"
#include "display.h"
#include "nfc_reader.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Module-level pointers
static WebServer* webServer = nullptr;
static Config* config = nullptr;
static PubSubClient* mqttClient = nullptr;
static uint32_t* mqttPublished = nullptr;
static void (*configSaveCallback)() = nullptr;

void setWebServerConfig(Config* cfg) {
  config = cfg;
}

void setWebServerMqttClient(void* client) {
  mqttClient = (PubSubClient*)client;
}

void setWebServerMqttPublished(uint32_t* counter) {
  mqttPublished = counter;
}

void setConfigSaveCallback(void (*callback)()) {
  configSaveCallback = callback;
}

void initWebServer(WebServer* server) {
  webServer = server;
  
  // Register handlers
  webServer->on("/", handleRoot);
  webServer->on("/config", HTTP_GET, handleConfig);
  webServer->on("/config", HTTP_POST, handleConfigSave);
  webServer->on("/status", handleStatus);
  
  webServer->begin();
  Serial.println(F("Web server started"));
}

void handleRoot() {
  if (!webServer || !config) return;
  
  NFCStatus nfcStatus = getNFCStatus();
  
  String html = F("<!DOCTYPE html><html><head><title>RFID Reader</title>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='2'>");
  html += F("<style>");
  html += F("body{font-family:monospace;margin:0;padding:0;background:#000;color:#0F0}");
  html += F(".container{max-width:800px;margin:0 auto;padding:10px}");
  html += F(".section{border:2px solid #0F0;margin:10px 0;padding:10px;background:#001100}");
  html += F(".section-upper{min-height:80px}");
  html += F(".section-middle{min-height:120px}");
  html += F(".section-lower{min-height:100px}");
  html += F("h2{color:#00FF00;margin:5px 0;font-size:20px;border-bottom:1px solid #0F0;padding-bottom:5px}");
  html += F(".local-tag{font-size:24px;color:#00FFFF;margin:10px 0;font-weight:bold}");
  html += F(".scanning{font-size:20px;color:#FFA500;margin:10px 0}");
  html += F(".mqtt-line{font-size:18px;margin:8px 0;padding:5px;border-left:3px solid #0F0}");
  html += F(".mqtt-first{position:relative;display:flex;justify-content:space-between;align-items:center}");
  html += F(".mqtt-first span{flex-grow:1}");
  html += F(".mqtt-read{color:#00FF00;border-left-color:#00FF00}");
  html += F(".mqtt-continue{color:#FFFF00;border-left-color:#FFFF00}");
  html += F(".mqtt-unread{color:#FF0000;border-left-color:#FF0000}");
  html += F(".copy-btn{background:#00AA00;color:#FFF;border:none;padding:4px 12px;cursor:pointer;");
  html += F("border-radius:3px;font-family:monospace;font-size:14px;margin-left:10px}");
  html += F(".copy-btn:hover{background:#45a049}");
  html += F(".copy-btn:active{background:#3d8b40}");
  html += F(".status-line{margin:6px 0;font-size:16px}");
  html += F(".status-ok{color:#00FF00}");
  html += F(".status-err{color:#FF0000}");
  html += F(".status-val{color:#00FF00}");
  html += F(".status-label{color:#FFFFFF}");
  html += F(".config-link{color:#FFFF00;text-decoration:none;font-size:18px}");
  html += F(".config-link:hover{text-decoration:underline}");
  html += F("</style>");
  html += F("<script>");
  html += F("function copyUID(uid){");
  html += F("var input=document.createElement('input');");
  html += F("input.style.position='fixed';");
  html += F("input.style.opacity='0';");
  html += F("input.value=uid;");
  html += F("document.body.appendChild(input);");
  html += F("input.select();");
  html += F("input.setSelectionRange(0,99999);");
  html += F("try{");
  html += F("document.execCommand('copy');");
  html += F("alert('UID copied: '+uid);");
  html += F("}catch(err){alert('Copy failed');}");
  html += F("document.body.removeChild(input);");
  html += F("}");
  html += F("</script>");
  html += F("</head><body>");
  
  html += F("<div class='container'>");
  
  // ===== UPPER SECTION - LOCAL TAG =====
  html += F("<div class='section section-upper'>");
  html += F("<h2>Local Tag Read:</h2>");
  String currentUID = getCurrentUID();
  bool currentTagPresent = getCurrentTagPresent();
  if (currentUID.length() > 0 && currentTagPresent) {
    html += F("<div class='local-tag'>");
    html += currentUID;
    html += F("</div>");
  } else {
    html += F("<div class='scanning'>Scanning...</div>");
  }
  html += F("</div>");
  
  // ===== MIDDLE SECTION - MQTT BROKER =====
  html += F("<div class='section section-middle'>");
  html += F("<h2>MQTT Broker:</h2>");
  
  int mqttHistoryCount = getMqttHistoryCount();
  for (int i = 0; i < mqttHistoryCount && i < 4; i++) {
    MqttMessage msg = getMqttHistoryItem(i);
    
    // First line gets special treatment with copy button
    if (i == 0) {
      html += F("<div class='mqtt-first mqtt-line ");
    } else {
      html += F("<div class='mqtt-line ");
    }
    
    if (msg.direction == 'R') html += F("mqtt-read");
    else if (msg.direction == 'C') html += F("mqtt-continue");
    else if (msg.direction == 'U') html += F("mqtt-unread");
    html += F("'>");
    
    // Message text
    if (i == 0) {
      html += F("<span>");
    }
    html += F("s:");
    html += String(msg.sensor);
    html += F(" ");
    html += msg.uid;
    html += F(" ");
    html += String(msg.direction);
    if (i == 0) {
      html += F("</span>");
      // Add copy button for first line
      html += F("<button class='copy-btn' onclick='copyUID(\"");
      html += msg.uid;
      html += F("\")'>[Copy]</button>");
    }
    
    html += F("</div>");
  }
  
  if (mqttHistoryCount == 0) {
    html += F("<div class='status-label'>No messages yet</div>");
  }
  html += F("</div>");
  
  // ===== LOWER SECTION - STATUS =====
  html += F("<div class='section section-lower'>");
  
  // Config line
  html += F("<div class='status-line'>");
  html += F("<span class='status-label'>Config  : </span>");
  html += F("<span class='config-link'>http://");
  html += WiFi.localIP().toString();
  html += F("</span></div>");
  
  // PN5180 status
  html += F("<div class='status-line'>");
  html += F("<span class='status-label'>PN5180 : </span>");
  if (nfcStatus.initialized) {
    html += F("<span class='status-ok'>OK</span>");
  } else {
    html += F("<span class='status-err'>FAIL</span>");
  }
  html += F("<span class='status-label'>  Ver : </span>");
  html += F("<span class='status-val'>");
  html += String(nfcStatus.productVersion / 10.0, 1);
  html += F("</span>");
  html += F("<span class='status-label'>  Protocol : </span>");
  html += F("<span class='status-val'>ISO15693</span></div>");
  
  // Scan statistics
  html += F("<div class='status-line'>");
  html += F("<span class='status-label'>Scans  : </span>");
  html += F("<span class='status-val'>");
  html += String(nfcStatus.totalScans);
  html += F("</span>");
  html += F("<span class='status-label'>  OK : </span>");
  html += F("<span class='status-val'>");
  html += String(nfcStatus.successfulReads);
  html += F("</span>");
  html += F("<span class='status-label'>  Fail : </span>");
  html += F("<span class='status-val'>");
  html += String(nfcStatus.failedReads);
  html += F("</span></div>");
  
  // MQTT status
  html += F("<div class='status-line'>");
  html += F("<span class='status-label'>MQTT   : </span>");
  if (mqttClient && mqttClient->connected()) {
    html += F("<span class='status-ok'>Connected</span>");
  } else {
    html += F("<span class='status-err'>Disconnected</span>");
  }
  html += F("<span class='status-label'>  URL : </span>");
  html += F("<span class='status-val'>");
  html += config->mqtt_broker;
  html += F(":");
  html += String(config->mqtt_port);
  html += F("</span></div>");
  
  // Topic
  html += F("<div class='status-line'>");
  html += F("<span class='status-label'>Topic  : </span>");
  html += F("<span class='status-val'>");
  html += config->mqtt_base_topic;
  html += F("/#</span></div>");
  
  html += F("</div>");  // End status section
  
  html += F("</div>");  // End container
  
  // Configuration link at bottom
  html += F("<div style='text-align:center;margin-top:20px'>");
  html += F("<a href='/config' class='config-link'>[Configuration]</a>");
  html += F("</div>");
  
  html += F("</body></html>");
  
  webServer->send(200, "text/html", html);
}

void handleConfig() {
  if (!webServer || !config) return;
  
  String html = F("<!DOCTYPE html><html><head><title>Configuration</title>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;background:#f0f0f0}");
  html += F(".card{background:white;padding:20px;margin:10px 0;border-radius:5px}");
  html += F("input{width:100%;padding:8px;margin:5px 0;box-sizing:border-box}");
  html += F("button{background:#4CAF50;color:white;padding:12px;border:none;border-radius:4px;cursor:pointer;width:100%}");
  html += F("</style></head><body>");
  
  html += F("<h1>Configuration</h1><form method='POST'>");
  html += F("<div class='card'><h2>WiFi</h2>");
  html += F("<label>SSID:</label><input name='ssid' value='"); html += config->wifi_ssid; html += F("'>");
  html += F("<label>Password:</label><input type='password' name='pass' placeholder='");
  if (strlen(config->wifi_password) > 0) {
    html += F("(password set - leave blank to keep current)");
  } else {
    html += F("Enter WiFi password");
  }
  html += F("'>");
  html += F("</div>");
  
  html += F("<div class='card'><h2>MQTT</h2>");
  html += F("<label>Broker:</label><input name='broker' value='"); html += config->mqtt_broker; html += F("'>");
  html += F("<label>Port:</label><input type='number' name='port' value='"); html += config->mqtt_port; html += F("'>");
  html += F("<label>Publish Base Topic:</label><input name='pub_topic' value='"); html += config->mqtt_base_topic; html += F("'>");
  html += F("<p style='font-size:12px;color:#666;margin:5px 0'>This node publishes to: [base]/Read, [base]/Continuing, [base]/Unread</p>");
  html += F("<label>Subscribe Topic:</label><input name='sub_topic' value='"); html += config->mqtt_subscribe_topic; html += F("'>");
  html += F("<p style='font-size:12px;color:#666;margin:5px 0'>Examples: rfid/# (all), rfid/Read (reads only), rfid/+ (one level)</p>");
  html += F("<label>Sensor ID:</label><input type='number' name='sensor' min='1' max='255' value='"); html += config->sensor_id; html += F("'>");
  html += F("</div>");
  
  html += F("<button type='submit'>Save & Reboot</button></form>");
  html += F("<p><a href='/'>[Back]</a></p></body></html>");
  
  webServer->send(200, "text/html", html);
}

void handleConfigSave() {
  if (!webServer || !config) return;
  
  if (webServer->hasArg("ssid")) strlcpy(config->wifi_ssid, webServer->arg("ssid").c_str(), sizeof(config->wifi_ssid));
  
  // Only update password if a new one is provided
  if (webServer->hasArg("pass") && webServer->arg("pass").length() > 0) {
    strlcpy(config->wifi_password, webServer->arg("pass").c_str(), sizeof(config->wifi_password));
  }
  
  if (webServer->hasArg("broker")) strlcpy(config->mqtt_broker, webServer->arg("broker").c_str(), sizeof(config->mqtt_broker));
  if (webServer->hasArg("port")) config->mqtt_port = webServer->arg("port").toInt();
  if (webServer->hasArg("pub_topic")) strlcpy(config->mqtt_base_topic, webServer->arg("pub_topic").c_str(), sizeof(config->mqtt_base_topic));
  if (webServer->hasArg("sub_topic")) strlcpy(config->mqtt_subscribe_topic, webServer->arg("sub_topic").c_str(), sizeof(config->mqtt_subscribe_topic));
  if (webServer->hasArg("sensor")) config->sensor_id = constrain(webServer->arg("sensor").toInt(), 1, 255);
  
  // Call save callback if registered
  if (configSaveCallback) {
    configSaveCallback();
  }
  
  String html = F("<!DOCTYPE html><html><head><title>Saved</title>");
  html += F("<meta http-equiv='refresh' content='3;url=/'></head><body>");
  html += F("<h1>Saved!</h1><p>Rebooting...</p></body></html>");
  
  webServer->send(200, "text/html", html);
  delay(1000);
  ESP.restart();
}

void handleStatus() {
  if (!webServer || !config) return;
  
  NFCStatus nfcStatus = getNFCStatus();
  
  StaticJsonDocument<512> doc;
  doc["version"] = "1.0.1";
  doc["uptime"] = millis();
  doc["nfc_initialized"] = nfcStatus.initialized;
  doc["nfc_version"] = nfcStatus.productVersion / 10.0;
  doc["total_scans"] = nfcStatus.totalScans;
  doc["successful_reads"] = nfcStatus.successfulReads;
  doc["failed_reads"] = nfcStatus.failedReads;
  if (mqttPublished) {
    doc["mqtt_published"] = *mqttPublished;
  }
  doc["wifi_ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  if (mqttClient) {
    doc["mqtt_connected"] = mqttClient->connected();
  }
  
  String json;
  serializeJson(doc, json);
  webServer->send(200, "application/json", json);
}
