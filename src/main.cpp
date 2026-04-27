#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include "localization.h"
#include "crono.h"
#include "stove_fsm.h"
#include "telegram_bot.h"
#include "thermometer.h"
#include "button.h"

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  stateMutex = xSemaphoreCreateMutex();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  securedClient.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  initTime();
  LOG("Current time: " + getCurrentTime());
  loadCronoSettings();
  initThermometer();

  initButton();


  bot.sendMessage(CHAT_ID_VAL, MSG_SYSTEM_ONLINE, "Markdown");

  xTaskCreatePinnedToCore(taskStoveControl, "StoveControl", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskTelegram,     "TelegramBot",  8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskThermometer, "Thermometer", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskButton, "Button", 2048, NULL, 2, NULL, 1);


  LOG("FreeRTOS System Started.");
}

void loop() {
  vTaskSuspend(NULL);
}