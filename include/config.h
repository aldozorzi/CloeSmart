#pragma once

// --- PIN ---
const int RELAY_PIN       = 2;   // relay control pin
const int TEMP_PIN        = 4;   // sensor pin
const int  BUTTON_PIN          = 1; // button pin (GPIO1 is also TX, but we won't use Serial1)
const int  BUTTON_HOLD_MS      = 2000;   // ms to trigger long press action
const int  BUTTON_DEBOUNCE_MS  = 50;     // anti-bounce delay

// --- TIMING (milliseconds) ---
const uint32_t AUTO_OFF_DELAY      = 10 * 60000;  // 10 minutes
const uint32_t SAFETY_MARGIN       =  2 * 60000;  //  2 minutes before auto-off
const uint32_t KICKSTART_DURATION  =      30000;  // 30 seconds pulse

// --- STATIC NETWORK CONFIG ---
#include <IPAddress.h>
const IPAddress LOCAL_IP      (192, 168,   1, 184);
const IPAddress GATEWAY       (192, 168,   1,   1);
const IPAddress SUBNET        (255, 255, 255,   0);
const IPAddress PRIMARY_DNS   (  8,   8,   8,   8);
const IPAddress SECONDARY_DNS (  8,   8,   4,   4);

// thermometer
const float DEFAULT_TARGET_TEMP = 21.0f;  // °C
const float TEMP_HYSTERESIS     =  1.0f;  // margin to avid oscillations
const int   TEMP_READ_INTERVAL  = 5000;   // reading every X milliseconds

// --- DEBUG ---
#define CLOE_DEBUG  1
#if CLOE_DEBUG 
  #define LOG(x) Serial.println(x)
#else
  #define LOG(x)
#endif