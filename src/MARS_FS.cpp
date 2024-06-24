#include <MARS_FS.h>

Preferences prefs;

void startFS(const char* device) {
  // Preferences is considered File System since it's persisted
  prefs.begin(device);
  
  // Begin LittleFS for WebServer
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

String getWiFiSSID() {
  return prefs.getString("ssid", "");
}

String getWiFiPass() {
  return prefs.getString("pass", "");
}

void setWiFiSSID(const String ssid) {
  prefs.putString("ssid", ssid);
}

void setWiFiPass(const String pass) {
  prefs.putString("pass", pass);
}
