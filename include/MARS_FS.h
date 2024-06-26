#ifndef MARS_FS
#define MARS_FS

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <vector>

void startFS(const char *device);
String getWiFiSSID();
String getWiFiPass();
void setWiFiSSID(const String ssid);
void setWiFiPass(const String ssid);

std::vector<int> getIgnore();
void addIgnore(int id);
void delIgnore(int id);
void setIgnore(std::vector<int> ids);

#endif
