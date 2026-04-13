#pragma once
#include <Arduino.h>

extern volatile float currentTemperature;
extern float targetTemperature;

float readTemperature();   // sostituire con lettura reale quando disponibile
void taskThermometer(void *pvParameters);
void initThermometer();