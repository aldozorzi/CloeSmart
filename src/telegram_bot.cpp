#include <Arduino.h>
#include <ezTime.h>
#include "telegram_bot.h"
#include "command_handler.h" 
#include "stove_fsm.h"
#include "crono.h"
#include "config.h"
#include "secrets.h"
#include "localization.h"
#include "thermometer.h"
#include <WiFi.h>
#include <Preferences.h>

WiFiClientSecure securedClient;
UniversalTelegramBot bot(BOT_TOKEN, securedClient);

bool isUserAuthorized(String chat_id) {
    if (chat_id == CHAT_ID_VAL) return true;

    Preferences prefs;
    if (!prefs.begin("whitelist", false)) {
        return false;
    }
    bool authorized = prefs.isKey(chat_id.c_str());
    prefs.end();
    return authorized;
}

void taskTelegram(void *pvParameters) {
  LOG("Telegram bot task started.");
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        LOG("Nuovi messaggi telegram: " + String(numNewMessages));
        for (int i = 0; i < numNewMessages; i++) {
          String chat_id   = String(bot.messages[i].chat_id);
          String text      = bot.messages[i].text;
          String from_name = bot.messages[i].from_name;

          if (bot.messages[i].type == "callback_query") {
            if (chat_id == CHAT_ID_VAL && text.startsWith("AUTH_")) {
              String newUserId = text.substring(5);
              Preferences prefs;
              prefs.begin("whitelist", false);
              prefs.putBool(newUserId.c_str(), true);
              prefs.end();
              bot.sendMessage(CHAT_ID_VAL, ADMIN_AUTH_SUCCESS, "");
              bot.sendMessage(newUserId, USER_GRANTED, "");
            }
            continue;
          }

          if (!isUserAuthorized(chat_id)) {
            bot.sendMessage(chat_id, String(MSG_UNAUTHORIZED) + chat_id, "");
            bot.sendMessage(chat_id, MSG_REQ_SENT, "");
            String adminMsg  = String(ADMIN_NOTIF_REQ) + from_name + ADMIN_NOTIF_ID + chat_id;
            String keyboard  = "[[{\"text\":\"" + String(ADMIN_BTN_AUTH) + "\", \"callback_data\":\"AUTH_" + chat_id + "\"}]]";
            bot.sendMessageWithInlineKeyboard(CHAT_ID_VAL, adminMsg, "Markdown", keyboard);
            continue;
          }

          String response = processCommand(text);
          bot.sendMessage(chat_id, response, "");
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
    } else {
      LOG("WiFi disconnesso, riconnessione in corso...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      int tentativi = 0;
      while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        tentativi++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        LOG("\nWiFi riconnesso.");
        waitForSync();
      } else {
        LOG("\nRiconnessione fallita, riprovo al prossimo ciclo.");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}