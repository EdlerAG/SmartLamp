# 🪄 SmartLamp — MagicBox v5.6 (Sovereign Real-Time Edition)
### *Персоналізований медіахаб, розумне освітлення та турбота на базі ESP32-S3*

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/RTOS-FreeRTOS%20Dual--Core-blue.svg?style=flat-square" alt="FreeRTOS" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/Realtime-WebSocket%2015ms-00C7B7.svg?style=flat-square" alt="WebSocket" />
  <img src="https://img.shields.io/badge/Design-Hallmark%20Horology%20Deck-E07A5F.svg?style=flat-square" alt="Design" />
  <img src="https://img.shields.io/badge/Care-Nastya%20Routines%20%26%20Pomodoro-e63946.svg?style=flat-square" alt="Care" />
  <img src="https://img.shields.io/badge/Privacy-Google%20Calendar%20Zero--Knowledge-4EA8DE.svg?style=flat-square" alt="Calendar" />
  <img src="https://img.shields.io/badge/Spotify-Bayer%20Vinyl%20%26%20AmbiLight-1DB954.svg?style=flat-square&logo=spotify" alt="Spotify" />
  <img src="https://img.shields.io/badge/Audio-I2S%20MAX98357A%20%2B%20Edge--TTS-yellow.svg?style=flat-square" alt="Audio" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20uvloop%20%2F%20Oracle%20VPS-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/Version-5.6%20Sovereign-brightgreen.svg?style=flat-square" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
</p>

---

> **MagicBox v5.6** — автономна апаратно-програмна екосистема розумного затишку на мікроконтролері **ESP32-S3**, круглому IPS-дисплеї GC9A01 у хорологічному стилі **Hallmark Horology** та хмарному ядрі на **Oracle Cloud VPS**. Створена як душевний, високотехнологічний подарунок для Насті.
> 
> Поєднує блискавичну **двосторонню WebSocket-синхронізацію (15 мс)**, апаратний аудіо-відеоплеєр (вінілові платівки з dithering-обкладинками Spotify, AmbiLight-підсвічування), трекінг мікроклімату з енергонезалежною «чорною скринькою» блекаутів, персонального ШІ-асистента, навчальний Pomodoro-таймер та Telegram Mini App без навантаження на акумулятор смартфона й сервер.

---

## ⚡ Архітектурні стовпи MagicBox v5.6

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            MAGICBOX v5.6 TOPOLOGY                           │
└─────────────────────────────────────────────────────────────────────────────┘
      
    [ 🖥️ ESP32-S3 Lamp ]               [ ☁️ Oracle VPS Backend ]             [ 📱 Client Layer ]
    • Dual-Core Xtensa LX7             • FastAPI + uvloop (PID 2422056)       • Telegram WebApp v5.6
    • 8 KB WebSocket Buffer            • MemoryGuard (< 65 MB RAM)            • Web Studio Atelier
    • FreeRTOS Core 0/1 Split          • SQLite WAL + VACUUM INTO Backups     • Zero-Click Auth
            │                                     │                                    │
            │◄────── ws://.../ws/box ────────────►│◄───── wss://.../ws/dashboard ─────►│
            │   (15ms latency, 0% CPU idle)       │   (RealtimeHub, Event-Driven)      │
            │                                     │                                    │
            │◄────── Binary Vinyl (37 KB) ────────┤◄───── Spotify OAuth2 & LRCLIB ─────┘
            │   (Pull-demand, 5 TCP chunks)       │
            ▼                                     ▼
    [ Hardware Drivers ]                  [ Third-Party APIs ]
    • GC9A01 (SPI 80 MHz DMA)             • Spotify Web API (AmbiLight + Lyrics)
    • MAX98357A (I2S DMA Audio)           • Google Calendar (Zero-Knowledge)
    • SK6812 (60 RGBW LEDs)               • Open-Meteo & LRCLIB
    • SHT31 + BMP280 + LDR                • Microsoft Edge-TTS / Voice Clone
```

### 1. ⚡ Повнодуплексний WebSocket Core (0.00% CPU у простої)
- Замість важкого постійного HTTP-опитування лампа та веб-студія підключені через постійні бінарні й текстові **WebSocket-тунелі** (`/ws/box` та `/ws/dashboard`).
- **ESP32-S3 Buffer 8192 B**: Приймальний буфер 8 КБ усуває фрагментацію пакетів і прискорює завантаження вінілових обкладинок (37 КБ) до **5 швидких чанків** без навантаження на шину I2S.
- **Серіалізація через `asyncio.Lock`**: Виключає гонки корутин при одночасній відправці JSON-телеметрії та бінарних кадрів.

### 2. 🎵 Spotify Vinyl Player & Живий Текст (LRCLIB)
- **Bayer 4×4 Ordered Dithering**: Обкладинка альбому на сервері перетворюється на кругле вінілове яблуко 136×136 px у форматі RGB565 Big-Endian з промальованими доріжками й центральним отвором.
- **AmbiLight Sync**: Сервер аналізує домінантні кольори обкладинки та синхронно транслює RGB-тон у 12-й режим підсвічування лампи (`MODE_SPOTIFY_AMBI`).
- **Синхронізовані рядки пісні**: Автоматичне підтягування тексту пісні через LRCLIB з субсекундною інтерполяцією рядка за поточним прогресом треку.
- **Demand-Driven цикл**: Опитування Spotify активне **лише тоді, коли відкритий інтерфейс** або лампа перебуває на екрані Smart Hub. При закритті вкладок фоновий воркер автоматично зупиняється (0% CPU).

### 3. 🌸 Турбота про Настю (Routines & Deep Work)
- **Тихі години (23:00–08:00)**: Автоматичне плавне зниження яскравості до 15% та переведення аудіосистеми в режим Mute.
- **Розумний світанок (Sunrise Alarm)**: За 20 хвилин до будильника лампа плавно розпалює бурштиновий схід сонця (1% → 100%), після чого звучить гармонійний процедурний акорд або голосове вітання.
- **Навчальний фокус Pomodoro**: 25 хвилин глибокої концентрації під тепле бурштинове світло без зайвих сповіщень + 5 хвилин відпочинку під смарагдовий бриз. Стан миттєво синхронізується між лампою, веб-студією та Telegram.
- **Zero-Knowledge Google Calendar**: Сервер запитує лише найближчу подію (`maxResults=1`), коли до неї залишається $\le 15$ хвилин, і транслює коротке нагадування на Smart Hub. Події не логуються і не зберігаються в системних таблицях.

### 4. 📊 Метеостанція та «Чорна скринька» блекаутів
- **Датчики лабораторного класу**: Sensirion **SHT31** (температура й вологість, похибка $\pm 0.2^\circ\text{C}$) + Bosch **BMP280** (атмосферний тиск, точність $\pm 0.12\text{ гПа}$) + фоторезистор **LDR** (освітленість кімнати).
- **Барометричний вердикт Dieter Rams**: Розрахунок тренду тиску за 24 години, векторна стрілка зміни та надійний прогноз погоди без інтернету.
- **Офлайн-буфер блекаутів**: При зникненні Wi-Fi лампа кожні 10 хвилин записує кліматичні зрізи в циклічний RAM-буфер. Щойно зв'язок відновлюється — весь масив передається пакетом у SQLite WAL без втрати історії графіків.

### 5. 📱 Telegram Mini App & Web Studio v5.6
- **Atelier Glassmorphism**: Преміальний темний інтерфейс на чистих CSS-змінних без сторонніх важких фреймворків, розроблений за стандартами анти-слопу (Zero generic gradients, 1px crisp borders, JetBrains Mono tabular numerals).
- **0.00s Instant Paint**: Миттєвий рендер останнього стану з `localStorage` до завершення першого мережевого запиту.
- **Zero-Click Telegram Auth**: Безшовна криптографічна валідація через `initData` Telegram WebApp.
- **Cache-Busting & no-store**: Гарантоване оновлення коду на iOS/Android без залипання застарілого кешу WebView.

---

## 🖥️ Екранні інтерфейси GC9A01 (240×240 IPS)

```
       [ 0: CLIMATE ]              [ 1: LIVING GIF ]             [ 2: SMART HUB ]             [ 3: BAROMETER ]
    ┌──────────────────┐         ┌──────────────────┐         ┌──────────────────┐         ┌──────────────────┐
    │     14:32:08     │         │   (·) SWISS      │         │  (◎)  VINYL SPIN │         │    752.4 mmHg    │
    │      22.4°C      │         │     PENDULUM     │         │   "Birds of ..." │         │  ТРЕНД: СТАБІЛЬНО│
    │  48%  ·  752 мм  │         │   ⏱ 18m POMO     │         │   Billie Eilish  │         │  ▂▃▅▆▇██▇▆▅▄▃▂   │
    │  WiFi -58dBm [●] │         │  [ 240x240 DMA ] │         │   01:42 ──●── 03:15│       │  [ 24h Sparkline ]│
    └──────────────────┘         └──────────────────┘         └──────────────────┘         └──────────────────┘
```

### Рівень А. Системні вікна
1. **`SYS_BOOT_SPLASH` (2.8 с)**: Швейцарський хронограф — промінь запалює 60 засічок контуру, герб `MAGICBOX`, версія прошивки та статус самодіагностики (`I2C · I2S · PSRAM · OK`) під фірмовий акорд.
2. **`SYS_ONBOARDING_WIZARD`**: 6-кроковий майстер першого запуску з вибором звуку, генерацією QR-кодів для підключення до Wi-Fi Captive Portal та швидкої прив'язки Spotify.
3. **`SYS_NETWORK_DIAG`**: Екран інженерної діагностики за утриманням лівої кнопки (IP, SSID, RSSI, вільна PSRAM/Heap, температура чипа).

### Рівень B. Основне кільце (Короткий клік лівої сенсорної кнопки)
- **Вікно 0 · Клімат кімнати**: 60 хорологічних міток із секундною точкою, дуга комфортної вологості, температура з акцентним десятковим знаком, тиск та статус хмари.
- **Вікно 1 · Живий екран / Заставка**: Анімація `.mbx` з flash-пам'яті, бейдж активного Pomodoro або точний швейцарський годинник-маятник.
- **Вікно 2 · Smart Hub**: Пріоритет 1 — непрочитані повідомлення від Ярослава; Пріоритет 2 — вінілова вертушка Spotify із прогресом треку та текстом; Пріоритет 3 — екран `ALL CAUGHT UP`.
- **Вікно 3 · Барометр Rams**: Стрілка метеорологічного тренду, контрастний тиск і 24-годинний стовпчиковий спарклайн.

---

## 🔘 Сенсорне керування (Touch Controls)

| Сенсор | Клік у системному майстрі | Робочий клік (до 300 мс) | Утримання (від 900 мс) |
|---|---|---|---|
| **Лівий (GPIO 4)** | `[ ◄ НІ / ПРОПУСТИТИ ]` | Наступне вікно ($0 \to 1 \to 2 \to 3 \to 0$) | Виклик вікна діагностики `SYS_NETWORK_DIAG` |
| **Кришка (GPIO 6)** | Тест звуку / Підтвердження | **Зупинити активний звук** / Наступний режим світла ($0..11$) | Увімкнути / Вимкнути підсвітку (Toggle) |
| **Правий (GPIO 7)** | `[ ТАК / ДАЛІ ► ]` | Перемикання годинника / Наступний TG лист / Play-Pause | Наступний трек Spotify / Тестова реакція |

---

## 💡 Світловий рушій (12 режимів SK6812 RGBW)

| № | Ідентифікатор | Атмосфера та призначення |
|---|---|---|
| 0 | `MODE_WARM_WHITE` | Теплий ламповий затишок (2700K, нативний White канал) |
| 1 | `MODE_COLD_WHITE` | Нейтральне робоче денне освітлення для читання |
| 2 | `MODE_SUNSET` | Бурштиновий захід сонця з повільним переливом |
| 3 | `MODE_FIREPLACE` | Процедурний вогонь каміна з випадковим мерехтінням вуглинок |
| 4 | `MODE_AURORA` | Смарагдово-індигове північне сяйво |
| 5 | `MODE_NEON_PULSE` | М'який пульсуючий кіберпанк-неон |
| 6 | `MODE_OCEAN` | Спокійна глибинна бірюзова океанічна хвиля |
| 7 | `MODE_FOREST` | Заспокійливий смарагдово-хвойний лісовий мох |
| 8 | `MODE_BREATHING` | Медитативне дихання 4-7-8 для зняття напруги |
| 9 | `MODE_RAINBOW` | Плавний шовковий спектральний градієнт |
| 10 | `MODE_CANDLE` | Живе тремтіння полум'я воскової свічки |
| 11 | `MODE_SPOTIFY_AMBI` | **Реактивне підсвічування в тон поточного альбому Spotify** |

---

## 🛠️ Апаратне підключення (Pinout ESP32-S3)

```text
  ┌──────────────────────────────────────────────────────────────┐
  │                 ESP32-S3-DevKitC-1 Pin Mapping               │
  ├──────────────────────┬─────────────┬─────────────────────────┤
  │ Модуль периферії     │ GPIO        │ Призначення             │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ GC9A01 IPS Дисплей   │ GPIO 10     │ SPI CS (Chip Select)    │
  │                      │ GPIO 11     │ SPI MOSI (Data)         │
  │                      │ GPIO 12     │ SPI SCLK (Clock 80 MHz) │
  │                      │ GPIO 13     │ DC (Command / Data)     │
  │                      │ GPIO 14     │ RST (Hardware Reset)    │
  │                      │ GPIO 2      │ Backlight PWM           │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ MAX98357A I2S ЦАП    │ GPIO 15     │ I2S BCLK (Bit Clock)    │
  │                      │ GPIO 16     │ I2S LRC (Word Select)   │
  │                      │ GPIO 17     │ I2S DOUT (Serial Data)  │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ SK6812 RGBW (60 LED) │ GPIO 5      │ RMT / NeoPixel Data     │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ Сенсорні зони        │ GPIO 4      │ Лівий сенсор            │
  │                      │ GPIO 6      │ Сенсор кришки           │
  │                      │ GPIO 7      │ Правий сенсор           │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ I2C Шина             │ GPIO 8      │ SDA (SHT31 + BMP280)    │
  │                      │ GPIO 9      │ SCL (SHT31 + BMP280)    │
  ├──────────────────────┼─────────────┼─────────────────────────┤
  │ LDR Фоторезистор     │ GPIO 1      │ ADC1 Ch0 (Яскравість)   │
  └──────────────────────┴─────────────┴─────────────────────────┘
```

---

## 📂 Структура репозиторію

```text
SmartLamp/
├── include/
│   ├── Config.h                  # Системні ліміти, версія v5.6, таймінги
│   ├── Secrets.h                 # Локальні токени (у .gitignore)
│   ├── cloud/
│   │   ├── OracleCloudService.h  # WebSocket клієнт ESP32-S3 (8KB буфер) + HTTP fallback
│   │   ├── CloudOtaService.h     # Надійний OTA-флешер із перевіркою SHA-256
│   │   └── TgJsonUtils.h         # Декодер UTF-8 та парсер JSON
│   ├── core/
│   │   ├── AppState.h            # Спільний стан FreeRTOS, блокування, черги команд
│   │   ├── CommandRouter.h       # Обробка дій кнопок, Spotify, медіа та світла
│   │   └── StorageService.h      # NVS-сховище конфігурацій та Flash-диск
│   ├── hardware/
│   │   ├── AudioService.h        # I2S аудіо-рушій, процедурні акорди, Mute
│   │   ├── InputService.h        # Обробник ємнісних сенсорів (клік/утримання)
│   │   └── SensorService.h       # Драйвери SHT31, BMP280 та аналогового LDR
│   ├── led/
│   │   ├── LedEffects.h          # 12 процедурних шейдерів SK6812 RGBW
│   │   └── LedService.h          # Контролер світлодіодів і плавної анімації
│   └── ui/
│       ├── DisplayDriver.h       # LovyanGFX конфігурація GC9A01 (80 МГц DMA)
│       ├── ScreenRenderer.h      # Графічні процедури 4 екранів та Wizard
│       └── DisplayService.h      # Стейт-машина дисплея та перемикання вікон
├── oracle_server/
│   ├── main.py                   # Точка входу FastAPI + Lifespan + Watchdog
│   ├── deploy_native.sh          # Скрипт розгортання на сервері
│   └── app/
│       ├── api/
│       │   ├── endpoints_box.py     # WebSocket /ws/box + роздача бінарного вінілу
│       │   └── endpoints_control.py # WebSocket /ws/dashboard + REST API студії
│       ├── core/
│       │   ├── events.py         # RealtimeHub: черги, сокети, asyncio.Lock
│       │   └── memory_guard.py   # Контроль споживання RAM (< 65 МБ) та дисків
│       ├── services/
│       │   ├── bot_service.py    # Telegram бот (@SmartLampEdlerAGBot)
│       │   ├── spotify_service.py# Опитування Spotify, Bayer dithering, LRCLIB
│       │   ├── nastya_routine_service.py # Тихі години, Pomodoro, Sunrise
│       │   └── database_service.py# SQLite WAL, мікроклімат, VACUUM бекапи
│       └── web/
│           └── portal_html.py    # Повний SPA-код MagicBox Studio v5.6
├── platformio.ini                # Конфігурація збірки ESP32-S3
└── README.md
```

---

## 🚀 Швидкий старт та розгортання

### 1. Збірка прошивки лампи (ESP32-S3)
Потрібен встановлений [PlatformIO](https://platformio.org/):
```bash
# Клонування репозиторію
git clone https://github.com/EdlerAG/SmartLamp.git
cd SmartLamp

# Налаштування секретів
cp include/Secrets.example.h include/Secrets.h
# Відредагуйте include/Secrets.h (введіть URL вашого сервера та API-ключ)

# Компіляція мікропрограми
pio run

# Прошивка через USB-C (порт UART або Native USB)
pio run -t upload
```

### 2. Запуск бекенда (Oracle Cloud VPS або будь-який Linux-сервер)
```bash
cd oracle_server

# Створення ізольованого середовища
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

# Створення .env файлу за зразком .env.example
cp .env.example .env
nano .env

# Запуск через високопродуктивний Uvicorn з uvloop
uvicorn main:app --host 0.0.0.0 --port 8000 --loop uvloop --http httptools
```

Для створення фонового системного сервісу Linux використовуйте готовий скрипт [`deploy_native.sh`](file:///d:/Vlasne/code/SmartLamp/oracle_server/deploy_native.sh):
```bash
sudo ./deploy_native.sh
```

---

## 🛡️ Безпека, автономність та надійність

1. **Захист від блекаутів**: Якщо сервер або Wi-Fi недоступні, лампа зберігає повноцінну автономність — працюють сенсори, підсвітка, перемикання екранів, таймери та локальне накопичення кліматичних даних.
2. **Нульовий зомбі-стан команд (Command TTL)**: Команди сповіщень та звуку мають час життя 180 с. Застарілі повідомлення ніколи не «вистрілять» посеред ночі після відновлення електропостачання.
3. **Автоматичні щотижневі бекапи**: Сервер щонеділі виконує неблокуючу гарячу реплікацію бази через SQLite `VACUUM INTO` і автоматично надсилає зашифрований архів у Telegram власнику.
4. **Суворий ліміт ресурсів (FinOps / MemoryGuard)**: Споживання RAM бекенда обмежено 65–85 МБ, картинки рендеряться в пам'яті через `/dev/shm` без зношування SSD-диска.

---

<p align="center">
  <b>MagicBox v5.6</b> • Зроблено з любов'ю та інженерною точністю для Насті 🌸<br>
  <i>Designed by Yaroslav (Edler) • 2026</i>
</p>
