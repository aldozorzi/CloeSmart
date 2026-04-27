#include "crono.h"
#include "config.h"

String cronoOnTime  = "07:00";
String cronoOffTime = "22:00";
bool isCronoEnabled = true;

Preferences prefs;

void loadCronoSettings() {
    prefs.begin("crono", true);
    cronoOnTime     = prefs.getString("on",      "07:00");
    cronoOffTime    = prefs.getString("off",     "22:00");
    isCronoEnabled  = prefs.getBool  ("enabled", true);
    prefs.end();
}

void saveCronoSettings() {
    prefs.begin("crono", false);
    prefs.putString("on",      cronoOnTime);
    prefs.putString("off",     cronoOffTime);
    prefs.putBool  ("enabled", isCronoEnabled);
    prefs.end();
}

bool isValidTime(String t) {
    if (t.length() != 5 || t.charAt(2) != ':') return false;
    int hh = t.substring(0, 2).toInt();
    int mm = t.substring(3, 5).toInt();
    if (hh < 0 || hh > 23) return false;
    if (mm < 0 || mm > 59) return false;
    return true;
}