#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum StoveState { STATE_OFF, STATE_WORK, STATE_MODULATING, STATE_KICKSTART };

extern volatile StoveState currentStoveState;
extern volatile bool isStoveEnabled;
extern volatile bool isTargetReached;
extern unsigned long modulationStartTime;
extern unsigned long kickstartStartTime;
extern SemaphoreHandle_t stateMutex;

String getCurrentTime(); // restituisce tz.dateTime("H:i")
void initTime();

void taskStoveControl(void *pvParameters);