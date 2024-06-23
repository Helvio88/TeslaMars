#include <algorithm>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <iomanip>
#include <iostream>
#include <list>
#include <Preferences.h>
#include <sstream>
#include <string>
#include <unordered_set>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

// Own Includes
#include "fs/FS.h"
#include "twai/TWAI.h"
#include "twai/TWAI_IDS.h"
#include "ota/OTA.h"
#include "wifi/WIFI.h"

// Define program constants
#define DEVICE_NAME   "TeslaMars"
#define BAUDRATE      115200
#define WIFI_TIMEOUT  10000
#define LED_PIN       2
#define TX_PIN        21
#define RX_PIN        22
#define HTTP_PORT     80

// Web Server
AsyncWebServer server(HTTP_PORT);

// WebSocket Server
AsyncWebSocket ws("/ws");

// WiFi Networks
struct WiFiInfo {
  String ssid;
  uint32_t rssi;

  WiFiInfo(const String& ssid, uint32_t rssi) : ssid(ssid), rssi(rssi) {}
};
std::list<WiFiInfo> wifiList;

// Setup Functions
void startWebSocket();
void startWebServer();

// Loop Functions
void handleWiFiScan();
void handleWebSocket(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void setup() {
  // Start Serial Interface
  Serial.begin(BAUDRATE);
  
  // Start File System (LittleFS + Preferences)
  startFS(DEVICE_NAME);
  
  // Start WiFi
  startWiFi(getWiFiSSID(), getWiFiPass(), LED_PIN, WIFI_TIMEOUT);
  
  // Start OTA Service
  startOTA(DEVICE_NAME);
  
  // TWAI (CAN) Bus Service
  startTWAI(TX_PIN, RX_PIN);
  
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
  auto frame = handleTWAI();

  // Tasks to run at millis interval
  // timer.tick();
  // timer.every(1000, [](void *){ webSocket.broadcastPing(); return true; });
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
    setWiFiSSID(request->arg("ssid"));
    setWiFiPass(request->arg("pass"));

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
          unsigned int numData;
          try
          {
            numData = std::stoul(payload, nullptr, 16);
          }
          catch(const std::exception& e)
          {
            std::cerr << e.what() << '\n';
          }
          

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
