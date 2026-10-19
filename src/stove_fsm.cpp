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

// Regola POSIX per Europe/Rome: CET (UTC+1) + DST CEST.
// Funziona offline, senza dipendere dal server timezoned.rop.nl di ezTime.
// Formato POSIX: STDoffsetDST,regola-inizio,regola-fine
// CET-1 = CET e' 1h a est di UTC; CEST da ultima dom di marzo a ultima dom di ottobre, ore 03:00.
static const char* ROME_POSIX = "CET-1CEST,M3.5.0,M10.5.0/3";

String getCurrentTime() {
    return tz.dateTime("H:i");
}

void refreshTimezone() {
    // Prova la lookup dinamica, ma se fallisce (timeout/rete/server giu')
    // mantieni il fallback POSIX invece di restare su UTC.
    if (!tz.setLocation(TIMEZONE)) {
        LOG("Timezone lookup fallita, uso fallback POSIX Europe/Rome");
        tz.setPosix(ROME_POSIX);
    }
    LOG("Timezone: " + tz.getOlson() + " " + tz.getPosix());
}

void initTime() {
    // 1. Fallback offline SUBITO: anche se la lookup fallisce, l'ora e' corretta.
    tz.setPosix(ROME_POSIX);
    // 2. Sincronizza NTP (blocca finche' l'ora UTC e' valida).
    waitForSync();
    // 3. Prova la lookup dinamica (per coerenza con TIMEZONE), con fallback.
    refreshTimezone();
    LOG("Ora locale: " + tz.dateTime("H:i") + " UTC: " + UTC.dateTime("H:i"));
}

void taskStoveControl(void *pvParameters) {
  LOG("Stove control task started.");
  for (;;) {
    // Mantiene aggiornato NTP / ezTime. Senza questa chiamata l'orologio
    // deriva e non si risincronizza mai (loop() principale e' sospeso).
    events();

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