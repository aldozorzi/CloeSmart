// telnet_server.h
#pragma once

// FreeRTOS task: listens on TCP port 23, accepts one client at a time,
// processes stove commands via processCommand() and echoes the response.
void taskTelnet(void *pvParameters);
