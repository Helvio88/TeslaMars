#include "Arduino.h"
#include "TWAI.h"

void startTWAI(const int tx_pin, const int rx_pin) {
  twai_general_config_t general_config = {
      .mode = TWAI_MODE_NORMAL,
      .tx_io = (gpio_num_t)tx_pin,
      .rx_io = (gpio_num_t)rx_pin,
      .clkout_io = (gpio_num_t)TWAI_IO_UNUSED,
      .bus_off_io = (gpio_num_t)TWAI_IO_UNUSED,
      .tx_queue_len = 10,
      .rx_queue_len = 10,
      .alerts_enabled = TWAI_ALERT_ALL,
      .clkout_divider = 0
  };

  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  esp_err_t error;

  error = twai_driver_install(&general_config, &timing_config, &filter_config);

  if(error == ESP_OK) {
      Serial.println("TWAI Driver Installation OK");
  } else {
      Serial.println("TWAI Driver Installation Failed");
  }

  error = twai_start();

  if(error == ESP_OK) {
      Serial.println("TWAI Driver Start OK");
  } else {
      Serial.println("TWAI Driver Start Failed");
  }
}

twai_message_t handleTWAI() {
    twai_message_t rx_frame;
    twai_receive(&rx_frame, pdMS_TO_TICKS(1000));
    return rx_frame;
}

/*
void handleTWAI() {
  twai_message_t rx_frame;

  if (twai_receive(&rx_frame, pdMS_TO_TICKS(1000)) == ESP_OK) {
    std::ostringstream message;

    auto id = rx_frame.identifier;
    if (!shouldIgnore(id)) {
      message << "(0x" << std::hex << id << "): ";
      for (int i = 0; i < sizeof(rx_frame.data); i++) {
        message << std::right << std::setw(3) << rx_frame.data[i];
        if (i < sizeof(rx_frame.data) - 1) {
          message << " | ";
        }
      }
      // ws.textAll(message.str().c_str());
    }
  }
}

*/