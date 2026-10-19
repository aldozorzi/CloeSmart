#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include "localization.h"
#include "crono.h"
#include "stove_fsm.h"
#include "telegram_bot.h"
#include "web_server.h"
#include "thermometer.h"
#include "button.h"

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  stateMutex = xSemaphoreCreateMutex();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);

#if USE_STATIC_IP
  if (!WiFi.config(LOCAL_IP, GATEWAY, SUBNET, PRIMARY_DNS, SECONDARY_DNS)) {
    LOG("Static IP configuration failed");
  } else {
    LOG("Using static IP");
  }
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  securedClient.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  LOG("\nIP locale: " + WiFi.localIP().toString());

  initTime();
  LOG("Current time: " + getCurrentTime());
  loadCronoSettings();
  LOG("Crono settings loaded.");
  initThermometer();
  Serial.flush();
  LOG("Thermometer initialized.");
  Serial.flush();
  initButton();


 

  xTaskCreatePinnedToCore(taskStoveControl, "StoveControl", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskTelegram,     "TelegramBot",  8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskThermometer,  "Thermometer",  4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskButton,       "Button",       2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskWebServer,    "WebServer",    8192, NULL, 2, NULL, 0);
  bot.sendMessage(CHAT_ID_VAL, MSG_SYSTEM_ONLINE, "Markdown");

  LOG("FreeRTOS System Started.");
}

void loop() {
  vTaskSuspend(NULL);
}