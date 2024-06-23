#ifndef TWAI_IDS
#define TWAI_IDS

#include <Preferences.h>
#include <vector>
#include <algorithm>

std::vector<int> loadIDList(const String &key);
void saveIDList(const String &key, const std::vector<int> &idList);
void addIDToList(const String &key, int newID);
void removeIDFromList(const String &key, int remID);

#endif
