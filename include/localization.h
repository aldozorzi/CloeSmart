// localization.h - Italian Version
#ifndef LOCALIZATION_H
#define LOCALIZATION_H

// --- Messaggi di Sistema ---
#define MSG_SYSTEM_ONLINE    "🤖 **Sistema Cloe Online**\nController riavviato correttamente.\nLa stufa è spenta ❄️"

// --- Comandi e Stato ---
#define CMD_ON               "/accendi"
#define CMD_OFF              "/spegni"
#define CMD_STATUS           "/stato"

// --- Risposte del Bot ---
#define RESP_STOVE_ON        "🔥 Comando inviato: La stufa si sta accendendo"
#define RESP_STOVE_OFF       "❄️ Comando inviato: La stufa si sta spegnendo"
#define RESP_STATUS_HEADER   "Stufa "
#define ST_LABEL_OFF         "spenta ❄️"
#define ST_LABEL_WORK        "accesa (Lavoro) 🔥"
#define ST_LABEL_MOD         "accesa (Modulazione) 📉"
#define ST_LABEL_KICK        "accesa (Kickstart) 📉"
#define ST_THERMO            "🌡 Temp: %TMP%°C / Target: %TGT%°C"
#define RESP_TEMP_UPDATED     "🌡 Setpoint aggiornato a "
#define RESP_TEMP_INVALID     "⚠️ Temperatura non valida. Usa un valore tra 10° e 30°C"
#define RESP_TEMP_HELP        "Usa: /set_temp XX.X (es. /set_temp 21.5)"

#define MSG_UNAUTHORIZED     "⚠️ Accesso negato. Non sei nella lista utenti.\nIl tuo ID: "
#define MSG_REQ_SENT         "Richiesta di autorizzazione inviata al proprietario 📨"
#define MSG_WAIT_ADMIN       "Attendi che il proprietario approvi la tua richiesta."

#define ADMIN_NOTIF_REQ      "🔔 *Nuova richiesta di accesso*\nUtente: "
#define ADMIN_NOTIF_ID       "\nID: "
#define ADMIN_BTN_AUTH       "Autorizza ✅"
#define ADMIN_AUTH_SUCCESS   "Utente autorizzato con successo e salvato in memoria! 💾"

#define USER_GRANTED         "🎉 Ottime notizie! Il proprietario ha accettato la tua richiesta. Ora puoi usare il comando /menu"

// CRONO
#define TIMEZONE                    "Europe/Rome"
#define MSG_CHRONO_STATUS           "📅 *Stato Crono*\n"
#define MSG_CHRONO_ON               "Crono: ATTIVO ✅"
#define MSG_CHRONO_OFF              "Crono: DISATTIVATO ❌"
#define MSG_CHRONO_SETTINGS         "\nOrari: "
#define RESP_CHRONO_UPDATED         "✅ Orario aggiornato correttamente!"
#define RESP_CHRONO_INVALID         "⚠️ Formato errato! Usa %CMD% HH:MM (es. %CMD% 07:30)"
#define RESP_CHRONO_INVALID_SAME    "⚠️ Orario di inizio e orario di fine non possono essere uguali"
#define RESP_CHRONO_HELP            "🕒 Usa: `%CMD% HH:MM` (es: `%CMD% 07:00` - clicca per copiare)"

#endif