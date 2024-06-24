#ifndef MARS_WEB
#define MARS_WEB

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <driver/twai.h>
#include <MARS_FS.h>
#include <MARS_WIFI.h>

void startWebServer(AsyncWebServer &server, AsyncWebSocket &ws);
void handleWebSocket(AsyncWebSocket *ws, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

String TWAI2JSON(twai_message_t frame);

#endif
