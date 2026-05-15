#include <WiFi.h>
#include "tcp_app_server.h"
#include "command_handler.h"
#include "config.h"

// Usiamo la porta 4242 o quella che hai impostato nel metodo C#
static WiFiServer appServer(4242); 

void taskTCPServer(void *pvParameters) {
    appServer.begin();
    appServer.setNoDelay(true); 
    LOG("App TCP Server started on port 4242");

    for (;;) {
        WiFiClient client = appServer.available();
        
        if (client) {
            unsigned long startTimeout = millis();
            while (!client.available() && (millis() - startTimeout < 1000)) {
                vTaskDelay(pdMS_TO_TICKS(5));
            }

            if (client.available()) {
                // Read a line of text sent by the app (terminated by \n)
                String command = client.readStringUntil('\n');
                command.trim();

                if (command.length() > 0) {
                    LOG("App TCP <- " + command);

                    // Execute the command and get the response
                    String response = processCommand(command);

                    // Respond to the app
                    client.println(cleanResponse(response)); // Invia la risposta al client
                    
                    client.flush();
                }
            }
            
            // Chiudiamo il socket come fa il tuo 'using TcpClient' in C#
            client.stop();
            LOG("App TCP transaction complete");
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}