#ifndef FS_PREFS
#define FS_PREFS

#include <LittleFS.h>
#include <Preferences.h>

void startFS(const char *device);
String getWiFiSSID();
String getWiFiPass();
void setWiFiSSID(const String ssid);
void setWiFiPass(const String ssid);

#endif
