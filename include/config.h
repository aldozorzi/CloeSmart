#pragma once

// --- PIN ---
const int RELAY_PIN = 26;

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

// --- DEBUG ---
#define CLOE_DEBUG  1

#if CLOE_DEBUG 
  #define LOG(x) Serial.println(x)
#else
  #define LOG(x)
#endif