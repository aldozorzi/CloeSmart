#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum StoveState { STATE_OFF, STATE_WORK, STATE_MODULATING, STATE_KICKSTART };

extern volatile StoveState currentStoveState;
extern volatile bool isStoveEnabled;
extern volatile bool isTargetReached;
extern volatile bool isOtaInProgress;
extern unsigned long modulationStartTime;
extern unsigned long kickstartStartTime;
extern SemaphoreHandle_t stateMutex;

String getCurrentTime(); // restituisce tz.dateTime("H:i") in ora locale Europe/Rome
void initTime();
void refreshTimezone(); // riapplica la timezone dopo riconnessione WiFi, con fallback POSIX

void taskStoveControl(void *pvParameters);