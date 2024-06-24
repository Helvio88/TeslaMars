#ifndef MARS_FS
#define MARS_FS

#include <LittleFS.h>
#include <Preferences.h>

void startFS(const char *device);
String getWiFiSSID();
String getWiFiPass();
void setWiFiSSID(const String ssid);
void setWiFiPass(const String ssid);

#endif
