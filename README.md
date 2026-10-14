# 🪄 SmartLamp — MagicBox

> ESP32-S3 smart ambient lamp with round GC9A01 display, SK6812 RGBW LED strip, environmental sensors, WiFi portal, OTA updates and Telegram notifications.

---

## ✨ Features

| Feature | Details |
|---|---|
| 🖥️ **Round display** | 240×240 GC9A01 via LovyanGFX, double-buffered 60fps sprite canvas |
| 💡 **LED animations** | Warm Cozy, Rainbow Flow, Aurora Wave, Cyber Runner, Fire Glow, Off |
| 🌡️ **Climate sensors** | SHT31 (temp + humidity, CRC-8 validated) + BMP280 (pressure, barometric forecast) |
| 📡 **WiFi captive portal** | Auto AP + captive portal for first-time setup, mDNS `magicbox.local` |
| 🌐 **Web dashboard** | Real-time sensor data, LED mode/brightness control, message push |
| 📲 **Telegram messages** | Push messages via `/api/msg` endpoint, shown on display with pulsing ring |
| ♻️ **OTA updates** | ArduinoOTA (wireless) + Web OTA (`/update`) with magic-byte `0xE9` validation |
| 🔆 **Auto brightness** | LDR-based smooth auto brightness for both LCD and LED strip |
| 💾 **NVS persistence** | Settings saved to flash with 5 s debounce (wear protection) |
| ⚡ **Adaptive FPS** | 60 fps during interaction → 4 fps idle (core sleep, energy saving) |
| 🌙 **Eco WiFi mode** | Auto radio-off after 3 min idle AP (hold Left button to wake) |

---

## 🗂️ Architecture

```
Core 0  ──► NetworkService (WiFi / NTP / OTA / PortalServer)
            SensorService  (SHT31 non-blocking + BMP280)

Core 1  ──► InputService   (capacitive touch buttons, debounced)
            LedService     (SK6812 animations + auto-brightness)
            DisplayService (sprite rendering, adaptive FPS)
            processEvents  (AppState event queue dispatcher)
```

All services share a single `AppState` struct with FreeRTOS critical sections for thread-safe cross-core access. Communication uses a lightweight `QueueHandle_t` event bus.

---

## 🖥️ Screen Modes

| Screen | Description |
|---|---|
| **CLIMATE** | Temperature, humidity arc gauge, pressure pill, brightness bar, time, LED mode |
| **BAROMETER** | Full-dial barometric gauge with weather forecast label |
| **TELEGRAM** | Message card with sender, 2-line text wrap, pulsing new-message ring |
| **NETWORK** | Auto QR code (WiFi join → web panel), IP address display when connected |

---

## 🔘 Button Controls

| Button | Short Press | Long Press (900 ms) |
|---|---|---|
| **Left** | Next screen | Start AP captive portal |
| **Top** | Next LED mode (or mark Telegram read) | Toggle LED on/off |
| **Right** | Cycle brightness (Auto → Low → Mid → High) | Test LED notify |

---

## 🛠️ Hardware

| Component | Part |
|---|---|
| MCU | ESP32-S3-DevKitC-1 N16R8 |
| Display | 1.28" GC9A01 round TFT 240×240 |
| LED strip | SK6812 RGBW (60 LEDs) |
| Temp/Humidity | SHT31 (I²C 0x44/0x45) |
| Pressure | BMP280 (I²C 0x76/0x77) |
| Ambient light | LDR on ADC1_CH0 (GPIO1) |
| Touch buttons | Capacitive pads on GPIO 4 / 6 / 7 |

---

## 📦 Pin Mapping

```
GPIO  1  — LDR (ADC)
GPIO  4  — Touch Left
GPIO  5  — SK6812 DIN
GPIO  6  — Touch Top (lid)
GPIO  7  — Touch Right
GPIO  8  — I²C SDA
GPIO  9  — I²C SCL
GPIO 10  — TFT CS
GPIO 11  — TFT MOSI
GPIO 12  — TFT SCLK
GPIO 13  — TFT DC
GPIO 14  — TFT RST
```

---

## 🚀 Getting Started

### 1. Clone & open
```bash
git clone https://github.com/<you>/SmartLamp.git
cd SmartLamp
```

### 2. Build & flash (PlatformIO)
```bash
pio run --target upload
pio device monitor
```

### 3. First boot
1. Device creates AP **`MagicBox-Setup`** and shows QR on display
2. Connect phone to that AP → captive portal opens automatically
3. Pick your WiFi network, enter password → device reboots and connects
4. Access web dashboard at **`http://magicbox.local/`** or via IP shown on display

### 4. Web API

| Endpoint | Method | Description |
|---|---|---|
| `/api/status` | GET | JSON: temp, humidity, pressure, brightness, LED mode |
| `/api/cmd?do=mode` | POST | Next LED mode |
| `/api/cmd?do=brt` | POST | Cycle brightness |
| `/api/msg` | POST | Push message (`sender=`, `msg=`) |
| `/api/scan` | GET | WiFi scan results JSON |
| `/update` | POST | Web OTA firmware upload |

---

## 📚 Libraries

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) `^1.1.16`
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) `^1.12.3`
- [Adafruit SHT31](https://github.com/adafruit/Adafruit_SHT31) `^2.2.2`
- [Adafruit BMP280](https://github.com/adafruit/Adafruit_BMP280_Library) `^2.6.8`

---

## 📄 License

MIT — do whatever you want, just keep the magic alive. ✨

