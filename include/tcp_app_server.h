#ifndef TCP_APP_SERVER_H
#define TCP_APP_SERVER_H

#include <Arduino.h>

/**
 * @brief Task per gestire i comandi TCP provenienti dall'app.
 * Porta dedicata: 4242 (o quella configurata nell'app).
 */
void taskTCPServer(void *pvParameters);

#endif