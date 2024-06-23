#ifndef WIFI
#define WIFI

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

void startWiFi(const String ssid, const String pass, const int led, const int timeout);
void startWiFiAP();

#endif
