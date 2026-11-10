# 🪄 SmartLamp — MagicBox (ESP32-S3 Sovereign Edition)

*Автономний медіахаб, прецизійний мікроклімат, I2S-аудіо та розумне RGBW-освітлення на базі ESP32-S3 і FastAPI.*

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/RTOS-FreeRTOS%20Dual--Core-blue.svg?style=flat-square" alt="FreeRTOS" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/Audio-MAX98357A%20I2S%20DMA-yellow.svg?style=flat-square" alt="Audio" />
  <img src="https://img.shields.io/badge/LED-SK6812%20RGBW%2060px-E07A5F.svg?style=flat-square" alt="LED" />
  <img src="https://img.shields.io/badge/Spotify-Vinyl%20%26%20AmbiLight-1DB954.svg?style=flat-square&logo=spotify" alt="Spotify" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20uvloop-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/License-MIT-green.svg?style=flat-square" alt="License" />
</p>

---

**SmartLamp (MagicBox)** — це апаратно-програмна екосистема розумної настільної шкатулки на мікроконтролері **ESP32-S3** (16 MB Flash, 8 MB OPI PSRAM) із круглим IPS-екраном **GC9A01 (240×240)** в естетиці **Hallmark Horology**, студійним тактильним звуком через **MAX98357A I2S**, адресною стрічкою **SK6812 RGBW** (двозонна топологія: дно + задня стінка) та хмарним сервером **Oracle Cloud Gateway** на FastAPI.

---

## ⚡ Архітектура системи

```text
    [ ESP32-S3 Dual-Core ]                [ Oracle Cloud VPS ]               [ Client Layer ]
    • Core 1: Input, Display (80MHz SPI), • FastAPI + uvloop + SQLite WAL    • Web Studio (SPA / PWA)
      LED RMT (60 FPS), NVS Storage       • MemoryGuard + /dev/shm RAM Cache • Telegram Mini App
    • Core 0: Wi-Fi, WebSocket, HTTP/OTA, • Spotify OAuth2 + Vinyl Dithering • Telegram Bot
      Sensors (SHT31+BMP280), I2S Audio   • Google Calendar + Edge-TTS + AI
            │                                     │                                   │
            │◄────── ws://.../ws/box ────────────►│◄──── wss://.../ws/dashboard ─────►│
            │   (Telemetry, Commands, Vinyl)      │   (RealtimeHub, Event-Driven)     │
```

### Ключові інженерні рішення
1. **Двоядерна ізоляція FreeRTOS (`src/main.cpp`):**
   - **Core 1:** опитування сенсорів дотику (`InputService`), 60 FPS LED RMT-рушій (`ledCore1Task`), рендеринг дисплея GC9A01 (`DisplayService`), відкладений запис NVS (`StorageService::checkDelayedSave`) і динамічний регулятор частоти CPU (`80 / 160 / 240 MHz` із термозахистом кремнію).
   - **Core 0:** кліматичні датчики (`SensorService`), Wi-Fi / Captive Portal (`NetworkService`), двосторонній WebSocket / HTTP Long-Poll + OTA (`OracleCloudService`) та аудіоконвеєр I2S (`AudioService`).
2. **Pre-Armed Zero-Latency I2S Audio Engine (`AudioService.h`):**
   - Три вбудовані студійні PCM-профілі кліків (`Apple Tock`, `Wood Tap`, `Soft Pop` у `ClickAudioSamples.h`) з **168 Hz High-Pass фільтром**, перцептивною таблицею гучності (`CLICK_VOL_LUT[22]`) та soft-knee лімітером.
   - Фонова ресинхронізація черги DMA на Core 0 після завершення звуку забезпечує миттєвий старт кліка (`< 1 мс` акустичної затримки) без блокування `loop()`.
   - Програмна синхронізація струму: 30-мс плавний вхід (`attack`) світлової хвилі `Touch Ripple` розносить піки струму I2S та LED на шині `5V`.
3. **Автономність, OTA та «Чорна скринька» блекаутів:**
   - `BootSafetyGuard`: захист від crash-loop (`streak >= 3`), 75-секундне випробувальне вікно OTA з авто-відкатом (`esp_ota_mark_app_invalid_rollback_and_reboot`) та апаратний вхід у Emergency AP (утримання лівої кнопки `1.5 с` при ввімкненні).
   - `offline_buf[144]`: запис кліматичних зрізів при відсутності хмари з пакетним вивантаженням через WebSocket `/ws/box` після відновлення зв'язку.
   - `Command TTL`: сповіщення та звуки мають TTL `180 с`, щоб застарілі команди не спрацьовували після довгого блекауту.

---

## 🔢 Єдине джерело версії (Single Source of Truth)

Версії прошивки та сервера не дублюються по файлах вручну:

| Компонент | Де задається версія | Куди підтягується автоматично |
|---|---|---|
| **Прошивка ESP32-S3** | `SMARTLAMP_FW_VERSION` у [`include/Config.h`](include/Config.h) (або build-прапор `-DSMARTLAMP_FW_VERSION=\"...\"` у [`platformio.ini`](platformio.ini)) | JSON-телеметрія `"fw"` (`CommandRouter.h`) $\rightarrow$ екран запуску `drawBootSplash` $\rightarrow$ екран діагностики `drawNetworkDiag` $\rightarrow$ сервер, Telegram-бот і бейдж `fwBadge` у Web Studio |
| **Сервер + Web Studio** | `Settings.VERSION` у [`oracle_server/app/core/config.py`](oracle_server/app/core/config.py) | `portal.html` (`{{APP_VERSION}}`), `/api/v5/state` (`server_version`), `/api/v5/metrics`, `/api/v5/admin/health-report` та бейдж `verBadge` у Web Studio |

---

## 🖥️ Екрани дисплея GC9A01 (240×240 IPS)

* **Системні стани (`SystemUiState`):**
  * `SYS_ONBOARDING_WIZARD` — 6-кроковий майстер першого налаштування (вибір звуку, QR Telegram-бота з `claim_secret`, Wi-Fi AP `192.168.4.1`, статус підключення).
  * `SYS_NETWORK_DIAG` — сервісне вікно (QR вебінтерфейсу, SSID, IP, RSSI, PSRAM, версія прошивки `FW_VERSION`).
  * `SYS_BIRTHDAY_CELEBRATE` — святковий екран привітання.
* **Основні екрани (`ScreenMode`):**
  * `0 · SCREEN_CLIMATE` — хорологічний циферблат, температура/вологість/тиск, погода за вікном, статус сну/DND.
  * `1 · SCREEN_LIVING_GIF` — жива `.mbx` анімація з PSRAM/LittleFS (до 18/10 кадрів з CRC32) або годинник поверх заставки.
  * `2 · SCREEN_SMART_HUB` — вхідні картки Telegram (`INBOX_CAPACITY = 5`), вінілова вертушка Spotify (136×136 RGB565) та найближча подія Google Календаря.
  * `3 · SCREEN_POMODORO` — фокус-таймер Pomodoro (налаштування `5..90 хв`, робочий/перервний цикл).

---

## 🔘 Сенсорне керування (TTP223)

| Сенсор | Короткий клік (`>= 40 мс`) | Довге утримання (`>= 850 мс`) |
|---|---|---|
| **Лівий (`GPIO 4`)** | Наступний основний екран (`0 → 1 → 2 → 0`); у Pomodoro під час бігу: `-1 хв` | На `Climate`: Нічний режим (`Auto 23-07`); на `Smart Hub`: `Warm Cozy ⇄ Spotify Ambi`; на `Pomodoro`: вихід у `Climate`; на інших: перехід у `Pomodoro` |
| **Кришка (`GPIO 6`)** | Наступний режим світла (`0..11`); на `Smart Hub`: прочитати Telegram або `Play/Pause` Spotify; у `Pomodoro`: `Старт/Пауза` | На `Pomodoro`: Стоп/Скидання; при увімкненому світлі: Вимкнути/Увімкнути світло; при вимкненому: `DND` («Не турбувати») |
| **Правий (`GPIO 7`)** | Наступна яскравість (`15 / 35 / 65 / 100%`); на `Living GIF`: годинник поверх заставки; на `Smart Hub`: наступний трек Spotify або лист; у `Pomodoro`: `+5/+1 хв` | На `Climate`: перемикання профілю звуку кліка (**`Apple Tock` → `Wood Tap` → `Soft Pop`**); на `Smart Hub`: попередній трек Spotify; у `Pomodoro`: Стоп |
| **Лівий + Правий (`10 с`)** | — | Аварійний запуск Wi-Fi AP порталу (`SYS_NETWORK_DIAG` + `MagicBox-AP`) |

---

## 💡 Світловий рушій (12 режимів SK6812 RGBW)

| ID | Константа (`Types.h`) | Назва |
|---|---|---|
| `0` | `MODE_WARM_COZY` | Теплий затишок (нативний White канал) |
| `1` | `MODE_CANDLE` | Жива свічка |
| `2` | `MODE_AMBER_EMBER` | Бурштинова свічка |
| `3` | `MODE_FIREPLACE` | Камін з іскрами |
| `4` | `MODE_SUNSET_VIBE` | Захід сонця |
| `5` | `MODE_SAKURA_NEON` | Сакура Неон |
| `6` | `MODE_AURORA_WAVE` | Північне сяйво |
| `7` | `MODE_OCEAN_BREEZE` | Океанський бриз |
| `8` | `MODE_CYBER_RUNNER` | Кібер-комета |
| `9` | `MODE_RAINBOW_FLOW` | Веселка |
| `10` | `MODE_LIGHT_OFF` | Вимкнено |
| `11` | `MODE_SPOTIFY_AMBI` | Spotify AmbiLight |

---

## 🛠️ Апаратне підключення (Pinout ESP32-S3)

```text
  ┌──────────────────────────────────────────────────────────────┐
  │                 ESP32-S3-DevKitC-1 Pin Mapping               │
  ├──────────────────────┬─────────────┬─────────────────────────┤
  │ Модуль периферії     │ GPIO        │ Призначення             │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ GC9A01 IPS Дисплей   │ GPIO 10     │ SPI CS                  │
  │ (80 MHz 4-wire SPI)  │ GPIO 11     │ SPI MOSI                │
  │                      │ GPIO 12     │ SPI SCLK                │
  │                      │ GPIO 13     │ DC                      │
  │                      │ GPIO 14     │ RST                     │
  │                      │ GPIO 2      │ Backlight PWM           │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ MAX98357A I2S ЦАП    │ GPIO 15     │ I2S BCLK                │
  │ (Живлення 5V)        │ GPIO 16     │ I2S LRC                 │
  │                      │ GPIO 17     │ I2S DOUT                │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ SK6812 RGBW (60 LED) │ GPIO 5      │ RMT0 Data (36 дно + 24) │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ Сенсорні кнопки      │ GPIO 4      │ Лівий сенсор            │
  │ (TTP223)             │ GPIO 6      │ Сенсор кришки           │
  │                      │ GPIO 7      │ Правий сенсор           │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ I2C Шина (400 kHz)   │ GPIO 8      │ SDA (SHT31 + BMP280)    │
  │                      │ GPIO 9      │ SCL (SHT31 + BMP280)    │
  └──────────────────────┴─────────────┴─────────────────────────┘
```

---

## 📂 Структура репозиторію

```text
SmartLamp/
├── include/
│   ├── Config.h                    # Глобальні константи, SMARTLAMP_FW_VERSION, піни
│   ├── Secrets.example.h           # Шаблон локальних секретів (скопіювати у Secrets.h)
│   ├── cloud/
│   │   ├── OracleCloudService.h    # WebSocket /ws/box, HTTP fallback, OTA, MBX/Vinyl
│   │   ├── CloudOtaService.h       # OTA з перевіркою SHA-256
│   │   ├── StandaloneOta.h         # Низькорівневий HTTP/Rollback OTA-флешер
│   │   └── TgJsonUtils.h           # UTF-8 декодер та URL-енкодер без heap-фрагментації
│   ├── core/
│   │   ├── AppState.h              # Потокобезпечний стан системи (portMUX_TYPE)
│   │   ├── BootSafetyGuard.h       # Anti-brick захист і авто-відкат прошивки
│   │   ├── CommandRouter.h         # Маршрутизатор команд і JSON-телеметрія
│   │   ├── StorageService.h        # NVS Preferences + LittleFS (/default_saver.mbx)
│   │   └── Types.h                 # Екрани, режими світла, події, MbxHeader
│   ├── hardware/
│   │   ├── AudioService.h          # I2S плеєр + Pre-Armed Zero-Latency DMA кліки
│   │   ├── ClickAudioSamples.h     # PCM-семпли 44.1 kHz (Apple Tock, Wood Tap, Soft Pop)
│   │   ├── I2sDiagnostics.h        # Автономний інструмент профілювання I2S DMA (0 B у prod)
│   │   ├── InputService.h          # Антибрязкіт TTP223 + Wi-Fi Reset combo
│   │   └── SensorService.h         # Драйвери SHT31 + BMP280 + відновлення шини I2C
│   ├── led/
│   │   ├── LedEffects.h            # 12 процедурних RGBW-шейдерів + Touch Ripple
│   │   └── LedService.h            # RMT контролер із slew-rate захистом і лімітом струму
│   ├── network/
│   │   ├── NetworkService.h        # Wi-Fi менеджер, NTP, Blackout Eco-режим
│   │   ├── PortalServer.h          # Локальний Captive Portal на 192.168.4.1
│   │   ├── WebPortalHtml.h         # Вбудована вебсторінка налаштування Wi-Fi
│   │   └── WifiScanner.h           # Асинхронне сканування Wi-Fi мереж
│   └── ui/
│       ├── DisplayDriver.h         # LovyanGFX SPI 80 MHz DMA для GC9A01
│       ├── DisplayService.h        # Стейт-машина дисплея та Slide-анімації
│       ├── HallmarkTheme.h         # RGB565 дизайн-токени палітри
│       ├── ReactionRenderer.h      # Рендеринг анімованих реакцій
│       ├── ScreenRenderer.h        # Усі циферблати, годинник, Smart Hub, Pomodoro
│       └── UkrainianFont*.h        # Вбудовані українські растрові шрифти
├── oracle_server/
│   ├── main.py                     # FastAPI + uvloop + супервізор бота
│   ├── deploy_native.sh            # Скрипт налаштування systemd сервісу
│   └── app/
│       ├── api/                    # endpoints_box.py (/ws/box), endpoints_control.py
│       ├── core/                   # config.py, events.py, memory_guard.py, security.py
│       ├── domain/                 # schemas.py (Pydantic-контракти)
│       ├── services/               # bot, spotify, media, ota, routine, calendar, db, ai
│       └── web/                    # portal_html.py + static/ (portal.html, portal.js, css)
├── src/
│   └── main.cpp                    # Точка входу, FreeRTOS таски, CPU Governor
└── platformio.ini                  # Конфігурація PlatformIO ESP32-S3
```

---

## 🚀 Швидкий старт

### 1. Збірка та прошивка ESP32-S3
```bash
git clone https://github.com/EdlerAG/SmartLamp.git
cd SmartLamp

cp include/Secrets.example.h include/Secrets.h
# Заповніть ORACLE_SERVER_URL та ORACLE_API_KEY у include/Secrets.h

pio run
pio run -t upload
```

> Щоб змінити версію прошивки перед релізом, достатньо оновити `SMARTLAMP_FW_VERSION` у [`include/Config.h`](include/Config.h) (або передати `-DSMARTLAMP_FW_VERSION=\"vX.Y\"` у [`platformio.ini`](platformio.ini)).

### 2. Запуск бекенда (Linux VPS)
```bash
cd oracle_server
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

cp .env.example .env
uvicorn main:app --host 0.0.0.0 --port 8000 --loop uvloop --http httptools
```

Для системного автозапуску через `systemd` використовуйте [`oracle_server/deploy_native.sh`](oracle_server/deploy_native.sh).
