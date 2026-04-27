#include <Arduino.h>
#include "thermometer.h"
#include "stove_fsm.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

volatile float currentTemperature = 18.0f;
float targetTemperature = DEFAULT_TARGET_TEMP;

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void loadTargetTemp() {
    Preferences prefs;
    prefs.begin("thermo", true);
    targetTemperature = prefs.getFloat("target", DEFAULT_TARGET_TEMP);
    prefs.end();
    LOG("Setpoint caricato: " + String(targetTemperature) + "°C");
}
 
void saveTargetTemp() {
    Preferences prefs;
    prefs.begin("thermo", false);
    prefs.putFloat("target", targetTemperature);
    prefs.end();
}

void initThermometer() {
    loadTargetTemp();
    sensors.begin();
}

float readTemperature() {
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C) {
        LOG("Sensore temperatura non trovato!");
        return currentTemperature; // mantieni l'ultimo valore valido
    }
    return t;
}

void taskThermometer(void *pvParameters) {
    for (;;) {
        currentTemperature = readTemperature();

        if (!isTargetReached && currentTemperature >= targetTemperature + TEMP_HYSTERESIS) {
            isTargetReached = true;
            LOG("Temperatura target raggiunta: " + String(currentTemperature) + "°C");
        } else if (isTargetReached && currentTemperature <= targetTemperature - TEMP_HYSTERESIS) {
            isTargetReached = false;
            LOG("Temperatura sotto setpoint: " + String(currentTemperature) + "°C");
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_READ_INTERVAL));
    }
}