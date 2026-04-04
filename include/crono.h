// crono.h
#pragma once
#include <Arduino.h>
#include <Preferences.h>

extern String cronoOnTime;
extern String cronoOffTime;
extern bool isCronoEnabled;

void loadCronoSettings();
void saveCronoSettings();
bool isValidTime(String t);