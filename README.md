# 🪄 SmartLamp — MagicBox v3.2 (Cloud Edition)

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/Framework-Arduino%20%2F%20FreeRTOS-blue.svg?style=flat-square&logo=arduino" alt="Framework" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/LEDs-SK6812%20RGBW-green.svg?style=flat-square" alt="LEDs" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20Oracle%20Cloud-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/Version-3.2%20Cloud-brightgreen.svg?style=flat-square" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
</p>

> **MagicBox** — розумна атмосферна лампа-шкатулка на базі **ESP32-S3**, оснащена круглим IPS-дисплеєм GC9A01, адресною RGBW-стрічкою SK6812, кліматичними датчиками високої точності, інтерактивними векторними мем-реакціями, двостороннім Telegram-ботом, власним хмарним бекендом **FastAPI на Oracle Cloud** та адаптивною стійкістю до блекаутів.

---

## ✨ Основні можливості

| Категорія | Опис |
|---|---|
| 🖥️ **Круглий IPS-екран** | 1.28" 240×240 GC9A01 через [LovyanGFX](https://github.com/lovyan03/LovyanGFX), подвійна буферизація, плавний рендеринг 60 FPS та адаптивне зниження до 4 FPS у режимі спокою для енергозбереження. |
| 💡 **11 режимів світла** | Жива свічка, Бурштинова свічка, Затишний камін, Захід сонця, Тепле світло, Аврора, Кіберпанк, Сакура Неон, Океанський бриз, Веселка, Вимкнено (чисті математичні шейдери). |
| 🌙 **Таймер сну «Захід сонця»** | Розумний таймер (15 / 30 / 60 хв) з плавним згасанням яскравості пропорційно залишку часу та м'яким вимкненням світла. Зворотний відлік на екрані, у Telegram та Web. |
| 🎭 **5 векторних мем-реакцій** | Повноекранні динамічні анімації на круглому дисплеї з синхронним підсвічуванням стрічки: 🔥 Вогник, ⚡ Блискавка, 🐱 Котик (підморгує оком!), 👽 Пороблено (UFO Beam), 👑 Краш-режим (Корона). |
| 📬 **Inbox Hub (Історія)** | Кільцевий буфер на 5 останніх повідомлень із мітками часу. Перемикання правою кнопкою по колу (1/5, 2/5...) або перегляд через API та Web-панель. |
| 🌡️ **Метеостанція & Графік** | SHT31 (температура та вологість із калібруванням `TEMP_OFFSET_C`) + BMP280 (тиск). **Живий Sparkline-графік тиску на 24 точки** та розрахунок барометричного тренду ($\Delta P$: STEADY / RISING / FALLING). |
| ☁️ **Oracle Cloud Hub [NEW]** | Власний легкий мікросервіс зв'язку з Oracle Cloud Server: періодична передача телеметрії, черга віддалених команд (зміна режимів, повідомлення, реакції), трекінг живлення. |
| 📲 **Telegram Бот** | Прямий клієнт Telegram Bot API (TLS Keep-Alive): статус, зміна режимів, інлайн-клавіатури, авто-реакції на повідомлення, команди авторизації `/iam_admin` та `/iam_nastia`. |
| 🛡️ **Стійкість до блекаутів** | Blackout Eco режим, захист від крашу ядра через атомарний прапорець `tls_in_flight` при відключенні Wi-Fi, Cold-Boot синхронізація (`offset=-1`) та виявлення відключень сервером. |
| 🌐 **Web Portal & Captive AP** | Сучасний адаптивний веб-інтерфейс у темній темі, авто-портал налаштування Wi-Fi, підтримка CORS Preflight (`HTTP_OPTIONS`) для мобільних застосунків. |
| ♻️ **Потрійне OTA-оновлення** | Локальний ArduinoOTA + Web OTA (`/update`) + віддалений хмарний Pull-OTA через буфер PSRAM з перевіркою магічного байта ESP32 (`0xE9`) та захистом від перетворення на «цеглину» (Rollback). |
| 🔒 **Захист приватних даних** | Архітектура нульового витоку: токени TG, паролі та ключі Oracle Server зберігаються в ігнорованому Git файлі `include/Secrets.h`. |

---

## 📦 Модульна структура проєкту

Архітектура коду розбита на слабопов'язані мікросервіси та математичні рушії:

```text
📦 SmartLamp
 ┣ 📂 include
 ┃ ┣ 📜 Config.h                     // Апаратні константи, таймінги та налаштування
 ┃ ┣ 📜 Secrets.h                    // (У .gitignore) Токени TG та ключі Oracle Server
 ┃ ┣ 📜 Secrets.example.h            // Шаблон для конфігурації секретів
 ┃ ┃
 ┃ ┣ 📂 core                         // Ядро системи та диспетчер мікросервісів
 ┃ ┃ ┣ 📜 Types.h                    // Усі Enum-и та структури даних (винесені з AppState)
 ┃ ┃ ┣ 📜 IService.h                 // Універсальний контракт мікросервісу
 ┃ ┃ ┣ 📜 ServiceRegistry.h          // Контейнер-оркестратор сервісів для ядер FreeRTOS
 ┃ ┃ ┣ 📜 AppState.h                 // Потокобезпечна шина стану (Spinlock Mux)
 ┃ ┃ ┣ 📜 CommandRouter.h            // Єдиний фасад команд для кнопок, Web, TG та Oracle
 ┃ ┃ ┗ 📜 StorageService.h           // Менеджер енергонезалежної пам'яті NVS (Preferences)
 ┃ ┃
 ┃ ┣ 📂 hardware                     // Фізичний рівень (Сенсори та Ввід)
 ┃ ┃ ┣ 📜 InputService.h             // Драйвер ємнісних сенсорів (Short / Long press)
 ┃ ┃ ┗ 📜 SensorService.h            // Неблокуючий SHT31 (CRC-8) + BMP280 + АЦП LDR
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
 ┃ ┃ ┗ 📜 DisplayService.h           // Керування Adaptive FPS (60/4 Гц) та кешем QR-коду
 ┃ ┃
 ┃ ┣ 📂 network                      // Локальна мережа та Веб-портал
 ┃ ┃ ┣ 📜 WifiScanner.h              // Pre-Scan ефіру та генерація JSON без алокацій
 ┃ ┃ ┣ 📜 WebPortalHtml.h            // PROGMEM верстка панелі керування
 ┃ ┃ ┣ 📜 PortalServer.h             // Уніфікований REST API + Captive Portal + CORS
 ┃ ┃ ┣ 📜 OtaManager.h               // Локальний ArduinoOTA
 ┃ ┃ ┗ 📜 NetworkService.h           // Стейт-машина Wi-Fi та режим Blackout Eco
 ┃ ┃
 ┃ ┗ 📂 cloud                        // Хмарні інтеграції (Telegram + Oracle Server + OTA)
 ┃   ┣ 📜 CloudOtaService.h          // Pull-OTA через PSRAM буфер та Anti-Brick Rollback
 ┃   ┣ 📜 TgJsonUtils.h              // Надшвидкий парсер JSON, URL-кодер та Unicode-декодер
 ┃   ┣ 📜 TgKeyboards.h              // Генератор Reply та Inline меню
 ┃   ┣ 📜 TelegramService.h          // Двосторонній клієнт Telegram API (TLS Keep-Alive)
 ┃   ┗ 📜 OracleCloudService.h       // [НОВЕ] Мікросервіс зв'язку з сервером Oracle Cloud
 ┃
 ┣ 📂 src
 ┃ ┗ 📜 main.cpp                     // Чистий оркестратор ядер FreeRTOS
 ┃
 ┗ 📂 oracle_server                  // [НОВЕ] Бекенд для твого сервера Oracle Cloud
   ┣ 📜 main.py                      // FastAPI сервер: телеметрія, блекаут-детектор, черга команд, AI/OTA
   ┗ 📜 requirements.txt             // Python-залежності (fastapi, uvicorn, pydantic)
```

---

## 🗂️ Двоядерна архітектура FreeRTOS

```text
┌────────────────────────────────────────────────────────────────────────┐
│                              ESP32-S3                                  │
├───────────────────────────────────┬────────────────────────────────────┤
│              CORE 0               │               CORE 1               │
├───────────────────────────────────┼────────────────────────────────────┤
│  Core0Net (Пріоритет 2, 8 KB)     │  loop()                            │
│  ├── NetworkService (Wi-Fi, Web)  │  ├── InputService (сенсорні кнопки)│
│  ├── SensorService (SHT31, BMP)   │  ├── processEvents (черга подій)   │
│  ├── OracleCloudService (Sync)    │  ├── LedService (SK6812 60 FPS)    │
│  └── NTP клієнт часу              │  └── DisplayService (GC9A01 UI)    │
│                                   │                                    │
│  Core0Telegram (Пріоритет 1, 10 KB│                                    │
│  └── TelegramService (TLS)        │                                    │
│      ├── Keep-Alive сесія         │                                    │
│      ├── Cold-Boot Offset Sync    │                                    │
│      └── Інтерактивні меню        │                                    │
└───────────────────────────────────┴────────────────────────────────────┘
                  Потокобезпечна шина даних: AppState (FreeRTOS Spinlock)
```

---

## ☁️ Oracle Cloud Backend (`oracle_server`)

У папці [`oracle_server/`](file:///d:/Vlasne/code/SmartLamp/oracle_server) розміщено готовий легкий FastAPI-сервер для розгортання на віртуальній машині (Oracle Cloud Free Tier, VPS або локально):

### Встановлення та запуск
```bash
cd oracle_server
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 8000
```

### Основні ендпоінти
- `POST /api/telemetry` — прийом телеметрії від шкатулки (температура, тиск, яскравість, Wi-Fi, RSSI, пам'ять). У відповідь відразу повертає чергову команду з черги.
- `GET /api/telemetry` — отримання актуального стану лампи для зовнішніх дашбордів.
- `POST /api/commands` — постановка команди в чергу (`set_mode`, `toggle_power`, `set_brightness`, `sleep_timer`, `reaction`, `push_message`).
- `GET /api/blackout` — інтелектуальний блекаут-детектор: відслідковує переривання зв'язку, фіксує точний час зникнення живлення та тривалість відключень.
- `GET /api/ota/latest` — віддача метаданих та URL для завантаження свіжої прошивки лампою.

---

## 🖥️ Екрани дисплея (Screen Modes)

Короткий дотик до **лівої кнопки** перемикає екрани:

1. **CLIMATE (Клімат)** — температура (велика цифра), дуга вологості, тиск, статус Wi-Fi/AP, поточний час, активний світловий режим та статус таймера сну.
2. **BAROMETER (Барометр)** — циферблат атмосферного тиску, барометричний прогноз погоди, стрілка тренду ($\Delta P$) та **живий стовпчиковий графік (Sparkline) на 24 точки**.
3. **INBOX HUB (Повідомлення)** — картка останніх повідомлень із міткою часу. Коротким кліком правої кнопки можна гортати історію 1/5, 2/5...
4. **NETWORK (Мережа)** — динамічний QR-код (для підключення до AP або швидкого переходу на веб-панель), IP-адреса, SSID та рівень сигналу RSSI.

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
> `Secrets.h` автоматично ігнорується Git. Якщо файл не створено, лампа скомпілюється з дефолтними параметрами, а ключі можна ввести через веб-панель.

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
| `/api/status` | `GET` | Повний JSON-статус (клімат, світло, таймер, heap, wifi). |
| `/api/inbox` | `GET` | JSON-масив 5 останніх повідомлень з мітками часу. |
| `/api/cmd?do=mode` | `POST` | Наступний режим підсвічування. |
| `/api/cmd?do=mode&id=3` | `POST` | Увімкнення конкретного режиму (0–10). |
| `/api/cmd?do=brt` | `POST` | Циклічна яскравість або авто-режим. |
| `/api/cmd?do=sleep&min=30` | `POST` | Запуск таймера сну на 15, 30 або 60 хв (`min=0` — скасувати). |
| `/api/cmd?do=reaction&id=1` | `POST` | Запуск мем-реакції (1: 🔥, 2: ⚡, 3: 🐱, 4: 👽, 5: 👑). |
| `/api/msg` | `POST` | Відправка повідомлення на екран (`sender=`, `msg=`). |
| `/api/scan` | `GET` | Сканування Wi-Fi мереж без динамічних алокацій пам'яті. |
| `/update` | `POST` | Web OTA завантаження бінарного файлу прошивки. |

---

## 🤖 Команди Telegram Бота

- `/start` — головне інтерактивне меню з клавіатурами.
- `/status` — актуальний кліматичний звіт, тиск, стан стрічки та час аптайму.
- `/iam_admin` — авторизація адміністратора.
- `/iam_nastia` — авторизація користувача.
- `/sleep 15` / `/sleep 30` / `/sleep 60` — запуск таймера сну.
- Будь-який текст або фото, надіслані боту, миттєво з'являються на круглому дисплеї шкатулки з приємною світловою хвилею!

---

## 📄 Ліцензія

Розповсюджується за ліцензією **MIT**. Повністю відкрито для використання та кастомізації.
