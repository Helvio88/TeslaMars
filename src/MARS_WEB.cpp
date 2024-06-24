#include <MARS_WEB.h>

void startWebServer(AsyncWebServer &server, AsyncWebSocket &ws) {
  ws.onEvent(handleWebSocket);
  server.addHandler(&ws);
  Serial.println("Web Socket registered");
  
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

void handleWebSocket(AsyncWebSocket *ws, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
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
          try {
            numData = std::stoul(payload, nullptr, 16);
            Serial.println(numData);
          } catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
          }

          // Handle WS Message
          if (strcmp(payload, "reboot") == 0) {
            Serial.println("Restarting due to WS command");
          
            sleep(500);
            ESP.restart();
            return;
          }
        }
      }(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}