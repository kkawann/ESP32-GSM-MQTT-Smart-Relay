# ESP32-GSM-MQTT-Smart-Relay

An advanced, feature-rich 4-channel smart relay system based on the **ESP32-C3 Super Mini**. It is designed to operate independently of local WiFi infrastructure by utilizing a **SIM800 module** for GPRS-based MQTT connectivity and SMS control. 

This repository is formatted as a standardized, compile-ready **PlatformIO** project. Anyone can download, import, and build this project with a single click.

---

## 📖 English Version

### ✨ Key Features
* **4-Channel Relay Control:** Supports ON/OFF, Toggle, and high-precision customizable timers. Configurable Active HIGH/LOW states.
* **RF 433 MHz Integration:** Learns RF remote buttons with multi-press decoding (single, double, triple, and long-press) and multi-button combination macros.
* **RF Sensor Decoding:** Decodes wireless sensors (temperature, humidity, voltage, distance, battery percentage, etc.) at 433 MHz.
* **Built-in Rule Engine (Edge Automation):** Complex automation scripts using AND/OR logic criteria based on time, days of week, sensor thresholds, or relay states (supporting hysteresis and cooldown logic).
* **Flexible Scenarios (Macros):** Sequential multi-step action sequences with scheduled daily triggers.
* **GSM & SMS Control:** Whitelist-based access security. Controls relays or queries status via standard text messages.
* **MQTT over GPRS:** Entirely runs custom MQTT client over cellular GPRS data using SIM800 AT commands (completely independent of WiFi).
* **Embedded SPA Web Panel:** Built-in SoftAP hosting a responsive, beautiful Single-Page Application (SPA) for full configuration without needing to alter code.
* **Over-The-Air (OTA) Updates:** Secure, dual-partition safe wireless firmware updates (utilizing `app0` and `app1` virtual paths).
* **Time Sync:** Automatic clock synchronisation using the local cellular provider network (via `AT+CCLK`).

### 🛠️ Hardware Pinout & Specs (ESP32-C3)
* **ESP32-C3 Super Mini** is used as the core MCU.
* **GSM/GPRS Module:** SIM800L or SIM800C.
* **RF Module:** MX-RM-5V or RXB6 receiver.

| Function | GPIO | Notes |
|---|---|---|
| **Relays 1 - 4** | `5`, `6`, `9`, `10` | Relay control outputs |
| **RF RX Pin** | `4` | Interrupt-driven RF decoder |
| **SIM800 Reset**| `7` | Hard reset pins for modem rescue |
| **Status LED** | `1` | Blinks for network activity/SoftAP |
| **SIM800 RX (ESP TX)** | `3` (`Serial1` TX) | Hardware UART communication |
| **SIM800 TX (ESP RX)** | `2` (`Serial1` RX) | Hardware UART communication |

> **⚠️ Hardware note:** The SIM800 module can draw up to **2A peaks** during network registration. A dedicated high-current power supply (e.g. LM2596 step-down) with a large decoupling capacitor is required. A common ground with the ESP32 is mandatory.

### 🚀 Direct Build & Run with PlatformIO
This project is configured out-of-the-box for **VS Code + PlatformIO**.

1. **Clone the repository:**
   ```bash
   git clone https://github.com/kkawann/ESP32-GSM-MQTT-Smart-Relay.git
   ```
2. **Open the folder in VS Code** (PlatformIO will automatically detect the project and install all dependencies).
3. **Build the firmware:**
   Press `Ctrl + Alt + B` or click the checkmark icon in the PlatformIO toolbar.
4. **Upload to Hardware:**
   Connect your ESP32-C3 Super Mini via USB and click the Arrow icon (`pio run -t upload`).
5. **Open Serial Monitor:**
   Configure terminal speed to `115200` to monitor logs in real-time.

---

## 📖 معرفی به زبان فارسی (Persian Version)

پروژه **ESP32-GSM-MQTT-Smart-Relay** یک فریمور پیشرفته و جامع برای میکروکنترلر **ESP32-C3 Super Mini** است که به منظور کنترل یک سیستم رله هوشمند ۴ کاناله طراحی شده است. نقطه قوت اصلی این پروژه، **استقلال کامل از زیرساخت‌های اینترنت محلی (مانند WiFi خانگی)** است؛ به این معنی که دستگاه با استفاده از ماژول cellular **SIM800** هم از طریق پیامک (SMS) و هم از طریق پروتکل MQTT بر روی شبکه داده موبایل GPRS قابل پایش و کنترل است.

### 💡 قابلیت‌های برجسته سیستم
۱. **کنترل هوشمند رله‌ها:** ۴ کانال با پشتیبانی از منطق روشن، خاموش، Toggle و تایمر دقیق به همراه تعریف وضعیت جریان فعال (Active HIGH/LOW).
۲. **سیستم ریموت ۴۳۳ مگاهرتز:** قابلیت یادگیری دکمه‌های ریموت و تشخیص انواع کلیک‌ها (تک‌کلیک، دابل، سه‌کلیک و نگه‌داشتن لانگ) به همراه دیکود سنسورهای RF محیطی (دما، رطوبت، ولتاژ و باتری).
۳. **موتور اتوماسیون داخلی (Rule Engine):** ایجاد سناریوهای پیچیده شرطی AND/OR بر اساس خطاهای زمان، وضعیت رله‌ها، روزهای هفته و داده‌های سنسورها؛ شامل منطق‌های هیسترزیس و دوره‌های Cooldown برای پایداری رله‌ها.
۴. **ارتباط بی سیم ۲ طرفه:** ارسال فرامین کنترل رله و دریافت وضعیت کلی سیستم به وسیله پیامک (با امنیت لیست سفید Whitelist) یا کلاینت دائمی تحت شبکه MQTT بر روی سیم‌کارت.
۵. **پنل وب اختصاصی امبد شده:** راه‌اندازی و سرو وب‌پنل تک‌صفحه‌ای (SPA) بسیار شکیل از روی حافظه فلاش دستگاه پس از اتصال به SoftAP برای پیکربندی تمام لایه‌ها، بدون نیاز به هاردکد کردن مقادیر.
۶. **پشتیبانی از OTA ایمن:** آپدیت فریمور به صورت بی‌سیم از روی سرور با استفاده از سیستم دو پارتیشنه جابجای فعال (`app0` و `app1`) بدون ریسک خاموش شدن یا خرابی دستگاه.

### 🗂️ معماری نرم‌افزار (FreeRTOS Multitasking)
یک نرم‌افزار صنعتی و مقاوم با معماری مالتی‌تسکینگ روی هسته **FreeRTOS** که از صف‌ها (Queues) و موتکس‌ها (Mutexes) برای ارتباط غیراثرگذار و همزمان تسک‌ها استفاده می‌کند:

* **Task_RF (High Priority):** پردازش سریع و غیربلاکینگ داده‌های ورودی گیرنده رادیویی.
* **Task_GSM (Medium Priority):** مدیریت دستورات AT ماژول SIM800، دریافت و تفکیک پیامک‌ها و احیای خودکار شبکه در صورت قطعی سیگنال.
* **Task_MQTT (Medium Priority):** اتصال دائم به سرور کارگزاری مکیوتت روی بستر دیتا GPRS.
* **Task_Scene (Low Priority):** پایش سناریوهای شرطی و ماشین قوانین موتور اتوماسیون.
* **Task_Save (Low Priority):** نوشتن تغییرات پیکربندی در حافظه غیرفرار NVS به صورت نامتقارن.
* **Task_Web (Low Priority):** پاسخ به کدهای API و سرو پنل تنظیمات SoftAP.

---

### 📂 Project Structure / ساختار فایل‌ها

```text
├── platformio.ini          # PlatformIO configurations & OTA partition metadata
├── partitions.csv          # 4MB ESP32-C3 partition scheme
├── src/                    # Source code (.cpp)
│   ├── main.cpp            # App startup, routing, and task initialization
│   ├── globals.cpp         # Global structures & state handles
│   ├── relay.cpp           # Relay control actions
│   ├── rf.cpp              # RF interrupt decoding & remote learn state
│   ├── automation.cpp      # Edge automation engine (Rule evaluator)
│   ├── scene.cpp           # Macros & scene script executors
│   ├── gsm.cpp             # Cellular SIM800 SMS & network layers
│   ├── SIM800_MQTT.cpp     # AT command-based MQTT implementation
│   ├── webapi.cpp          # Static SoftAP webserver API routing
│   ├── ota.cpp             # Web-based Over-The-Air safe flashing
│   ├── storage.cpp         # NVS read/write configurations
│   ├── tasks.cpp           # FreeRTOS continuous system loops
│   └── utils.cpp           # General string & formatting utilities
└── lib/
    └── include/            # C++ headers (.h) & default configs
```
