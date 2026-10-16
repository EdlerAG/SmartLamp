# 🪄 SmartLamp — MagicBox v4.5 (Cloud & Audio Edition)

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/Framework-Arduino%20%2F%20FreeRTOS-blue.svg?style=flat-square&logo=arduino" alt="Framework" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/Audio-I2S%20MAX98357A-yellow.svg?style=flat-square" alt="Audio" />
  <img src="https://img.shields.io/badge/LEDs-SK6812%20RGBW-green.svg?style=flat-square" alt="LEDs" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20Oracle%20Cloud-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/AI-Google%20Gemini-4285F4.svg?style=flat-square&logo=google" alt="AI" />
  <img src="https://img.shields.io/badge/Version-4.5%20CloudMedia-brightgreen.svg?style=flat-square" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
</p>

> **MagicBox** — розумна атмосферна лампа-шкатулка на базі **ESP32-S3**, оснащена круглим IPS-дисплеєм GC9A01, цифровим звуковим трактом I2S (MAX98357A), адресною RGBW-стрічкою SK6812, кліматичними датчиками високої точності, динамічним медіа-рушієм RGB565 для Telegram-фото та наліпок, двостороннім ботом Telegram, власним модульним бекендом **FastAPI на Oracle Cloud** з підтримкою **Google Gemini AI**, аналітикою відключень світла та адаптивною стійкістю до блекаутів.

---

## ✨ Основні можливості

| Категорія | Опис |
|---|---|
| 🖥️ **Круглий IPS-екран** | 1.28" 240×240 GC9A01 через [LovyanGFX](https://github.com/lovyan03/LovyanGFX), подвійна буферизація, плавний рендеринг 60 FPS та адаптивне зниження до 4 FPS у режимі спокою для максимального енергозбереження. |
| 🔊 **Цифровий звук I2S [NEW]** | Підсилювач **MAX98357A** (I2S DMA): апаратне відтворення аудіо (MP3 / WAV), звуковий супровід мем-реакцій, сповіщення та озвучення повідомлень українським TTS через хмару. |
| 🖼️ **Медіа-рушій RGB565 [NEW]** | Прямий стрімінг у PSRAM оброблених сервером зображень, фото та прозорих WebP-наліпок Telegram у бінарному форматі **RGB565 (240×240)**. Синхронізація шини з нульовими розривами (Zero-Tearing). |
| 💡 **11 режимів світла** | Жива свічка, Бурштинова свічка, Затишний камін, Захід сонця, Тепле світло, Аврора, Кіберпанк, Сакура Неон, Океанський бриз, Веселка, Вимкнено (чисті математичні шейдери). |
| 🌙 **Таймер сну «Захід сонця»** | Розумний таймер (15 / 30 / 60 хв) з плавним згасанням яскравості пропорційно залишку часу та м'яким вимкненням світла. Зворотний відлік на екрані, у Telegram та Web. |
| 🎭 **5 векторних мем-реакцій** | Повноекранні динамічні векторні анімації на круглому дисплеї з синхронним підсвічуванням стрічки: 🔥 Вогник, ⚡ Блискавка, 🐱 Котик (підморгує оком!), 👽 Пороблено (UFO Beam), 👑 Краш-режим (Корона). |
| 📬 **Inbox Hub (Історія)** | Кільцевий буфер на 5 останніх повідомлень із мітками часу. Перемикання правою кнопкою по колу (1/5, 2/5...) або перегляд через API та Web-панель. |
| 🌡️ **Метеостанція & Графік** | SHT31 (температура та вологість із калібруванням `TEMP_OFFSET_C`) + BMP280 (тиск). **Живий Sparkline-графік тиску на 24 точки** та розрахунок барометричного тренду ($\Delta P$: STEADY / RISING / FALLING). |
| ☁️ **Oracle Cloud Backend [NEW]** | Модульний мікросервісний бекенд FastAPI: конвеєр обробки медіа, черга команд з дренажем (`has_more`), аналітика Open-Meteo, інтеграція з **Google Gemini AI** та LRU-кеш медіафайлів (150 МБ). |
| 📲 **Telegram Бот** | Прямий клієнт Telegram Bot API (TLS Keep-Alive) + розширений бот на сервері: прийом стікерів, фото, голосових команд, інлайн-меню, авторизація `/iam_admin` та `/iam_nastia`. |
| 🛡️ **Стійкість до блекаутів** | Режим Blackout Eco, захист від крашу ядра через атомарний прапорець `tls_in_flight` при відключенні Wi-Fi, Cold-Boot синхронізація (`offset=-1`) та серверний моніторинг тривалості відключень живлення. |
| 🌐 **Web Portal & Captive AP** | Сучасний адаптивний веб-інтерфейс у темній темі, авто-портал налаштування Wi-Fi, динамічне збереження налаштувань Oracle Cloud у пам'ять NVS без перепрошивки та CORS Preflight (`HTTP_OPTIONS`). |
| ♻️ **Потрійне OTA-оновлення** | Локальний ArduinoOTA + Web OTA (`/update`) + віддалений хмарний Pull-OTA через буфер PSRAM з перевіркою магічного байта ESP32 (`0xE9`), SHA256 та захистом від перетворення на «цеглину» (Rollback). |
| 🔒 **Захист приватних даних** | Архітектура нульового витоку: токени TG, паролі та ключі Oracle Server зберігаються в ігнорованому Git файлі `include/Secrets.h` та захищеному сховищі NVS. |

---

## 📦 Модульна структура проєкту

Архітектура коду побудована на принципах Clean Architecture та розбита на ізольовані мікросервіси:

```text
📦 SmartLamp
 ┣ 📂 include
 ┃ ┣ 📜 Config.h                     // Апаратні константи, таймінги, I2S та хмарні налаштування
 ┃ ┣ 📜 Secrets.h                    // (У .gitignore) Токени TG та ключі Oracle Server
 ┃ ┣ 📜 Secrets.example.h            // Шаблон для конфігурації секретів
 ┃ ┃
 ┃ ┣ 📂 core                         // Ядро системи та диспетчер мікросервісів
 ┃ ┃ ┣ 📜 Types.h                    // Усі Enum-и та структури даних
 ┃ ┃ ┣ 📜 IService.h                 // Універсальний контракт мікросервісу з інтервалами
 ┃ ┃ ┣ 📜 ServiceRegistry.h          // Контейнер-оркестратор сервісів для ядер FreeRTOS
 ┃ ┃ ┣ 📜 AppState.h                 // Потокобезпечна шина стану (Spinlock Mux + PSRAM Canvas)
 ┃ ┃ ┣ 📜 CommandRouter.h            // Єдиний фасад команд для кнопок, Web, TG та Oracle
 ┃ ┃ ┗ 📜 StorageService.h           // Менеджер енергонезалежної пам'яті NVS (Preferences)
 ┃ ┃
 ┃ ┣ 📂 hardware                     // Фізичний рівень (Сенсори, Звук та Ввід)
 ┃ ┃ ┣ 📜 InputService.h             // Драйвер ємнісних сенсорів (Short / Long press)
 ┃ ┃ ┣ 📜 SensorService.h            // Неблокуючий SHT31 (CRC-8) + BMP280 + АЦП LDR
 ┃ ┃ ┗ 📜 AudioService.h             // [НОВЕ] Цифровий I2S звук (MAX98357A, MP3/WAV/TTS)
 ┃ ┃
 ┃ ┣ 📂 led                          // Світловий рушій SK6812
 ┃ ┃ ┣ 📜 LedEffects.h               // Чисті математичні шейдери (11 режимів + 5 реакцій)
 ┃ ┃ ┗ 📜 LedService.h               // Контролер яскравості, Таймера сну та кадрів
 ┃ ┃
 ┃ ┣ 📂 ui                           // Графічний рушій GC9A01 (240x240)
 ┃ ┃ ┣ 📜 DisplayDriver.h            // Конфігурація шини SPI 80 МГц + DMA
 ┃ ┃ ┣ 📜 TextUtils.h                // UTF-8 транслітератор та розбиття тексту на рядки
 ┃ ┃ ┣ 📜 ReactionRenderer.h         // Векторна графіка 5 мем-реакцій (Кіт, НЛО, Корона...)
 ┃ ┃ ┣ 📜 ScreenRenderer.h           // Відмальовка 4 робочих вікон та OTA-екрана
 ┃ ┃ ┗ 📜 DisplayService.h           // Керування Adaptive FPS (60/4 Гц) та PSRAM Canvas-кадрами
 ┃ ┃
 ┃ ┣ 📂 network                      // Локальна мережа та Веб-портал
 ┃ ┃ ┣ 📜 WifiScanner.h              // Pre-Scan ефіру та генерація JSON без алокацій
 ┃ ┃ ┣ 📜 WebPortalHtml.h            // PROGMEM верстка панелі керування (налаштування Oracle Cloud)
 ┃ ┃ ┣ 📜 PortalServer.h             // Уніфікований REST API + Captive Portal + CORS
 ┃ ┃ ┣ 📜 OtaManager.h               // Локальний ArduinoOTA
 ┃ ┃ ┗ 📜 NetworkService.h           // Стейт-машина Wi-Fi та режим Blackout Eco
 ┃ ┃
 ┃ ┗ 📂 cloud                        // Хмарні інтеграції (Telegram + Oracle Server + OTA)
 ┃   ┣ 📜 CloudOtaService.h          // Pull-OTA через PSRAM буфер та Anti-Brick Rollback
 ┃   ┣ 📜 TgJsonUtils.h              // Надшвидкий парсер JSON, URL-кодер та Unicode-декодер
 ┃   ┣ 📜 TgKeyboards.h              // Генератор Reply та Inline меню
 ┃   ┣ 📜 TelegramService.h          // Двосторонній клієнт Telegram API (TLS Keep-Alive)
 ┃   ┗ 📜 OracleCloudService.h       // [ОНОВЛЕНО] Синхронізація, RGB565 Canvas стрімінг, черга команд
 ┃
 ┣ 📂 src
 ┃ ┗ 📜 main.cpp                     // Оркестратор ядер FreeRTOS та пріоритетів задач
 ┃
 ┗ 📂 oracle_server                  // [ОНОВЛЕНО] Хмарний бекенд для Oracle Cloud VM
   ┣ 📜 .env.example                 // Змінні середовища (TG токен, Gemini API, ключі)
   ┣ 📜 deploy_native.sh             // 1-клік інсталятор для Linux (systemd + venv)
   ┣ 📜 Dockerfile                   // Ізольований контейнер Python 3.11-slim
   ┣ 📜 docker-compose.yml           // Готовий продакшен-сервіс (restart: always)
   ┣ 📜 requirements.txt             // Залежності (fastapi, pillow, edge-tts, httpx)
   ┣ 📜 main.py                      // Точка входу FastAPI + Lifespan Watchdog
   ┗ 📂 app
     ┣ 📂 core                       // Конфігурація, security (X-Device-Key) та асинхронний HTTP
     ┣ 📂 models                     // Строгі Pydantic v2 схеми телеметрії, медіа та команд
     ┣ 📂 services                   // Мікросервіси: AI, Audio TTS, Blackout, Media, Storage (LRU), Weather
     ┗ 📂 api                        // Маршрути для лампи (/api/box/*) та клієнтів (/api/control/*)
```

---

## 🗂️ Двоядерна архітектура FreeRTOS

Система використовує повний потенціал двох ядер **ESP32-S3**:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                              ESP32-S3                                  │
├───────────────────────────────────┬────────────────────────────────────┤
│              CORE 0               │               CORE 1               │
├───────────────────────────────────┼────────────────────────────────────┤
│  Core0Net (Пріоритет 2, 8 KB)     │  loop()                            │
│  ├── NetworkService (Wi-Fi, Web)  │  ├── InputService (сенсорні кнопки)│
│  ├── SensorService (SHT31, BMP)   │  ├── LedService (SK6812 60 FPS)    │
│  ├── AudioService (I2S DMA Звук)  │  ├── DisplayService (GC9A01 60 FPS)│
│  └── NTP клієнт синхронізації часу│  └── processEvents (черга подій)   │
│                                   │                                    │
│  Core0Cloud (Пріоритет 1, 10 KB)  │  Адаптивний FPS:                   │
│  ├── TelegramService (TLS Session)│  - 60 FPS під час анімацій та дій  │
│  └── OracleCloudService           │  - 4 FPS у режимі очікування       │
│      ├── Multi-Command Drain      │                                    │
│      └── RGB565 Canvas Streamer   │                                    │
└───────────────────────────────────┴────────────────────────────────────┘
                  Потокобезпечна шина даних: AppState (FreeRTOS Spinlock)
```

> [!TIP]
> Під час відтворення звуку через `AudioService`, активної роздачі Wi-Fi точки доступу або OTA-прошивки фонова задача `Core0Net` автоматично перемикається в режим низької латентності (затримка 1 мс), що гарантує безперервне і кристально чисте звучання.

---

## ☁️ Oracle Cloud Backend (`oracle_server`)

Хмарний сервер створений для розгортання на **Oracle Cloud Free Tier VM** (Ubuntu / Oracle Linux) або будь-якому іншому VPS.

### 🚀 Швидке розгортання

#### Варіант A. 1-клік встановлення через Native Systemd (Рекомендовано):
```bash
cd oracle_server
chmod +x deploy_native.sh
./deploy_native.sh
```
Скрипт автоматично створить віртуальне оточення Python, встановить залежності та зареєструє системну службу `smartlamp.service` з автозапуском.

#### Варіант B. Запуск через Docker Compose:
```bash
cd oracle_server
cp .env.example .env
# Відредагуйте змінні в .env (BOT_TOKEN, GEMINI_API_KEY, DEVICE_KEY тощо)
docker-compose up -d --build
```

### 📡 Архітектура API ендпоінтів

#### 1. Шлюз шкатулки (`/api/box/*`):
- `POST /api/box/sync` — прийом телеметрії від лампи (клімат, стрічка, живлення, RSSI, пам'ять). Повертає пакет команд із черги (підтримується мульти-командний дренаж `has_more: true`).
- `GET /api/box/ota/firmware.bin` — віддача бінарного файлу прошивки з перевіркою заголовка `0xE9` та SHA256 хешу.
- `GET /api/box/media/{filename}` — потокова роздача згенерованих raw RGB565 бінарних кадрів (240×240) або аудіотреків.

#### 2. Шлюз керування та аналітики (`/api/control/*`):
- `GET /api/control/state` — актуальний стан лампи, сенсорів та історія клімату.
- `POST /api/control/commands` — постановка команд у чергу (`set_mode`, `toggle_power`, `set_brightness`, `sleep_timer`, `reaction`, `push_message`, `play_sound`).
- `GET /api/control/blackout` — статистика відключень світла (точний час зникнення мережі, тривалість, журнал).
- `POST /api/control/ai/digest` — генерація зведення про клімат, прогноз погоди та порад від **Google Gemini AI**.
- `POST /api/control/tts` — синтез українського мовлення з тексту через Edge-TTS / gTTS.

---

## 🖥️ Екрани дисплея (Screen Modes)

Короткий дотик до **лівої кнопки** перемикає екрани:

1. **CLIMATE (Клімат)** — велика цифра температури, кольорова дуга вологості, тиск, статус Wi-Fi/AP, поточний годинник, активний світловий режим та статус таймера сну.
2. **BAROMETER (Барометр)** — циферблат атмосферного тиску, барометричний прогноз погоди, стрілка тренду ($\Delta P$) та **живий стовпчиковий графік (Sparkline) на 24 точки**.
3. **INBOX HUB (Повідомлення)** — картка останніх повідомлень із міткою часу. Коротким кліком правої кнопки можна гортати історію 1/5, 2/5...
4. **NETWORK (Мережа)** — динамічний QR-код (для підключення до AP або швидкого переходу на веб-панель), IP-адреса, SSID та рівень сигналу RSSI.
5. **CUSTOM CANVAS (Медіа-кадр)** — повноекранне відображення фотографій або стікерів із Telegram, конвертованих хмарним сервером у формат 240×240 RGB565.

---

## 🔘 Сенсорне керування (Touch Controls)

| Кнопка | Короткий дотик | Утримання (900 мс) |
|---|---|---|
| **Ліва (Touch Left / GPIO 4)** | Наступний екран дисплея | Перезапуск точки доступу (AP Portal) |
| **Кришка (Touch Top / GPIO 6)** | Наступний режим світла / Скинути сповіщення | Увімкнути / Вимкнути світло |
| **Права (Touch Right / GPIO 7)** | **На екрані Inbox:** перегляд наступного повідомлення (1..5)<br>**На інших екранах:** яскравість (Авто ➔ 15% ➔ 35% ➔ 65% ➔ 100%) | Циклічний запуск 5 мем-реакцій (🔥 ➔ ⚡ ➔ 🐱 ➔ 👽 ➔ 👑) |

---

## 🛠️ Апаратне забезпечення (Hardware & Pinout)

```text
┌─────────────┬───────────┬──────────────────────────────────────────┐
│ GPIO        │ Призначення│ Опис                                     │
├─────────────┼───────────┼──────────────────────────────────────────┤
│ GPIO 1      │ LDR       │ Фоторезистор (ADC1_CH0, автояскравість)  │
│ GPIO 4      │ TOUCH_L   │ Ліва кнопка (зміна екранів / скидання AP)│
│ GPIO 5      │ LED_DATA  │ DIN стрічки SK6812 RGBW (60 LED)         │
│ GPIO 6      │ TOUCH_TOP │ Кришка (керування світлом / прочитано)   │
│ GPIO 7      │ TOUCH_R   │ Права кнопка (яскравість / Inbox / меми) │
│ GPIO 8      │ I2C_SDA   │ SHT31 & BMP280 SDA                       │
│ GPIO 9      │ I2C_SCL   │ SHT31 & BMP280 SCL                       │
│ GPIO 10     │ TFT_CS    │ GC9A01 Chip Select                       │
│ GPIO 11     │ TFT_MOSI  │ GC9A01 SPI MOSI                          │
│ GPIO 12     │ TFT_SCLK  │ GC9A01 SPI Clock                         │
│ GPIO 13     │ TFT_DC    │ GC9A01 Data / Command                    │
│ GPIO 14     │ TFT_RST   │ GC9A01 Reset                             │
│ GPIO 15     │ I2S_BCLK  │ MAX98357A I2S Bit Clock                  │
│ GPIO 16     │ I2S_LRC   │ MAX98357A I2S Left/Right Clock (WS)      │
│ GPIO 17     │ I2S_DOUT  │ MAX98357A I2S Data Out                   │
└─────────────┴───────────┴──────────────────────────────────────────┘
```

---

## 🚀 Швидкий старт

### 1. Клонування репозиторію
```bash
git clone https://github.com/EdlerAG/SmartLamp.git
cd SmartLamp
```

### 2. Налаштування ключів доступу (Secrets)
Створіть файл `include/Secrets.h` на основі `include/Secrets.example.h`:

```cpp
#pragma once

namespace Secrets {
    constexpr const char* TG_TOKEN          = "1234567890:AAH_YOUR_TELEGRAM_BOT_TOKEN";
    constexpr const char* ADMIN_ID          = "1047202957";
    constexpr const char* GIRL_ID           = "1334785533";
    constexpr const char* ORACLE_SERVER_URL = "http://your-oracle-ip:8000";
    constexpr const char* ORACLE_API_KEY    = "smartlamp-secret-key";
}
```

> [!NOTE]
> `Secrets.h` автоматично ігнорується Git. Якщо файл не створено або адресу сервера змінено в процесі роботи, параметри Oracle Server URL та Device Key можна налаштувати або оновити безпосередньо через вбудований Web Portal без повторної перепрошивки.

### 3. Збірка та прошивка через PlatformIO
```bash
# Збірка прошивки
pio run

# Прошивка через USB
pio run --target upload

# Монітор серійного порту
pio device monitor
```

---

## 🌐 REST API шкатулки

| Endpoint | Метод | Опис |
|---|---|---|
| `/api/status` | `GET` | Повний JSON-статус (клімат, світло, таймер, heap, wifi, oracle). |
| `/api/inbox` | `GET` | JSON-масив 5 останніх повідомлень з мітками часу. |
| `/api/cmd?do=mode` | `POST` | Наступний режим підсвічування. |
| `/api/cmd?do=mode&id=3` | `POST` | Увімкнення конкретного режиму (0–10). |
| `/api/cmd?do=brt` | `POST` | Циклічна яскравість або авто-режим. |
| `/api/cmd?do=sleep&min=30` | `POST` | Запуск таймера сну на 15, 30 або 60 хв (`min=0` — скасувати). |
| `/api/cmd?do=reaction&id=1` | `POST` | Запуск мем-реакції (1: 🔥, 2: ⚡, 3: 🐱, 4: 👽, 5: 👑). |
| `/api/msg` | `POST` | Відправка повідомлення на екран (`sender=`, `msg=`). |
| `/api/scan` | `GET` | Сканування Wi-Fi мереж без динамічних алокацій пам'яті. |
| `/api/config/oracle` | `GET` | Отримання налаштованої адреси Oracle Cloud сервера. |
| `/api/config/oracle` | `POST` | Збереження адреси сервера та Device Key в NVS пам'ять. |
| `/update` | `POST` | Web OTA завантаження бінарного файлу прошивки. |

---

## 🤖 Можливості Telegram Бота

- `/start` — головне інтерактивне меню з клавіатурами та швидким вибором режимів.
- `/status` — актуальний кліматичний звіт, графік тиску, стан стрічки та час безперервної роботи.
- `/iam_admin` та `/iam_nastia` — авторизація адміністратора та користувачів.
- `/sleep 15` / `/sleep 30` / `/sleep 60` — запуск таймера сну.
- **Фото та наліпки:** Будь-яке зображення або стікер автоматично конвертуються сервером і відображаються на круглому дисплеї з підсвічуванням!
- **Голосові повідомлення та звук:** Підтримка озвучення вхідних повідомлень та реакцій через цифровий динамік I2S.

---

## 📄 Ліцензія

Розповсюджується за ліцензією **MIT**. Повністю відкрито для некомерційного використання, форків та власних модифікацій.
