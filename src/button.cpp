#include <Arduino.h>
#include "button.h"
#include "stove_fsm.h"
#include "config.h"

void initButton() {
    pinMode(BUTTON_PIN, INPUT);  
    LOG("Button initalized on pin " + String(BUTTON_PIN));
}

void taskButton(void *pvParameters) {
    bool lastState        = LOW;  
    bool pressing         = false; 
    unsigned long pressStart = 0; 

    for (;;) {
        bool currentState = digitalRead(BUTTON_PIN);

        if (currentState == HIGH && lastState == LOW) {
            pressing   = true;
            pressStart = millis();
        }

        if (currentState == LOW && lastState == HIGH) {
            pressing = false;
        }

        if (pressing && (millis() - pressStart >= BUTTON_HOLD_MS)) {
            pressing = false; 

            isStoveEnabled = !isStoveEnabled;
            LOG(isStoveEnabled ? "Bottone: stufa accesa" : "Bottone: stufa spenta");

            while (digitalRead(BUTTON_PIN) == HIGH) {
                vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
            }
        }

        lastState = currentState;
        vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
    }
}
