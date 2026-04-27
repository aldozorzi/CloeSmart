#pragma once
#include <Arduino.h>

extern volatile float currentTemperature;
extern float targetTemperature;

float readTemperature();
void loadTargetTemp();
void saveTargetTemp();
void taskThermometer(void *pvParameters);
void initThermometer();