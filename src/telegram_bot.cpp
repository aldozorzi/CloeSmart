#include <Arduino.h>
#include <ezTime.h>
#include "telegram_bot.h"
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
    prefs.begin("whitelist", true);
    bool authorized = prefs.isKey(chat_id.c_str());
    prefs.end();
    return authorized;
}

void taskTelegram(void *pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
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

          if (text == CMD_ON) {
            isStoveEnabled = true;
            bot.sendMessage(chat_id, RESP_STOVE_ON, "");
          }
          else if (text == CMD_OFF) {
            isStoveEnabled = false;
            bot.sendMessage(chat_id, RESP_STOVE_OFF, "");
          }
          else if (text == CMD_STATUS) {
            String statusLabel = (currentStoveState == STATE_OFF)       ? ST_LABEL_OFF  :
                                 (currentStoveState == STATE_WORK)      ? ST_LABEL_WORK :
                                 (currentStoveState == STATE_KICKSTART) ? ST_LABEL_KICK :
                                                                          ST_LABEL_MOD;
                                                                          
            
            String msg  = RESP_STATUS_HEADER + statusLabel + "\n";
            msg += ST_THERMO;
            msg.replace("%TMP%", String(currentTemperature,1));
            msg.replace("%TGT%", String(targetTemperature,1));
            msg += "\n";
            msg += (isCronoEnabled ? MSG_CHRONO_ON : MSG_CHRONO_OFF);
            msg += MSG_CHRONO_SETTINGS + cronoOnTime + " - " + cronoOffTime + "\n";
            msg += "🕒 Ora sistema: " + getCurrentTime();
            bot.sendMessage(chat_id, msg, "Markdown");
          }
          else if (text == "/crono_on") {
            isCronoEnabled = true;
            saveCronoSettings();
            bot.sendMessage(chat_id, MSG_CHRONO_ON, "");
          }
          else if (text == "/crono_off") {
            isCronoEnabled = false;
            saveCronoSettings();
            bot.sendMessage(chat_id, MSG_CHRONO_OFF, "");
          }
          else if (text == "/set_on") {
            String msg = RESP_CHRONO_HELP;
            msg.replace("%CMD%", "/set_on");
            bot.sendMessage(chat_id, msg, "Markdown");
          }
          else if (text.startsWith("/set_on ")) {
            int spaceIndex = text.indexOf(' ');
            if (spaceIndex == -1) {
              bot.sendMessage(chat_id, RESP_CHRONO_INVALID, "");
            } else {
              String newTime = text.substring(spaceIndex + 1);
              newTime.trim();
              if (isValidTime(newTime)) {
                if (newTime != cronoOffTime) {
                  cronoOnTime = newTime;
                  saveCronoSettings();
                  bot.sendMessage(chat_id, RESP_CHRONO_UPDATED + newTime, "");
                } else {
                  bot.sendMessage(chat_id, RESP_CHRONO_INVALID_SAME, "");
                }
              } else {
                String errorMsg = RESP_CHRONO_INVALID;
                errorMsg.replace("%CMD%", "/set_on");
                bot.sendMessage(chat_id, errorMsg, "");
              }
            }
          }
          else if (text == "/set_off") {
            String msg = RESP_CHRONO_HELP;
            msg.replace("%CMD%", "/set_off");
            bot.sendMessage(chat_id, msg, "Markdown");
          }
          else if (text.startsWith("/set_off ")) {
            int spaceIndex = text.indexOf(' ');
            if (spaceIndex == -1) {
              bot.sendMessage(chat_id, RESP_CHRONO_INVALID, "");
            } else {
              String newTime = text.substring(spaceIndex + 1);
              newTime.trim();
              if (isValidTime(newTime)) {
                if (newTime != cronoOnTime) {
                  cronoOffTime = newTime;
                  saveCronoSettings();
                  bot.sendMessage(chat_id, RESP_CHRONO_UPDATED + newTime, "");
                } else {
                  bot.sendMessage(chat_id, RESP_CHRONO_INVALID_SAME, "");
                }
              } else {
                String errorMsg = RESP_CHRONO_INVALID;
                errorMsg.replace("%CMD%", "/set_off");
                bot.sendMessage(chat_id, errorMsg, "");
              }
            }
          }
          else if (text == "/set_temp") {
              bot.sendMessage(chat_id, RESP_TEMP_HELP, "");
          }
          else if (text.startsWith("/set_temp ")) {
              int spaceIndex = text.indexOf(' ');
              String val = text.substring(spaceIndex + 1);
              val.trim();
              float newTemp = val.toFloat();
              if (newTemp >= 10.0f && newTemp <= 30.0f) {
                  targetTemperature = newTemp;
                  saveTargetTemp();
                  bot.sendMessage(chat_id, RESP_TEMP_UPDATED + String(newTemp, 1) + "°C", "");
              } else {
                  bot.sendMessage(chat_id, RESP_TEMP_INVALID, "");
              }
          }
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