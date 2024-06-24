#ifndef MARS_WIFI
#define MARS_WIFI

#include <Arduino.h>
#include <list>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

// WiFi Networks
struct WiFiInfo {
  String ssid;
  uint32_t rssi;

  WiFiInfo(const String& ssid, uint32_t rssi) : ssid(ssid), rssi(rssi) {}
};
extern std::list<WiFiInfo> wifiList;

void startWiFi(const String ssid, const String pass, const int led, const int timeout, const String fallbackSSID);
void startWiFiAP(const String ssid);

void handleWiFiScan();

#endif
