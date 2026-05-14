#include <Arduino.h>
#include <WiFi.h>
#include <TelnetStream.h>
#include "telnet_server.h"
#include "command_handler.h"
#include "config.h"
#include "localization.h"

static const size_t INPUT_BUF_MAX = 128;

// Mantiene la tua funzione cleanResponse originale immutata
String cleanResponse(String text) {
  String cleaned = "";
  cleaned.reserve(text.length());
  
  text.replace("à", "a");
  text.replace("è", "e");
  text.replace("é", "e");
  text.replace("ì", "i");
  text.replace("ò", "o");
  text.replace("ù", "u");

  bool isAtStartOfLine = true;
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '\n') {
      if (cleaned.length() == 0 || cleaned[cleaned.length() - 1] != '\r') {
        cleaned += '\r';
      }
      cleaned += '\n';
      isAtStartOfLine = true;
      continue;
    }
    if (c == '\r') {
      cleaned += '\r';
      continue;
    }
    if ((uint8_t)c < 32 || (uint8_t)c > 126) continue;
    if (isAtStartOfLine) {
      if (c == ' ' || c == '\t') continue;
      else isAtStartOfLine = false;
    }
    cleaned += c;
  }
  return cleaned;
}

static void sendStreamBanner() {
  TelnetStream.println("+----------------------------------+");
  TelnetStream.println("|       CLOE - Telnet Shell        |");
  TelnetStream.println("+----------------------------------+");
  TelnetStream.println("Comandi disponibili:");
  TelnetStream.print("  "); TelnetStream.print(CMD_ON); TelnetStream.print("  "); TelnetStream.print(CMD_OFF); TelnetStream.print("  "); TelnetStream.println(CMD_STATUS);
  TelnetStream.println("  /crono_on  /crono_off");
  TelnetStream.println("  /set_on HH:MM   /set_off HH:MM");
  TelnetStream.println("  /set_temp N     (10-30 degC)");
  TelnetStream.println("  /quit           (chiudi sessione)");
  TelnetStream.println();
}

void taskTelnet(void *pvParameters) {
  TelnetStream.begin();
  LOG("TelnetStream started on port 23");

  String inputBuf = "";
  inputBuf.reserve(INPUT_BUF_MAX);

  for (;;) {
    while (TelnetStream.available()) {
      char c = (char)TelnetStream.read();

      if (c == '\r' || c == '\n') {
        inputBuf.trim();

        if (inputBuf.length() > 0) {
          if (inputBuf == "/quit" || inputBuf == "exit") {
            TelnetStream.println("\r\nArrivederci!");
            TelnetStream.flush();
            TelnetStream.stop();
            inputBuf = "";
            break;
          }else if (inputBuf == "?" || inputBuf == "/help") {
            TelnetStream.println();
            sendStreamBanner();
            TelnetStream.print("> ");
            TelnetStream.flush();
            inputBuf = "";
            continue;
          }

          String response = processCommand(inputBuf);
          String cleanRes = cleanResponse(response);

          TelnetStream.println();
          TelnetStream.println(cleanRes);
          TelnetStream.print("> ");
          TelnetStream.flush();
        } else {
          TelnetStream.println();
          sendStreamBanner();
          TelnetStream.print("> ");
          TelnetStream.flush();
        }
        inputBuf = "";
      } 
      else if (c >= 32 && c <= 126) {
        if (inputBuf.length() < INPUT_BUF_MAX) {
          inputBuf += c;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}