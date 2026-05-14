// telnet_server.cpp
//
// Minimal Telnet server (port 23) for local LAN control.
// Accepts one client at a time; delegates command parsing to processCommand().
//
#include <Arduino.h>
#include <WiFi.h>
#include "telnet_server.h"
#include "command_handler.h"
#include "config.h"
#include "localization.h"

static const uint16_t TELNET_PORT    = 23;
static const size_t   INPUT_BUF_MAX  = 128;   // max chars per line

// Telnet control bytes we want to swallow silently
// (IAC sequences sent by most Telnet clients on connect)
static const uint8_t IAC  = 0xFF;
static const uint8_t DONT = 0xFE;
static const uint8_t DO   = 0xFD;
static const uint8_t WONT = 0xFC;
static const uint8_t WILL = 0xFB;
static const uint8_t ECHO                 = 0x01;
static const uint8_t SUPPRESS_GO_AHEAD   = 0x03;
static const uint8_t LINEMODE             = 0x22;

static WiFiServer telnetServer(TELNET_PORT);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Skip a 3-byte IAC option sequence already started (first byte = IAC already read)
static void skipIacOption(WiFiClient& client) {
  // We need 2 more bytes: verb + option
  unsigned long deadline = millis() + 200;
  int got = 0;
  while (got < 2 && millis() < deadline) {
    if (client.available()) { client.read(); got++; }
    else vTaskDelay(pdMS_TO_TICKS(5));
  }
}

String cleanResponse(String text) {
  String cleaned = "";
  cleaned.reserve(text.length());
  
  // 1. Sostituzione preventiva delle lettere accentate UTF-8
  text.replace("à", "a");
  text.replace("è", "e");
  text.replace("é", "e");
  text.replace("ì", "i");
  text.replace("ò", "o");
  text.replace("ù", "u");

  bool isAtStartOfLine = true;

  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];

    // 2. Gestione dei caratteri di a capo con forzatura \r\n
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

    // 3. Salta i caratteri speciali non ASCII (emoji e relativi frammenti UTF-8)
    if ((uint8_t)c < 32 || (uint8_t)c > 126) {
      continue;
    }

    // 4. Se siamo a inizio riga e il carattere è uno spazio (lasciato dall'emoji rimossa), lo salta
    if (isAtStartOfLine) {
      if (c == ' ' || c == '\t') {
        continue;
      } else {
        isAtStartOfLine = false; // Trovato il primo carattere valido della riga
      }
    }

    cleaned += c;
  }
  return cleaned;
}

static void sendBanner(WiFiClient& client) {
  client.println("+----------------------------------+");
  client.println("|       CLOE - Telnet Shell        |");
  client.println("+----------------------------------+");
  client.println("Comandi disponibili:");
  client.print("  "); client.print(CMD_ON); client.print("  "); client.print(CMD_OFF); client.print("  "); client.println(CMD_STATUS);
  client.println("  /crono_on  /crono_off");
  client.println("  /set_on HH:MM   /set_off HH:MM");
  client.println("  /set_temp N     (10-30 degC)");
  client.println("  /quit           (chiudi sessione)");
  client.println();
}

void taskTelnet(void *pvParameters) {
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  LOG("Telnet server avviato sulla porta " + String(TELNET_PORT));

  for (;;) {
    WiFiClient client = telnetServer.accept();
    if (!client) {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    LOG("Telnet: client connesso da " + client.remoteIP().toString());

    uint8_t disableClientEcho[] = { IAC, DONT, ECHO };
    client.write(disableClientEcho, sizeof(disableClientEcho));

    vTaskDelay(pdMS_TO_TICKS(50));
    while (client.available()) { client.read(); } // Svuota i byte di risposta

    sendBanner(client);
    client.print("> ");

    String inputBuf = "";
    inputBuf.reserve(INPUT_BUF_MAX);
    int escapeStage = 0;

    while (client.connected()) {
      while (client.available()) {
        uint8_t c = (uint8_t)client.read();

        // Salta i comandi di negoziazione nativi di Telnet se inviati dal client
        if (c == IAC) {
          skipIacOption(client);
          continue;
        }

        // Gestione sequenza di escape delle frecce (Frecce inviano: 0x1B, 0x5B, seguito da 0x41..0x44)
        if (c == 0x1B) { 
          escapeStage = 1; // Intercettato ESC
          continue;
        }
        if (escapeStage == 1 && c == 0x5B) {
          escapeStage = 2; // Intercettato '['
          continue;
        }
        if (escapeStage == 2) {
          escapeStage = 0; // Fine sequenza della freccia (scartata)
          continue;
        }

        // Carriage return / line feed
        if (c == '\r' || c == '\n') {
          client.println();   
          inputBuf.trim();

          if (inputBuf.length() == 0) {
            client.print("> ");
            continue;
          }

          if (inputBuf == "/quit" || inputBuf == "exit") {
            client.println("Arrivederci!");
            client.stop();
            break;
          }

          String response = processCommand(inputBuf);
          String cleanRes = cleanResponse(response); // Rimuove emoji e icone di Telegram

          client.println(cleanRes);
          client.print("> ");

          inputBuf = "";
          continue;
        }

        // Backspace
        if (c == 0x7F || c == 0x08) {
          if (inputBuf.length() > 0) {
            inputBuf.remove(inputBuf.length() - 1);
            client.print("\b \b");   
          }
          continue;
        }

        // Caratteri stampabili: reinserito l'eco locale e filtrati i residui delle frecce
        if (c >= 0x20 && c < 0x7F && escapeStage == 0) {
          if (inputBuf.length() < INPUT_BUF_MAX) {
            inputBuf += (char)c;
            client.write(c);   // Riattivato l'eco gestito dall'ESP32
          }
        }
      }

      vTaskDelay(pdMS_TO_TICKS(20));
    }

    client.stop();
    LOG("Telnet: client disconnesso.");
  }
}