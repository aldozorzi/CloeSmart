#include <Arduino.h>
#include "thermometer.h"
#include "stove_fsm.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Definizione variabili (dichiarate extern in thermometer.h) ---
volatile float currentTemperature = 18.0f;
float targetTemperature = DEFAULT_TARGET_TEMP;

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void initThermometer() {
    sensors.begin();
}

// --- Lettura temperatura ---
// ⚠️ MOCKUP: sostituire il corpo di questa funzione con la lettura reale
// Esempi:
//   DS18B20  → sensors.getTempCByIndex(0)
//   DHT22    → dht.readTemperature()
//   NTC      → conversione da analogRead(TEMP_PIN)
float readTemperature() {
    // Simula una temperatura che sale lentamente se la stufa è accesa
    // e scende se è spenta — utile per testare le transizioni FSM
    static float mockTemp = 18.0f;

    if (isStoveEnabled && currentStoveState != STATE_OFF && currentStoveState != STATE_MODULATING) {
        mockTemp += 0.2f;  // Stufa accesa: temperatura sale
    } else {
        mockTemp -= 0.1f;  // Stufa spenta o in modulazione: temperatura scende
    }

    // Clamp tra valori realistici
    mockTemp = constrain(mockTemp, 10.0f, 35.0f);
    return mockTemp;
}

/*float readTemperature() {
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C) {
        LOG("Sensore temperatura non trovato!");
        return currentTemperature; // mantieni l'ultimo valore valido
    }
    return t;
}*/

// --- TASK 3: TEMPERATURE MANAGEMENT ---
void taskThermometer(void *pvParameters) {
    for (;;) {
        currentTemperature = readTemperature();

        // Isteresi: evita oscillazioni continue attorno al setpoint
        // isTargetReached va a true quando supera target + isteresi
        // torna false solo quando scende sotto target - isteresi
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