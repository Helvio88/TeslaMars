#include "TWAI_IDS.h"

#define PREFS_NAME "TWAI"

Preferences preferences;

std::vector<int> loadIDList(const String &key) {
    preferences.begin(PREFS_NAME, true);
    String intListString = preferences.getString(key.c_str(), "");
    preferences.end();

    std::vector<int> intList;
    if (intListString.length() > 0) {
        int start = 0;
        int end = intListString.indexOf(',');
        while (end != -1) {
            intList.push_back(intListString.substring(start, end).toInt());
            start = end + 1;
            end = intListString.indexOf(',', start);
        }
        intList.push_back(intListString.substring(start).toInt());
    }
    return intList;
}

void saveIDList(const String &key, const std::vector<int> &idList) {
    preferences.begin(PREFS_NAME, false);
    String idListString;
    for (size_t i = 0; i < idList.size(); ++i) {
        idListString += String(idList[i]);
        if (i < idList.size() - 1) {
            idListString += ",";
        }
    }
    preferences.putString(key.c_str(), idListString);
    preferences.end();
}

void addIDToList(const String &key, int newID) {
    std::vector<int> idList = loadIDList(key);
    idList.push_back(newID);
    saveIDList(key, idList);
}

void removeIDFromList(const String &key, int remID) {
    std::vector<int> idList = loadIDList(key);
    idList.erase(std::remove(idList.begin(), idList.end(), remID), idList.end());
    saveIDList(key, idList);
}
