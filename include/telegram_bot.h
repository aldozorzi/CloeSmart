// telegram_bot.h
#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

extern WiFiClientSecure securedClient;
extern UniversalTelegramBot bot;

void taskTelegram(void *pvParameters);