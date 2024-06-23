#include <algorithm>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/twai.h>
#include <driver/gpio.h>
#include <ESPmDNS.h>
#include <iomanip>
#include <iostream>
#include <list>
#include <LittleFS.h>
#include <Preferences.h>
#include <sstream>
#include <string>
#include <unordered_set>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

// Define program constants
#define DEVICE_NAME   "TeslaMars"
#define BAUDRATE      115200
#define LED_PIN       2
#define TX_PIN        21
#define RX_PIN        22
#define HTTP_PORT     80
#define WIFI_TIMEOUT  10000

// Web Server
AsyncWebServer server(HTTP_PORT);

// WebSocket Server
AsyncWebSocket ws("/ws");

// Preferences
Preferences prefs;

// WiFi Networks
struct WiFiInfo {
  String ssid;
  uint32_t rssi;

  WiFiInfo(const String& ssid, uint32_t rssi) : ssid(ssid), rssi(rssi) {}
};
std::list<WiFiInfo> wifiList;

// Setup Functions
void startFS();
void startWiFi();
void startWiFiAP();
void startWebSocket();
void startWebServer();
void startOTA();
void startTWAI();

// Loop Functions
void handleWiFiScan();
void handleWebSocket(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleTWAI();

void setup() {
  // Start Serial Interface
  Serial.begin(BAUDRATE);
  
  // Start File System (LittleFS + Preferences)
  startFS();
  
  // Start WiFi and run async scan
  startWiFi();
  
  // Start OTA Service
  startOTA();
  
  // TWAI (CAN) Bus Service
  startTWAI();
  
  // Start WebSocket
  startWebSocket();
  
  // Start Web Server
  startWebServer();
}

void loop() {  
  // Runs whenever there's a result of an async WiFi Scan
  handleWiFiScan();
  
  // Over The Air FS and FW
  ArduinoOTA.handle();
  
  // Websocket Handler
  ws.cleanupClients();

  // TWAI Handler
  handleTWAI();

  // Tasks to run at millis interval
  // timer.tick();
  // timer.every(1000, [](void *){ webSocket.broadcastPing(); return true; });
}

void startFS() {
  // Preferences is considered File System since it's persisted
  prefs.begin(DEVICE_NAME);
  
  // Begin LittleFS for WebServer
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

void startWiFi() {
  pinMode(LED_PIN, OUTPUT);
  
  // Get WiFi settings from Preferences
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  
  // If no SSID in preferences, use AP mode
  if (ssid.isEmpty()) {
    Serial.println("SSID is empty. Soft AP Mode.");
    startWiFiAP();
  } else {
    Serial.printf("Connecting to SSID '%s'\n", ssid);
    WiFi.begin(ssid, pass);
    
    // Connect to WiFi, timeout starts AP mode
    unsigned long now = millis();
    while(WiFi.status() != WL_CONNECTED) {
      if (millis() - now >= WIFI_TIMEOUT) {
        Serial.println("Failed to connect.");
        startWiFiAP();
        return;
      }
    }
    
    // Successfully connected. Print info.
    digitalWrite(LED_PIN, HIGH);
    auto ip = WiFi.localIP();
    Serial.printf("Connected to %s with IP: ", ssid);
    Serial.println(ip);
  }
}

void startWiFiAP() {
  WiFi.softAP(DEVICE_NAME);
  WiFi.scanNetworks(true);

  auto ip = WiFi.softAPIP();
  Serial.printf("Listening on AP Mode '%s' with IP: ", DEVICE_NAME);
  Serial.println(ip);
}

void startWebSocket() {
  // Start WS and register Event Handler
  ws.onEvent(handleWebSocket);
  server.addHandler(&ws);
  Serial.println("Web Socket registered");
}

void startWebServer() {
  // Default Web Page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  
  // Factory command clears preferences
  server.on("/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Factory Reset called.");
    request -> send(200, "text/html", "Resetting...");
    prefs.clear();
    sleep(500);
    ESP.restart(); });

  // Reboot command
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request -> send(200, "text/html", "ok");
    sleep(500);
    ESP.restart(); 
  });

  // Asynchronously scan for WiFi Networks
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    WiFi.scanNetworks(true);
    request->send(200, "text/html", "ok");
  });

  // Serves the results of the latest WiFi Network Scan as JSON
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Response Object
    String response;
    
    // JSON Marshaller Initialization - a bit odd, but works.
    JsonDocument net;
    JsonArray nets = net.to<JsonArray>();
    
    // Populate the JSON array
    for (auto wifi : wifiList) {
      JsonDocument doc;
      doc["ssid"] = wifi.ssid;
      doc["rssi"] = wifi.rssi;
      nets.add(doc);
    }
    
    // Serialize the JSON array of WiFi Networks into response
    serializeJson(nets, response);

    // Serve Networks as JSON
    request->send(200, "application/json", response);
  });

  // Endpoint to handle a new WiFi connection
  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Save SSID and Password to Preferences and reboot
    prefs.putString("ssid", request->arg("ssid"));
    prefs.putString("pass", request->arg("pass"));

    // Respond, sleep, reboot
    // No point in doing it any other way, since the connection will be broken
    request->send(200, "text/html", "Connecting...");
    sleep(500);
    ESP.restart();
  });

  // Lastly, serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();
  Serial.println("Web Server started");
}

void startOTA() {
  // Set Device Name for Flashing
  ArduinoOTA.setHostname(DEVICE_NAME);
  
  // Start configuration, copied from documentation
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  }).onEnd([]() {
    Serial.println("End");
  }).onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  }).onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  // Start OTA
  ArduinoOTA.begin();
  Serial.println("Arduino OTA started");
}

void startTWAI() {
  twai_general_config_t general_config = {
      .mode = TWAI_MODE_NORMAL,
      .tx_io = (gpio_num_t)TX_PIN,
      .rx_io = (gpio_num_t)RX_PIN,
      .clkout_io = (gpio_num_t)TWAI_IO_UNUSED,
      .bus_off_io = (gpio_num_t)TWAI_IO_UNUSED,
      .tx_queue_len = 10,
      .rx_queue_len = 10,
      .alerts_enabled = TWAI_ALERT_ALL,
      .clkout_divider = 0
  };

  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  esp_err_t error;

  error = twai_driver_install(&general_config, &timing_config, &filter_config);

  if(error == ESP_OK) {
      Serial.println("TWAI Driver Installation OK");
  } else {
      Serial.println("TWAI Driver Installation Failed");
  }

  error = twai_start();

  if(error == ESP_OK) {
      Serial.println("TWAI Driver Start OK");
  } else {
      Serial.println("TWAI Driver Start Failed");
  }
}

void handleWiFiScan() {
  // Read number of found networks
  int networks = WiFi.scanComplete();
  
  // 0 = none, -1 = in progress, -2 = not started
  if (networks > 0) {
    // Reset the list
    wifiList.clear();
    Serial.printf(PSTR("%d networks found.\n"), networks);

    // SSID variables
    String ssid;
    uint8_t encryptionType;
    int32_t rssi;
    uint8_t *bssid;
    int32_t channel;

    // Populate the list with found WiFi networks
    for (auto i = 0; i < networks; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel);
      wifiList.push_back(WiFiInfo(ssid, rssi));
      yield();
    }
    
    // Clear the scan results
    WiFi.scanDelete();
  }
}

void handleWebSocket(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      Serial.println("Handling event data...");
      [](void *arg, uint8_t *data, size_t len) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
          data[len] = 0;
          char *payload = (char *)data;
          int numData = atoi(payload);

          // Handle WS Message
          if (strcmp(payload, "reboot") == 0) {
            Serial.println("Restarting due to WS command");
            ws.textAll("Restarting due to WS command...");
          
            sleep(500);
            ESP.restart();
            return;
          }

          if (numData > 0) {
            ws.printfAll("(int) %d", numData);
            std::ostringstream right;
            right << std::right << std::setw(3) << numData;
            Serial.println(right.str().c_str());
          } else {
            Serial.println(payload);
            ws.textAll(payload);
          }
        }
      }(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleTWAI() {
  twai_message_t rx_frame;

  if (twai_receive(&rx_frame, pdMS_TO_TICKS(1000)) == ESP_OK) {
    std::ostringstream message;

    auto id = rx_frame.identifier;
    if (/*!shouldIgnore(id)*/ true) {
      message << "(0x" << std::hex << id << "): ";
      for (int i = 0; i < sizeof(rx_frame.data); i++) { //<< std::right << std::setw(4) << payload;
        message << std::right << std::setw(3) << rx_frame.data[i];
        if (i < sizeof(rx_frame.data) - 1) {
          message << " | ";
        }
      }
      ws.textAll(message.str().c_str());
    }
  }
}
