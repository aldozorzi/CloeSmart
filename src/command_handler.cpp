// command_handler.cpp
//
// Single source of truth for all stove commands.
// Called by both taskTelegram (telegram_bot.cpp) and taskTelnet (telnet_server.cpp).
//
#include "command_handler.h"
#include "stove_fsm.h"
#include "crono.h"
#include "thermometer.h"
#include "localization.h"
#include "config.h"

String processCommand(const String& text) {
  LOG("Processing command: " + text);
  // --- /on ---
  if (text == CMD_ON) {
    isStoveEnabled = true;
    return String(RESP_STOVE_ON);
  }

  // --- /off ---
  if (text == CMD_OFF) {
    isStoveEnabled = false;
    return String(RESP_STOVE_OFF);
  }

  // --- /status ---
  if (text == CMD_STATUS) {
    String statusLabel = (currentStoveState == STATE_OFF)       ? ST_LABEL_OFF  :
                         (currentStoveState == STATE_WORK)      ? ST_LABEL_WORK :
                         (currentStoveState == STATE_KICKSTART) ? ST_LABEL_KICK :
                                                                  ST_LABEL_MOD;

    String msg = RESP_STATUS_HEADER + statusLabel + "\n";
    msg += ST_THERMO;
    msg.replace("%TMP%", String(currentTemperature, 1));
    msg.replace("%TGT%", String(targetTemperature, 1));
    msg += "\n";
    msg += (isCronoEnabled ? MSG_CHRONO_ON : MSG_CHRONO_OFF);
    msg += MSG_CHRONO_SETTINGS + cronoOnTime + " - " + cronoOffTime + "\n";
    msg += "🕒 Ora sistema: " + getCurrentTime();
    return msg;
  }

  // --- /crono_on ---
  if (text == "/crono_on") {
    isCronoEnabled = true;
    saveCronoSettings();
    return String(MSG_CHRONO_ON);
  }

  // --- /crono_off ---
  if (text == "/crono_off") {
    isCronoEnabled = false;
    saveCronoSettings();
    return String(MSG_CHRONO_OFF);
  }

  // --- /set_on (no arg → help) ---
  if (text == "/set_on") {
    String msg = RESP_CHRONO_HELP;
    msg.replace("%CMD%", "/set_on");
    return msg;
  }

  // --- /set_on HH:MM ---
  if (text.startsWith("/set_on ")) {
    String newTime = text.substring(8);
    newTime.trim();
    if (!isValidTime(newTime)) {
      String err = RESP_CHRONO_INVALID;
      err.replace("%CMD%", "/set_on");
      return err;
    }
    if (newTime == cronoOffTime) return String(RESP_CHRONO_INVALID_SAME);
    cronoOnTime = newTime;
    saveCronoSettings();
    return String(RESP_CHRONO_UPDATED) + newTime;
  }

  // --- /set_off (no arg → help) ---
  if (text == "/set_off") {
    String msg = RESP_CHRONO_HELP;
    msg.replace("%CMD%", "/set_off");
    return msg;
  }

  // --- /set_off HH:MM ---
  if (text.startsWith("/set_off ")) {
    String newTime = text.substring(9);
    newTime.trim();
    if (!isValidTime(newTime)) {
      String err = RESP_CHRONO_INVALID;
      err.replace("%CMD%", "/set_off");
      return err;
    }
    if (newTime == cronoOnTime) return String(RESP_CHRONO_INVALID_SAME);
    cronoOffTime = newTime;
    saveCronoSettings();
    return String(RESP_CHRONO_UPDATED) + newTime;
  }

  // --- /set_temp (no arg → help) ---
  if (text == "/set_temp") {
    return String(RESP_TEMP_HELP);
  }

  // --- /set_temp N ---
  if (text.startsWith("/set_temp ")) {
    String val = text.substring(10);
    val.trim();
    float newTemp = val.toFloat();
    if (newTemp >= 10.0f && newTemp <= 30.0f) {
      targetTemperature = newTemp;
      saveTargetTemp();
      return String(RESP_TEMP_UPDATED) + String(newTemp, 1) + "°C";
    }
    return String(RESP_TEMP_INVALID);
  }

  // --- unknown ---
  return "❓ Comando non riconosciuto: " + text;
}
