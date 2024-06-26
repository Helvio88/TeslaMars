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

std::vector<int> getIgnore() {
  // Create a JSON Array and populate with saved IDs
  JsonDocument arr;
  deserializeJson(arr, prefs.getString("ignore", "[]"));

  std::vector<int> ids = {};
  for (int i = 0; i < arr.size(); i++) {
    ids.push_back(arr[i]);
  }
  
  return ids;
}

void addIgnore(int id) {
  auto ids = getIgnore();
  ids.push_back(id);
  setIgnore(ids);
}

void delIgnore(int id) {
  auto ids = getIgnore();
  auto it = std::find(ids.begin(), ids.end(), id);
  if (it != ids.end()) {
    ids.erase(it);
  }
  setIgnore(ids);
}

void setIgnore(std::vector<int> ids) {
  String ignore;
  JsonDocument arr = JsonDocument().to<JsonArray>();
  for(int id : ids) {
    arr.add(id);
  }
  serializeJson(arr, ignore);
  prefs.putString("ignore", ignore);
}
