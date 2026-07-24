ESP32-GSM-MQTT-Smart-Relay
Advanced ESP32-C3 Smart Relay Controller
RF433 Remote · GSM/SMS Control · MQTT over GPRS · Rule Engine · Web Panel · OTA Updates

Version
Framework
RTOS
License

<!-- TODO: Add a real hardware image here once available --><!-- ![Hardware Setup](images/device.jpg) -->
📖 Overview
The ESP32-GSM-MQTT-Smart-Relay is a feature-rich ESP32 firmware for a 4-channel smart relay system. It is designed to operate independently of local internet infrastructure by utilizing a SIM800 module for SMS control and MQTT communication over GPRS.

Configuration and local management are handled via a built-in SoftAP and an embedded Single Page Application (SPA) Web UI. All settings, rules, and scenarios are persistently stored in the ESP32's NVS (Non-Volatile Storage).

Ideal Use Cases:

Remote agriculture and greenhouse automation (RF Sensors + Rule Engine).
Locations with unstable or no WiFi (Control via SMS/GPRS).
Integration with Home Assistant or custom cloud servers via MQTT.
Lighting, pump, door, and HVAC control.
🎓 Engineering Skills Demonstrated
This project architecture highlights the following embedded engineering competencies:

Embedded C/C++: Efficient memory and resource management on constrained hardware.
RTOS Multitasking: FreeRTOS tasks, queues, and mutexes for non-blocking system design.
Hardware Communication: UART programming, AT command handling (SIM800), and interrupt-driven RF decoding.
Networking Protocols: Custom MQTT over GPRS implementation and RESTful HTTP API design.
IoT Architecture: OTA firmware updates, NVS data persistence, and edge automation (Rule Engine).
✨ Features
⚡ 4-Channel Relay Control: Supports ON/OFF, Toggle, and precise Timers (seconds). Configurable Active HIGH/LOW.
📻 RF433 Integration: Remote button learning, multi-click detection (single, double, triple, long press), and multi-button combinations.
🌡️ RF Sensor Decoding: Supports decoding RF sensors for temperature, humidity, battery percentage, voltage, and distance.
🧠 Built-in Rule Engine (Automation): Create complex automations using AND/OR conditions based on time, days of the week, RF sensors, or relay states (includes hysteresis and cooldown logic).
🎬 Scenarios (Macros): Sequential multi-step actions with daily time triggers.
📱 GSM & SMS Control: Whitelist-based security. Send commands to control relays or get system status. Supports soft/hard modem resets.
🌐 MQTT over GPRS: Full MQTT client running on the SIM800 cellular data (not WiFi). Publish status/logs and subscribe to command topics.
💻 Embedded Web Panel: Built-in SoftAP hosting a responsive Web UI for full system configuration without writing any code.
🔄 OTA Updates: Over-The-Air firmware updates via WiFi directly from a custom server.
🕒 Network Time Sync: Synchronizes time using the AT+CCLK command from the local cellular network.
📝 Event Logging: Ring-buffer event logging stored in memory and published to MQTT.
🛠️ Hardware & Pinout
Recommended Components
MCU: ESP32-C3 Super Mini
Relays: 4-Channel Relay Module (5V)
RF: 433 MHz Receiver (e.g., MX-RM-5V or RXB6)
GSM: SIM800L / SIM800C + Antenna + SIM Card
Misc: Status LED
Pin Mapping (ESP32-C3)
Function	GPIO
Relay 1 .. 4	5, 6, 9, 10
RF RX Data	4 (Interrupt driven)
SIM800 Reset	7
Status LED	1
SIM800 RX (ESP TX)	Serial1 TX = 3
SIM800 TX (ESP RX)	Serial1 RX = 2
⚠️ Hardware Note: The SIM800 module can draw peaks up to 2A during network registration. Ensure it is powered by a dedicated step-down converter (like LM2596) with a large decoupling capacitor. Common ground with the ESP32 is mandatory.

🏗️ Software Architecture (FreeRTOS)
The system is heavily multi-threaded, utilizing FreeRTOS to handle asynchronous tasks, queues, and mutexes to ensure stability without blocking the main loop.

Task	Priority	Role
Task_RF	High	Captures and decodes RF 433MHz interrupts.
Task_GSM	Medium	Handles SIM800 AT commands, SMS parsing, and line recovery.
Task_MQTT	Medium	Manages MQTT client over GPRS data connection.
Task_Scene	Low	Evaluates the Rule Engine, automations, and event queues.
Task_Save	Low	Asynchronously writes configuration changes to NVS.
Task_Web	Low	Serves the HTTP API and Web UI.
text

                    ┌─────────────┐
   RF 433 ─────────►│  Task_RF    │──► Scene / Relay Logic
                    └─────────────┘
   SMS/GPRS ───────►│  Task_GSM   │──► SMS Commands
                    │  Task_MQTT  │──► MQTT Broker
                    └─────────────┘
   SoftAP/HTTP ────►│  WebServer  │──► REST API + Web UI
                    └─────────────┘
                         │
                    NVS (Preferences)
🚀 Installation & Build
This project is built using PlatformIO.

Clone the repository:
Bash

git clone https://github.com/YOUR_USERNAME/ESP32-GSM-MQTT-Smart-Relay.git
cd ESP32-GSM-MQTT-Smart-Relay
Build the project:
Bash

pio run
Upload to ESP32-C3:
Bash

pio run -t upload
Open Serial Monitor (115200 baud):
Bash

pio device monitor
⚙️ Configuration & Security
Before compiling for a real-world deployment, modify lib/include/config.h to change default credentials:

C++

// --- SECURITY SETTINGS ---
#define AP_SSID "SmartRelay_Pro"
#define AP_PASS "YOUR_SECURE_WIFI_PASSWORD"

#define ADMIN_USER "admin"
#define ADMIN_PASS "YOUR_SECURE_WEBUI_PASSWORD"

// --- OTA SETTINGS ---
#define CURRENT_VERSION "1.0.0"
#define SERVER_IP "YOUR_OTA_SERVER_IP_OR_DOMAIN"
Pre-Publication Security Checklist:
If you are forking or deploying this project, ensure you have checked the following:

 Change the default SoftAP password.
 Change the default Web UI admin credentials.
 Populate the SMS Whitelist (if empty, anyone can control the device via SMS).
 Do not hardcode your MQTT Broker credentials; configure them via the Web UI so they are stored securely in NVS.
 Scan the repository for leftover sensitive data (passwords, IPs, tokens) using grep -r "password\|token\|ip" ..
📱 Usage
Initial Setup
Power up the device.
Connect your phone/PC to the WiFi network: SmartRelay_Pro.
Navigate to http://192.168.4.1 in your browser.
Login with default credentials and configure your Relays, SMS Whitelist, and MQTT Broker.
SMS Commands
(Only accepted from whitelisted numbers)

Command	Action
STATUS or ?	Returns brief system status.
?!	Returns detailed status (Relays, Sensors, Automations).
ON1 ... ON4	Turns on the specified relay.
OFF1 ... OFF4	Turns off the specified relay.
R1 30s	Turns on Relay 1 for 30 seconds (Timer).
ALL ON / ALL OFF	Controls all relays simultaneously.
SCENE1	Executes Scenario/Macro #1.
MQTT over GPRS
MQTT communication is handled via the SIM800 module.

Command Topic: device/SmartRelay_001/cmd (Accepts: R1:ON, R2:OFF, STATUS, RESTART)
Status Topic: device/SmartRelay_001/status (Publishes JSON status)
Log Topic: device/SmartRelay_001/log (Publishes system events)
📁 Project Structure
text

├── platformio.ini          # PlatformIO config & OTA partition table
├── src/
│   ├── main.cpp            # Boot, Task Initialization, Routing
│   ├── globals.cpp         # Global state management
│   ├── relay.cpp           # Relay control logic
│   ├── rf.cpp              # RF decoding & debounce logic
│   ├── automation.cpp      # Rule Engine processing
│   ├── scene.cpp           # Macro/Scenario execution
│   ├── gsm.cpp             # SIM800 SMS & Line Auth
│   ├── SIM800_MQTT.cpp     # Custom MQTT over AT Commands layer
│   ├── webapi.cpp          # HTTP REST Handlers
│   ├── ota.cpp             # HTTP OTA Update logic
│   ├── storage.cpp         # NVS Save/Load operations
│   └── tasks.cpp           # FreeRTOS task loops
└── lib/
    ├── include/            # config.h and headers
    └── SIM800_Arduino/     # Low-level SIM800 driver
🔄 OTA (Over-The-Air) Updates
The system supports Dual-Partition OTA updates (app0 and app1).
To update:

Connect the ESP32 to a local WiFi network with internet access via the Web UI.
Trigger the update via the Web UI button or API (POST /api/ota/check).
The ESP32 will fetch the latest .bin file from your configured SERVER_IP, flash it to the inactive partition, and reboot safely.
📄 License
This project is licensed under the MIT License - see the LICENSE file for details.

If you find this project interesting or have suggestions for the architecture, feel free to open an issue or connect!