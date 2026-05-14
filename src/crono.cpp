#include "crono.h"
#include "config.h"

String cronoOnTime  = "07:00";
String cronoOffTime = "22:00";
bool isCronoEnabled = true;



void loadCronoSettings() {
    Preferences prefs;
    if (!prefs.begin("crono", false)) {
        cronoOnTime    = "07:00";
        cronoOffTime   = "22:00";
        isCronoEnabled = true;
        return;
    }
    cronoOnTime    = prefs.isKey("on")      ? prefs.getString("on",      "07:00") : "07:00";
    cronoOffTime   = prefs.isKey("off")     ? prefs.getString("off",     "22:00") : "22:00";
    isCronoEnabled = prefs.isKey("enabled") ? prefs.getBool  ("enabled", true)    : true;
    prefs.end();
}

void saveCronoSettings() {
    Preferences prefs;
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