#include "Arduino.h"
#include "WIFI.h"

void startWiFi(const String ssid, const String pass, const int led, const int timeout) {
    
  pinMode(led, OUTPUT);
    
  // If no SSID
  if (ssid.isEmpty()) {
    Serial.println("SSID is empty. Soft AP Mode.");
    startWiFiAP();
  } else {
    Serial.printf("Connecting to SSID '%s'\n", ssid);
    WiFi.begin(ssid, pass);
    
    // Connect to WiFi, timeout starts AP mode
    unsigned long now = millis();
    while(WiFi.status() != WL_CONNECTED) {
      if (millis() - now >= timeout) {
        Serial.println("Failed to connect.");
        startWiFiAP();
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