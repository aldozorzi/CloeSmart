# 🔥 CloeSmart: ESP32 Stove Controller for MicroNova boards
An intelligent, Telegram-based stove control system powered by ESP32 and FreeRTOS, written for MicroNova boards (and all boards having a dry contact for an external Thermostat). 
It manages scheduled ignition (Crono), power modulation, and a "kickstart" safety system to prevent accidental shutdowns during maintenance phases.

## 🚀 Key Features
* **FSM (Finite State Machine):** Robust state management including `STATE_OFF`, `STATE_WORK`, `STATE_MODULATING` and `KICKSTART`.
* **Advanced Crono:** Daily scheduling for power ON/OFF saved in non-volatile memory via ESP32 Preferences.
* **Anti-Cooldown Safety:** Automatic "Pulse" system during modulation to keep the stove active without overheating.
* **Multi-Core Execution:** Asynchronous handling with Core 0 dedicated to Telegram/WiFi and Core 1 for physical relay control.
* **Access Security:** Whitelist system with real-time authorization requests sent to the administrator.

---

## 🛠 Initial Setup

### 1. Secret Management
The project uses a `secrets.h` file for sensitive credentials.
1. Locate `include/secrets.h.example`.
2. **Duplicate it** and rename it to `include/secrets.h`.
3. Fill in your WiFi credentials and your Telegram Bot Token.

### 2. Localization
The system supports multi-language support via `include/localization.h`. 

### 3. Firmware Configuration
Before flashing the ESP32, you **must** review the `include/config.h`. These parameters act as the core logic of the system and should be adjusted to match your specific hardware and stove model:

* **RELAY_PIN**: Default is `2`. Ensure this matches the physical GPIO connected to your relay module.
* **TEMP_PIN**: Default is `4`. Ensure this matches the physical GPIO connected to your temp sensor.
* **SWITCHER_PIN**: Default is `1`. Ensure this matches the physical GPIO connected to your physical switcher.
* **AUTO_OFF_DELAY**: Default is `10 * 60000` (10 minutes). This is the "Modulation" timeout of your stove. **Verify this value on your stove's technical menu** (often called "Standby Delay" or "Mantenimento") and adjust accordingly.
* **KICKSTART_DURATION**: Default is `30000` (30 seconds). This defines how long the relay closes to "wake up" the stove and reset its internal shutdown timer.
* **Network Parameters**: If you are using a Static IP, verify the `local_IP`, `gateway`, and `subnet` to ensure they are compatible with your home router's range. If you want to use these parameters, go to setup() and uncomment

```cpp
/*
if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
}
*/
```
Otherwise, the controller will connect in DHCP mode.

## 🤖 Bot Creation & Security

### Creation via @BotFather
1. Search for **@BotFather** on Telegram.
2. Send `/newbot` and follow the instructions to get your **API Token**.
3. Use `/setcommands` to configure the quick menu:
   * `on - Turn on the stove`
   * `off - Turn off the stove`
   * `crono_on - Activate hourly schedule`
   * `crono_off - Deactivate hourly schedule`
   * `set_on - Set ON time`
   * `set_off - Set OFF time`
   * `status - Check status and timers`

### Retrieve your Chat ID (Secure Method)
Do not use third-party "ID bots". Use the official Telegram API via your browser:
1. Open a chat with your new bot and press **Start**.
2. Write a message
3. Paste this URL into your browser: 
   `https://api.telegram.org/bot<YOUR_BOT_NAME>/getUpdates`
4. Look for the `"id":` field inside the `"from"` object. That is your **CHAT_ID_VAL** for `secrets.h`.

---

## 🏗 Installation (PlatformIO)
1. Clone or download this repository 
2. Ensure your `.gitignore` includes `.pio/` and `.vscode/`.
3. Open the folder with VS Code + PlatformIO.
4. The `platformio.ini` file will automatically handle dependencies:
   * `UniversalTelegramBot`
   * `ArduinoJson`
   * `ezTime`

---

## 🔌 Hardware Connection & Logic

### 1. Connection Schema
The system acts as an Intermediary: **ESP32 -> Relay -> External Thermostat Port**.
* Connect **ESP32 PIN 2** to the Relay module signal input.
* Connect **ESP32 GND** to the Relay module ground (GND) to ensure a common reference.
* Connect the Relay's **Normally Open (NO)** and **Common (COM)** terminals to the stove's external thermostat input (dry contact).
* Connect OneWire temp sensor to pin 4 (optional but if you don't, comment initThermometer() and taskThermometer in setup() in main.cpp)
* Connect switcher to pin 1 (optional but if you don't, comment initSwitcher() in setup() in main.cpp)

### 2. Stove Configuration (Master/Slave Logic)
To allow the ESP32 to take full control, the stove must be configured as follows:
* **Target Temperature:** Set to the minimum (e.g., **7°C**).
* **Standby Mode:** Must be set to **ON**.
* **Internal Chrono:** Must be **DISABLED**.

Check your stove manual to be sure these params are valid for your stove.

### 3. Operational Behavior
Once configured, the ESP32 dictates the stove's behavior via the relay:
* **Relay Open (Stove OFF/Modulating):** The stove enters "Modulation" mode because the low target temperature (7°C) is reached. After the timeout set in the technical settings (typically **10 minutes**), the stove will perform its shutdown cycle.
* **Relay Closed (Stove ON/Work):** The ESP32 overrides the internal thermostat, forcing the stove into "Work" mode. From this moment, the ESP32 logic (including the Anti-Shutdown Pulse) manages the state.

------

*Enjoy your smart heat!* 🔥