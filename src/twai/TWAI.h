#ifndef TWAI
#define TWAI

#include <driver/twai.h>
#include <driver/gpio.h>

void startTWAI(const int tx_pin, const int rx_pin);
twai_message_t handleTWAI();

#endif