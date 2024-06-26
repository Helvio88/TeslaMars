#include <MARS.h>

// Define program constants
#define DEVICE_NAME   "TeslaMars"
#define DEBOUNCE      500
#define BAUDRATE      115200
#define WIFI_TIMEOUT  10000
#define LED_PIN       2
#define TX_PIN        21
#define RX_PIN        22
#define HTTP_PORT     80
#define WS_PATH       "/ws"

// Debug Flags
#define DEBUG         false

// Timer for recurring tasks
auto timer = timer_create_default();
unsigned long previous = 0;

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
  
  // Simulate TWAI Frame ever 5s
  if (DEBUG) {
    timer.every(5 * 1000, [](void *none) {
      auto frame = simulateTWAIFrame();
      auto message = TWAI2JSON(frame);
      ws.textAll(message);
      return true;
    });
  }
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
  
  if (!DEBUG) {
    // Debounce to prevent spam
    unsigned long current = millis();
    if (current - previous >= DEBOUNCE) {
      // Debounce Logic
      previous = current;

      // TWAI Handler - Core of the App: Read the TWAI Bus
      auto frame = handleTWAI();

      // Proceed if a frame is received
      if (twai_receive(&frame, pdMS_TO_TICKS(1000)) == ESP_OK && !shouldIgnore(frame.identifier)) {
        String message = TWAI2JSON(frame);
        ws.textAll(message);
      }
    }
  }
}
