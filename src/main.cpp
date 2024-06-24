#include <MARS.h>

// Define program constants
#define DEVICE_NAME   "TeslaMars"
#define BAUDRATE      115200
#define WIFI_TIMEOUT  10000
#define LED_PIN       2
#define TX_PIN        21
#define RX_PIN        22
#define HTTP_PORT     80
#define WS_PATH       "/ws"

// Timer object with up to 10 tasks
Timer<10> timer;

// Web Servers
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws(WS_PATH);

void setup() {
  // Start Serial Interface
  Serial.begin(BAUDRATE);
  
  // Start File System (LittleFS + Preferences)
  startFS(DEVICE_NAME);
  
  // Start WiFi
  startWiFi(getWiFiSSID(), getWiFiPass(), LED_PIN, WIFI_TIMEOUT, DEVICE_NAME);
  
  // Start OTA Service
  startOTA(DEVICE_NAME);
  
  // TWAI (CAN) Bus Service
  startTWAI(TX_PIN, RX_PIN);
    
  // Start Web Server
  startWebServer(server, ws);
}

void loop() {
  // Start the clock
  timer.tick();

  // Runs whenever there's a result of an async WiFi Scan
  handleWiFiScan();
  
  // Over The Air FS and FW
  ArduinoOTA.handle();
  
  // Websocket Handler
  ws.cleanupClients();
  
  // Simulate TWAI Frame
  timer.every(15000, [](void *none) {
    auto frame = simulateTWAIFrame();
    // Process the simulated message directly
    Serial.print("Simulated CAN ID: 0x");
    Serial.print(frame.identifier, HEX);
    Serial.print(" Data: ");
    for (int i = 0; i < frame.data_length_code; i++) {
      Serial.print("0x");
      Serial.print(frame.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // JSON Serializer for TWAI frame.
    JsonDocument twai;
    String message;

    twai["id"] = frame.identifier;
    
    for (int i = 0; i < sizeof(frame.data); i++) {
      twai["data"][i] = frame.data[i];
    }

    serializeJson(twai, message);

    ws.textAll(message);
    return true;
  });

  // TWAI Handler - Core of the App: Read the TWAI Bus
  auto frame = handleTWAI();

  // Proceed if a frame is received
  if (twai_receive(&frame, pdMS_TO_TICKS(1000)) == ESP_OK) {
    std::ostringstream message;

    auto id = frame.identifier;
    if (!false) {
      message << "(0x" << std::hex << id << "): ";
      for (int i = 0; i < sizeof(frame.data); i++) {
        message << std::right << std::setw(3) << std::dec << static_cast<int>(frame.data[i]);
        if (i < sizeof(frame.data) - 1) {
          message << " | ";
        }
      }
      ws.textAll(message.str().c_str());
    }
  }
}
