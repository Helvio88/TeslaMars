#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <driver/twai.h>
#include <driver/gpio.h>
#include <ESPmDNS.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define ssid      "Pedreschi Pad"
#define password  "11112023"
#define device    "TeslaMars"
#define LED_PIN   2
#define TX_PIN    21
#define RX_PIN    22
#define HTTP_PORT 80
#define WS_PORT   81

String lastMessage = "";

const auto html = R"(<!DOCTYPE html>
<html>
  <head>
    <title>Mars Logger</title>
  </head>
  <body>
    <h1>Mars Logger</h1>
    <pre id='log' style='height: 600px; overflow-y: scroll; border: 1px solid #ccc; padding: 5px; font-family: "Consolas"'></pre>
    <button id="reboot">Reboot</button>
    <script>
        var socket = new WebSocket(`ws://${window.location.host}:81/`);
        var logElement = document.getElementById('log');
        var messageCount = 0;

        socket.onmessage = function(event) {
            var message = event.data;
            if (messageCount >= 100) {
                logElement.removeChild(logElement.firstChild);
            }
            logElement.innerHTML += message + '<br>';
            logElement.scrollTop = logElement.scrollHeight;
            messageCount++;
        };

        document.getElementById('reboot').addEventListener('click', () => {
            fetch('/reboot')
                .then(() => setTimeout(() => location.reload(), 5000))
                .catch(() => setTimeout(() => location.reload(), 5000));
        });
    </script>
  </body>
</html>
)";

const auto favicon = R"(<?xml version="1.0" encoding="utf-8"?>
<svg fill="#FF0000" width="800px" height="800px" viewBox="0 0 32 32" xmlns="http://www.w3.org/2000/svg">
  <path d="M16 7.151l3.302-4.036c0 0 5.656 0.12 11.292 2.74-1.443 2.182-4.307 3.25-4.307 3.25-0.193-1.917-1.536-2.385-5.807-2.385l-4.479 25.281-4.51-25.286c-4.24 0-5.583 0.469-5.776 2.385 0 0-2.865-1.057-4.307-3.24 5.635-2.62 11.292-2.74 11.292-2.74l3.302 4.031h-0.005zM16 1.953c4.552-0.042 9.766 0.703 15.104 3.036 0.714-1.292 0.896-1.859 0.896-1.859-5.833-2.313-11.297-3.109-16-3.13-4.703 0.021-10.167 0.813-16 3.13 0 0 0.26 0.703 0.896 1.865 5.339-2.344 10.552-3.083 15.104-3.047z"/>
</svg>)";

AsyncWebServer server(HTTP_PORT);
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);

const auto ignore = {
  0x102,
  0x103,
  0x108,
  0x113,
  0x118,
  0x119,
  0x122,
  0x123,
  0x126,
  0x128,
  0x129,
  0x132,
  0x139,
  0x142,
  0x186,
  0x195,
  0x1a5,
  0x1d5,
  0x1d6,
  0x1d8,
  0x1f8,
  0x1f9,
  0x1fa,
  0x1fb,
  0x201,
  0x204,
  0x20a,
  0x20c,
  0x211,
  0x212,
  0x213,
  0x214,
  0x217,
  0x219,
  0x21d,
  0x221,
  0x224,
  0x225,
  0x228,
  0x229,
  0x22a,
  0x22b,
  0x232,
  0x238,
  0x23a,
  0x23e,
  0x241,
  0x242,
  0x243,
  0x249,
  0x24a,
  0x252,
  0x253,
  0x257, // Vehicle Speed
  0x25a,
  0x25b,
  0x25c,
  0x25d,
  0x261,
  0x262,
  0x263,
  0x264,
  0x266,
  0x268,
  0x272,
  0x273,
  0x279,
  0x27a,
  0x27d,
  0x282,
  0x283,
  0x284,
  0x285,
  0x286,
  0x288,
  0x289,
  0x292,
  0x293,
  0x294,
  0x295,
  0x29d,
  0x2a1,
  0x2a4,
  0x2a6,
  0x2a7,
  0x2a8,
  0x2b2,
  0x2b3,
  0x2b4,
  0x2b6,
  0x2bd,
  0x2bf,
  0x2c1,
  0x2c2,
  0x2c3,
  0x2c4,
  0x2c7,
  0x2d1,
  0x2d2,
  0x2d3,
  0x2d7,
  0x2e1,
  0x2e2,
  0x2e3,
  0x2e5,
  0x2e8,
  0x2f1,
  0x2f2,
  0x2f3,
  0x2f9,
  0x300,
  0x301,
  0x302,
  0x303,
  0x304,
  0x308,
  0x309,
  0x310,
  0x312,
  0x313,
  0x315,
  0x316,
  0x318,
  0x319,
  0x31c,
  0x31d,
  0x31e,
  0x320,
  0x321,
  0x323,
  0x324,
  0x32c,
  0x330,
  0x332,
  0x333,
  0x334,
  0x335,
  0x339,
  0x33a,
  0x33d,
  0x340,
  0x343,
  0x344,
  0x34a,
  0x352,
  0x353,
  0x355,
  0x356,
  0x357,
  0x359,
  0x35a,
  0x35b,
  0x360,
  0x361,
  0x362,
  0x364,
  0x366,
  0x367,
  0x368,
  0x36b,
  0x36e,
  0x372,
  0x373,
  0x374,
  0x376,
  0x379,
  0x37a,
  0x37d,
  0x380,
  0x381,
  0x382,
  0x383,
  0x384,
  0x386,
  0x38a,
  0x38b,
  0x38d,
  0x392,
  0x393,
  0x395,
  0x396,
  0x397,
  0x399,
  0x39a,
  0x39d,
  0x3a0,
  0x3a1,
  0x3a2,
  0x3a3,
  0x3a4,
  0x3a7,
  0x3a8,
  0x3aa,
  0x3ab,
  0x3b2,
  0x3b3,
  0x3b5,
  0x3b6,
  0x3bb,
  0x3bd,
  0x3c0,
  0x3c2,
  0x3c3,
  0x3c4,
  0x3c5,
  0x3c8,
  0x3c9,
  0x3cc,
  0x3d2,
  0x3d3,
  0x3d5,
  0x3d6,
  0x3d8,
  0x3da,
  0x3db,
  0x3dd,
  0x3df,
  0x3e2,
  0x3e3,
  0x3e5,
  0x3e8,
  0x3e9,
  0x3ed,
  0x3f2,
  0x3f5,
  0x3f6,
  0x3f9,
  0x3fd,
  0x3fe,
  0x400,
  0x401,
  0x405,
  0x409,
  0x40a,
  0x419,
  0x41a,
  0x41d,
  0x420,
  0x421,
  0x422,
  0x423,
  0x428,
  0x42a,
  0x439,
  0x43a,
  0x43b,
  0x43d,
  0x441,
  0x448,
  0x452,
  0x453,
  0x458,
  0x45a,
  0x45d,
  0x464,
  0x46c,
  0x472,
  0x473,
  0x481,
  0x492,
  0x4e2,
  0x4e3,
  0x4f,
  0x514,
  0x51e,
  0x522, // Something with Brakes
  0x524,
  0x527, // Something with Accelerator
  0x528,
  0x52f,
  0x536,
  0x53e,
  0x545,
  0x54f,
  0x552,
  0x553,
  0x554,
  0x557,
  0x55a,
  0x55d, // Something with Brakes
  0x5d7,
  0x5f3,
  0x608,
  0x609,
  0x642,
  0x656,
  0x666,
  0x677,
  0x682,
  0x6a1,
  0x6a2,
  0x6c8,
  0x6d4,
  0x6e8,
  0x708,
  0x709,
  0x70a,
  0x70b,
  0x712,
  0x717,
  0x718,
  0x719,
  0x724,
  0x726,
  0x727,
  0x72a,
  0x737,
  0x739,
  0x743,
  0x744,
  0x748,
  0x752,
  0x757,
  0x75d,
  0x772,
  0x77a,
  0x77d,
  0x782,
  0x788,
  0x789,
  0x78a,
  0x797,
  0x798,
  0x7a8,
  0x7aa,
  0x7b8,
  0x7c8,
  0x7d5,
  0x7d7,
  0x7dd,
  0x7e8,
  0x7f1,
  0x7f4,
  0x7ff,
  0x82
};

bool shouldIgnore(int number) {
  return std::find(ignore.begin(), ignore.end(), number) != ignore.end();
}

void OTA() {
  ArduinoOTA.setHostname(device);
  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    }).onEnd([]() {
      Serial.println("End");
    }).onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    }).onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();
}

void connectWiFi() {
  pinMode(LED_PIN, OUTPUT);
  Serial.print("Connecting WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
}

void setupTwai() {
    twai_general_config_t general_config = {
        .mode = TWAI_MODE_NORMAL,
        .tx_io = (gpio_num_t)TX_PIN,
        .rx_io = (gpio_num_t)RX_PIN,
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

void startWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request -> send(200, "text/html", html);
  });
  
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request -> send(200, "image/svg+xml", favicon);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request -> send(200, "text/html", "ok");
    sleep(500);
    ESP.restart(); 
  });

  server.begin();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if(type == WStype_TEXT) {
    std::string strPayload(*payload, length);
    if (strPayload.compare("reboot") == 0) {
      webSocket.sendTXT(num, "Restarting...");
      sleep(1000);
      ESP.restart();
    }
    
    twai_message_t twai;
    twai.identifier = 0x404;
    twai.extd = 0;
    twai.rtr = 0;
    twai.data_length_code = 8;
    for (int i = 0; i < 8; i++) {
      twai.data[i] = static_cast<uint8_t>(std::strtoul(strPayload.c_str(), nullptr, 10));
    }
    
    if(twai_transmit(&twai, pdMS_TO_TICKS(1000)) == ESP_OK) {
      webSocket.broadcastTXT("[ 69 ]");
    } else {
      webSocket.broadcastTXT("(.)(.)");
      twai_clear_transmit_queue();
    }
  }
}

void startWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  connectWiFi();
  OTA();
  startWebServer();
  startWebSocket();
  setupTwai();
}

void loop() {
  ArduinoOTA.handle();
  webSocket.loop();
  twai_message_t rx_frame;

  if (twai_receive(&rx_frame, pdMS_TO_TICKS(1000)) == ESP_OK) {
    std::ostringstream message;

    auto id = rx_frame.identifier;
    if (!shouldIgnore(id)) {
      message << "Rcvd: (0x" << std::hex << id << "): ";
      for (int i = 0; i < sizeof(rx_frame.data); i++) {
        message << std::setw(3) << std::setfill(' ') << std::dec << static_cast<int>(rx_frame.data[i]);
        if (i < sizeof(rx_frame.data) - 1) {
          message << " | ";
        }
      }
      webSocket.broadcastTXT(message.str().c_str());
    }
  } else {
    Serial.print(".");
  }
}