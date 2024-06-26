#ifndef MARS_TWAI
#define MARS_TWAI

#include <Arduino.h>
#include <ArduinoJson.h>
#include <driver/twai.h>
#include <driver/gpio.h>
#include <MARS_FS.h>

void startTWAI(const int tx_pin, const int rx_pin);
twai_message_t handleTWAI();
twai_message_t simulateTWAIFrame();
bool shouldIgnore(int id);

#endif