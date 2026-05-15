#include <Arduino.h>
#include <WiFi.h>
#include "telnet_server.h"
#include "command_handler.h"
#include "config.h"
#include "localization.h"

static const size_t INPUT_BUF_MAX = 128;
static WiFiServer telnetServer(23);
static WiFiClient client;
static String inputBuf;

// Stati macchina
static bool sessionActive = false;
static bool skipLF = false;

static void sendBanner() {
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
    LOG("Native Telnet server started on port 23");
    inputBuf.reserve(INPUT_BUF_MAX);

    for (;;) {
        // 1. GESTIONE NUOVA CONNESSIONE
        if (!sessionActive || !client.connected()) {
            WiFiClient newClient = telnetServer.available();
            if (newClient) {
                client = newClient;
                sessionActive = true;
                inputBuf = "";
                skipLF = false;
                LOG("Client connected");

                // NEGOZIAZIONE TELNET AGGRESSIVA
                // Forza il client a non fare echo locale e a inviare i caratteri subito
                uint8_t telnetOpts[] = {
                    0xFF, 0xFB, 0x01, // IAC WILL ECHO
                    0xFF, 0xFB, 0x03, // IAC WILL SUPPRESS GO AHEAD
                    0xFF, 0xFD, 0x03, // IAC DO SUPPRESS GO AHEAD
                    0xFF, 0xFD, 0x01  // IAC DO ECHO
                };
                client.write(telnetOpts, sizeof(telnetOpts));
                client.flush();

                sendBanner();
                client.print("> ");
                client.flush();
            }
        }

        // 2. ELABORAZIONE FLUSSO DATI
        if (sessionActive && client.connected()) {
            while (client.available()) {
                uint8_t b = client.read();

                // FILTRO COMANDI DI PROTOCOLLO (IAC)
                if (b == 0xFF) {
                    // Se arriva un comando IAC, leggiamo i due byte successivi e li ignoriamo
                    // per evitare che sporchino il buffer dei comandi.
                    if (client.available() >= 2) {
                        client.read(); // Comando (WILL/WONT/DO/DONT)
                        client.read(); // Opzione
                    }
                    continue;
                }

                char c = (char)b;

                // GESTIONE FINE RIGA (NORMALIZZAZIONE \r, \n, \r\n)
                if (c == '\r' || c == '\n') {
                    if (c == '\n' && skipLF) {
                        skipLF = false;
                        continue;
                    }
                    skipLF = (c == '\r');

                    inputBuf.trim();
                    if (inputBuf.length() > 0) {
                        if (inputBuf.equalsIgnoreCase("/quit") || inputBuf.equalsIgnoreCase("exit")) {
                            client.println("\r\nArrivederci!");
                            client.flush();
                            client.stop();
                            sessionActive = false;
                            inputBuf = "";
                            break;
                        }

                        String response = processCommand(inputBuf);
                        client.print("\r\n" + cleanResponse(response));
                    }
                    
                    inputBuf = "";
                    client.print("\r\n> ");
                    client.flush();
                    continue;
                }

                // Se riceviamo un carattere normale, resettiamo il controllo del fine riga
                skipLF = false;

                // GESTIONE BACKSPACE
                if (c == 0x08 || c == 0x7F) {
                    if (inputBuf.length() > 0) {
                        inputBuf.remove(inputBuf.length() - 1);
                        client.print("\b \b"); // Cancella il carattere sul terminale
                        client.flush();
                    }
                    continue;
                }

                // ACCUMULO CARATTERI STAMPABILI ED ECHO
                if (c >= 32 && c <= 126) {
                    if (inputBuf.length() < INPUT_BUF_MAX) {
                        inputBuf += c;
                        client.print(c); // Echo immediato gestito dal server
                        client.flush();
                    }
                }
            }
        } else {
            sessionActive = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}