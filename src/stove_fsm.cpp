#include <Arduino.h>
#include <ezTime.h>
#include "stove_fsm.h"
#include "crono.h"
#include "config.h"
#include "localization.h"


volatile StoveState currentStoveState = STATE_OFF;
volatile bool isStoveEnabled          = false;
volatile bool isTargetReached         = false;
unsigned long modulationStartTime     = 0;
unsigned long kickstartStartTime      = 0;
SemaphoreHandle_t stateMutex;
Timezone tz; 

String getCurrentTime() {
    return tz.dateTime("H:i");
}
void initTime() {
    waitForSync();
    tz.setLocation(TIMEZONE);
}

void taskStoveControl(void *pvParameters) {
  LOG("Stove control task started.");
  for (;;) {

    if (isCronoEnabled) {
      String currentTime = tz.dateTime("H:i");
      if (currentTime.length() == 5) {
        if (currentTime == cronoOnTime && !isStoveEnabled) {
          isStoveEnabled = true;
        }
        if (currentTime == cronoOffTime && isStoveEnabled) {
          isStoveEnabled = false;
        }
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
        LOG("Relay On.");
        if (!isStoveEnabled) {
          currentStoveState = STATE_OFF;
        }
        else if (isTargetReached) {
          currentStoveState = STATE_MODULATING;
          xSemaphoreTake(stateMutex, portMAX_DELAY);
          modulationStartTime = millis();
          xSemaphoreGive(stateMutex);
        }
        break;

      case STATE_MODULATING:
        digitalWrite(RELAY_PIN, LOW);
        LOG("Relay Off.");
        if (!isStoveEnabled) {
          currentStoveState = STATE_OFF;
        }
        else if (!isTargetReached) {
          currentStoveState = STATE_WORK;
        }
        else {
          unsigned long elapsedModulation = millis() - modulationStartTime;
          if (elapsedModulation >= (AUTO_OFF_DELAY - SAFETY_MARGIN)) {
            xSemaphoreTake(stateMutex, portMAX_DELAY);
            kickstartStartTime = millis();
            xSemaphoreGive(stateMutex);
            currentStoveState = STATE_KICKSTART;
          }
        }
        break;

      case STATE_KICKSTART:
        digitalWrite(RELAY_PIN, HIGH);
        LOG("Relay On.");
        if (!isStoveEnabled) {
          currentStoveState = STATE_OFF;
        }
        else if (millis() - kickstartStartTime >= KICKSTART_DURATION) {
          digitalWrite(RELAY_PIN, LOW);
          LOG("Relay Off.");
          xSemaphoreTake(stateMutex, portMAX_DELAY);
          modulationStartTime = millis();
          xSemaphoreGive(stateMutex);
          currentStoveState = STATE_MODULATING;
        }
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}