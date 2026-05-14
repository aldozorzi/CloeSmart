# 🔥 CloeSmart: ESP32 Stove Controller for MicroNova boards
An intelligent, Telegram-based stove control system powered by ESP32 and FreeRTOS, written for MicroNova boards (and all boards having a dry contact for an external Thermostat). 
It manages scheduled ignition (Crono), power modulation, and a "kickstart" safety system to prevent accidental shutdowns during maintenance phases.
It can be managed by Telegram app and Telnet connection.

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
   * `status - Check status and timers`
   * `set_temp - Set target temperature`
   * `crono_on - Activate hourly schedule`
   * `crono_off - Deactivate hourly schedule`
   * `set_on - Set ON time`
   * `set_off - Set OFF time`
   

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

#### Component Connections

##### 1.1 Relay Module (Stove Control)
* **VCC (+):** RED wire → **5V** Pin
* **GND (-):** BLACK wire → **G** Pin
* **IN (Signal):** GREEN wire → **GPIO 2**

##### 1.2 Temperature Probe (DS18B20)
* **VCC:** RED wire → **3.3V** Pin
* **GND:** BLACK wire → **G** Pin
* **DATA:** YELLOW wire → **GPIO 4**
* **Note:** A **5.1kΩ pull-up resistor** must be soldered between the **3.3V** pin and **GPIO 4**.

##### 1.3 Capacitive Touch Sensor (TTP223)
* **VCC:** RED wire → **5V** Pin (Shared with Relay)
* **GND:** BLACK wire → **G** Pin (Shared with others)
* **I/O (Signal):** [Chosen Color] wire → **GPIO 1**

#### Pinout Summary Table (Right Side)

Following the physical layout of the ESP32-C3 SuperMini (USB-C port at the top), use this sequence for soldering:

| Physical Pin | Component / Wire | Action |
| :--- | :--- | :--- |
| **5V** | Relay VCC + Touch VCC | Solder both RED wires here |
| **G (GND)** | Relay, Probe, and Touch GND | Twist all BLACK wires and solder here |
| **3.3V** | Probe VCC + 5.1k Resistor leg | Probe RED wire + Resistor leg |
| **4 (GPIO 4)** | Probe DATA + 5.1k Resistor leg | Probe YELLOW wire + Resistor leg |
| **3 (GPIO 3)** | *EMPTY* | Reserved |
| **2 (GPIO 2)** | Relay Signal | Stove trigger (Internal LED linked) |
| **1 (GPIO 1)** | Touch Signal | Manual override switch |
| **0 (GPIO 0)** | *EMPTY* | Reserved |

#### Assembly Best Practices

* **Common Rails:** Since the SuperMini has limited pins, twist the multiple GND (black) and 5V (red) wires together and "tin" them with solder before inserting them into the PCB holes.
* **Resistor Placement:** To keep the build compact, solder the 5.1k resistor flat against the board between the 3.3V and GPIO 4 pins before attaching the probe wires.
* **Mechanical Stability:** * Solder all wires directly to the board (avoid headers/pins) for better resistance to stove vibrations.
    * After testing, apply a drop of **hot glue** over the solder joints to act as "strain relief."
* **Enclosure:** Mount the components on a rigid plastic base (e.g., Polyethylene) using **VHB 5952F tape** or hot glue to ensure the USB-C port remains aligned with the enclosure opening.

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

## 💻 Telnet Interface

You can connect to the device's remote shell via Telnet on port 23:

```bash
telnet 192.168.1.100 23
```
Upon a successful connection, the server will display an interactive banner listing all available commands and system status options.


------

*Enjoy your smart heat!* 🔥