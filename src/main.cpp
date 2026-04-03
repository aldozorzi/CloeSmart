#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include <ezTime.h>
#include "secrets.h"
#include "localization.h"

// --- CONFIGURATION ---
const int RELAY_PIN = 26;

Preferences prefs;

// Timing parameters (in milliseconds)
uint32_t AUTO_OFF_DELAY = 10 * 60000;   // 10 minutes (to be verified in A9)
uint32_t SAFETY_MARGIN = 2 * 60000;     // Intervention 2 mins before (8th minute)
uint32_t KICKSTART_DURATION = 30000;    // 30 seconds of "Work" (Pulse)

// --- Crono Variables ---

String cronoOnTime = "07:00";
String cronoOffTime = "22:00";
bool isCronoEnabled = true;
Timezone tz;

// --- STATIC NETWORK CONFIG ---
IPAddress local_IP(192, 168, 1, 184);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// Finite State Machine (FSM) States
enum StoveState { STATE_OFF, STATE_WORK, STATE_MODULATING };
StoveState currentStoveState = STATE_OFF;

// Global control variables
unsigned long modulationStartTime = 0;
bool isStoveEnabled = false;    // Controlled by /on and /off commands
bool isTargetReached = false;   // Will be managed by thermometer (simulated for now)

WiFiClientSecure securedClient;
UniversalTelegramBot bot(BOT_TOKEN, securedClient);

bool isUserAuthorized(String chat_id) {
    if (chat_id == CHAT_ID_VAL) return true; // Tu sei sempre admin
    prefs.begin("whitelist", true);
    bool authorized = prefs.isKey(chat_id.c_str());
    prefs.end();
    return authorized;
}

// --- Funzione per caricare/salvare orari ---
void loadCronoSettings() {
    prefs.begin("crono", true);
    cronoOnTime = prefs.getString("on", "07:00");
    cronoOffTime = prefs.getString("off", "22:00");
    isCronoEnabled = prefs.getBool("enabled", true);
    prefs.end();
}

void saveCronoSettings() {
    prefs.begin("crono", false);
    prefs.putString("on", cronoOnTime);
    prefs.putString("off", cronoOffTime);
    prefs.putBool("enabled", isCronoEnabled);
    prefs.end();
}

bool isValidTime(String t) {
    // Controllo lunghezza e separatore
    if (t.length() != 5 || t.charAt(2) != ':') return false;
    
    // Estrazione e conversione ore/minuti
    int hh = t.substring(0, 2).toInt();
    int mm = t.substring(3, 5).toInt();
    
    // Controllo range logico
    if (hh < 0 || hh > 23) return false;
    if (mm < 0 || mm > 59) return false;
    
    return true;
}

// --- TASK 1: PHYSICAL STOVE MANAGEMENT (CORE 1) ---
void taskStoveControl(void *pvParameters) {
  for (;;) {
    String currentTime = tz.dateTime("H:i"); // Prende HH:MM

    if (isCronoEnabled) {
      if (currentTime == cronoOnTime && !isStoveEnabled) {
        isStoveEnabled = true;
        // bot.sendMessage(CHAT_ID_VAL, "📅 *Crono*: Avvio programmato eseguito 🔥", "Markdown");
      }
      if (currentTime == cronoOffTime && isStoveEnabled) {
        isStoveEnabled = false;
        // bot.sendMessage(CHAT_ID_VAL, "📅 *Crono*: Spegnimento programmato eseguito ❄️", "Markdown");
      }
    }

    switch (currentStoveState) {
      
      case STATE_OFF:
        digitalWrite(RELAY_PIN, LOW);
        if (isStoveEnabled) {
          currentStoveState = STATE_WORK;
        }
        break;

      case STATE_WORK:
        digitalWrite(RELAY_PIN, HIGH);
        // User turns off via Telegram
        if (!isStoveEnabled) {
          currentStoveState = STATE_OFF;
        } 
        // Temperature OK, start modulating
        else if (isTargetReached) {
          currentStoveState = STATE_MODULATING;
          modulationStartTime = millis();
        }
        break;

      case STATE_MODULATING:
        digitalWrite(RELAY_PIN, LOW); // Relay open = Modulation
        
        // 1. User turns off: total stop
        if (!isStoveEnabled) {
          currentStoveState = STATE_OFF;
        }
        // 2. Temperature drops below setpoint: back to work
        else if (!isTargetReached) {
          currentStoveState = STATE_WORK;
        }
        // 3. ANTI-SHUTDOWN Logic (The "Pulse" or "Kickstart")
        else {
          uint32_t elapsedModulation = millis() - modulationStartTime;
          if (elapsedModulation >= (AUTO_OFF_DELAY - SAFETY_MARGIN)) {
            Serial.println("Resetting stove timer (Kickstart)...");
            digitalWrite(RELAY_PIN, HIGH); 
            vTaskDelay(pdMS_TO_TICKS(KICKSTART_DURATION)); 
            digitalWrite(RELAY_PIN, LOW);
            modulationStartTime = millis(); 
          }
        }
        break;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// --- TASK 2: TELEGRAM AND WIFI (CORE 0) ---
void taskTelegram(void *pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        for (int i = 0; i < numNewMessages; i++) {
          String chat_id = String(bot.messages[i].chat_id);
          String text = bot.messages[i].text;
          String from_name = bot.messages[i].from_name;

          // --- 1. GESTIONE CALLBACK (Pulsante Autorizza) ---
          if (bot.messages[i].type == "callback_query") {
            if (chat_id == CHAT_ID_VAL && text.startsWith("AUTH_")) {
              String newUserId = text.substring(5);
              
              prefs.begin("whitelist", false);
              prefs.putBool(newUserId.c_str(), true);
              prefs.end();

              bot.sendMessage(CHAT_ID_VAL, ADMIN_AUTH_SUCCESS, "");
              bot.sendMessage(newUserId, USER_GRANTED, "");
            }
            continue;
          }

          // --- 2. FILTRO SICUREZZA ---
          if (!isUserAuthorized(chat_id)) {
            // Notifica l'utente e l'admin
            bot.sendMessage(chat_id, String(MSG_UNAUTHORIZED) + chat_id, "");
            bot.sendMessage(chat_id, MSG_REQ_SENT, "");

            // Messaggio all'admin con pulsante inline
            String adminMsg = String(ADMIN_NOTIF_REQ) + from_name + ADMIN_NOTIF_ID + chat_id;

            // Costruisci la tastiera (Inline Keyboard)
            String keyboard = "[[{\"text\":\"" + String(ADMIN_BTN_AUTH) + "\", \"callback_data\":\"AUTH_" + chat_id + "\"}]]";

            // Inviato con 4 argomenti: ID, Testo, Modalità, Tastiera
            bot.sendMessageWithInlineKeyboard(CHAT_ID_VAL, adminMsg, "Markdown", keyboard);
            continue;
          }

          // --- 3. COMANDI AUTORIZZATI ---
          if (text == CMD_ON) {
            isStoveEnabled = true;
            bot.sendMessage(chat_id, RESP_STOVE_ON, "");
          } 
          else if (text == CMD_OFF) {
            isStoveEnabled = false;
            bot.sendMessage(chat_id, RESP_STOVE_OFF, "");
          } 
          else if (text == CMD_STATUS) {
            // 1. Recupera l'etichetta dello stato fisico della stufa
            String statusLabel = (currentStoveState == STATE_OFF) ? ST_LABEL_OFF : 
                                 (currentStoveState == STATE_WORK) ? ST_LABEL_WORK : ST_LABEL_MOD;
            
            // 2. Costruisci l'intestazione con lo stato attuale
            String msg = RESP_STATUS_HEADER + statusLabel + "\n";
            
            // 3. Aggiungi info sul Crono (Attivo/Disattivo)
            msg += (isCronoEnabled ? MSG_CHRONO_ON : MSG_CHRONO_OFF);
            
            // 4. Aggiungi il dettaglio degli orari impostati
            msg += MSG_CHRONO_SETTINGS + cronoOnTime + " - " + cronoOffTime + "\n";
            
            // 5. Aggiungi l'ora attuale del sistema (utile per debug)
            msg += "🕒 Ora sistema: " + tz.dateTime("H:i");

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
              String newTime = text.substring(8, 13); // Estrae HH:MM
              newTime.trim();

              if (isValidTime(newTime)) {
                  if(newTime != cronoOffTime)
                  {
                    cronoOnTime = newTime;
                  saveCronoSettings();
                  bot.sendMessage(chat_id, RESP_CHRONO_UPDATED + newTime, "");
                  }else{
                    bot.sendMessage(chat_id, RESP_CHRONO_INVALID_SAME, "");
                  }
                  
              } else {
                String errorMsg = RESP_CHRONO_INVALID;
                errorMsg.replace("%CMD%", "/set_on");
                bot.sendMessage(chat_id, errorMsg, "");
              }
          }
          else if (text == "/set_off") {
              String msg = RESP_CHRONO_HELP;
              msg.replace("%CMD%", "/set_off");
              bot.sendMessage(chat_id, msg, "Markdown");
          }
          else if (text.startsWith("/set_off ")) {
              String newTime = text.substring(9, 14);
              newTime.trim();

              if (isValidTime(newTime)) {
                if(newTime != cronoOnTime)
                {
                  cronoOffTime = newTime;
                  saveCronoSettings();
                  bot.sendMessage(chat_id, RESP_CHRONO_UPDATED + newTime, "");
                }else{
                   bot.sendMessage(chat_id, RESP_CHRONO_INVALID_SAME, "");
                }
                  
              } else {
                  String errorMsg = RESP_CHRONO_INVALID;
                errorMsg.replace("%CMD%", "/set_off");
                bot.sendMessage(chat_id, errorMsg, "");
              }
          }
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  // Static IP configuration
  /*if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }*/


  securedClient.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  waitForSync(); // Aspetta che l'ora sia sincronizzata da internet
  tz.setLocation(TIMEZONE); // Imposta il fuso orario italiano
  
  Serial.println("Current time: " + tz.dateTime());

  loadCronoSettings();

  bot.sendMessage(CHAT_ID_VAL, MSG_SYSTEM_ONLINE, "Markdown");

  // Task creation
  xTaskCreatePinnedToCore(taskStoveControl, "StoveControl", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskTelegram, "TelegramBot", 8192, NULL, 1, NULL, 0);

  Serial.println("FreeRTOS System Started.");
}

void loop() {
  // Empty loop, handled by tasks
  vTaskDelete(NULL); 
}