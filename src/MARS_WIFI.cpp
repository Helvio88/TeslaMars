#include <MARS_WIFI.h>

std::list<WiFiInfo> wifiList;

void startWiFi(const String ssid, const String pass, const int led, const int timeout, const String fallbackSSID) {
    
  pinMode(led, OUTPUT);
    
  // If no SSID
  if (ssid.isEmpty()) {
    Serial.println("SSID is empty. Soft AP Mode.");
    startWiFiAP(fallbackSSID);
  } else {
    Serial.printf("Connecting to SSID '%s'\n", ssid);
    WiFi.begin(ssid, pass);
    
    // Connect to WiFi, timeout starts AP mode
    unsigned long now = millis();
    while(WiFi.status() != WL_CONNECTED) {
      if (millis() - now >= timeout) {
        Serial.println("Failed to connect.");
        startWiFiAP(fallbackSSID);
        return;
      }
    }
    
    // Successfully connected. Print info.
    digitalWrite(led, HIGH);
    auto ip = WiFi.localIP();
    Serial.printf("Connected to %s with IP: ", ssid);
    Serial.println(ip);
  }
}

void startWiFiAP(const String ssid) {
  WiFi.softAP(ssid);
  WiFi.scanNetworks(true);

  auto ip = WiFi.softAPIP();
  Serial.printf("Listening on AP Mode '%s' with IP: ", ssid);
  Serial.println(ip);
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