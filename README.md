# 🪄 SmartLamp — MagicBox v4.0 (Hallmark Screensavers, Zero-Idle Spotify & Procedural I2S Edition)

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/Framework-Arduino%20%2F%20FreeRTOS-blue.svg?style=flat-square&logo=arduino" alt="Framework" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/Design-Hallmark%20%2F%20Horology%20Deck-E07A5F.svg?style=flat-square" alt="Design" />
  <img src="https://img.shields.io/badge/Audio-Procedural%20I2S%20PCM%20Synth-yellow.svg?style=flat-square" alt="Audio" />
  <img src="https://img.shields.io/badge/Spotify-Zero--Idle%20Demand-1DB954.svg?style=flat-square&logo=spotify" alt="Spotify" />
  <img src="https://img.shields.io/badge/Dithering-Bayer%204x4%20TrueColor-4EA8DE.svg?style=flat-square" alt="Dithering" />
  <img src="https://img.shields.io/badge/OTA-Standalone%20Single--Header-52B788.svg?style=flat-square" alt="OTA" />
  <img src="https://img.shields.io/badge/LEDs-SK6812%20RGBW-green.svg?style=flat-square" alt="LEDs" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20Oracle%20Cloud-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/Version-4.0%20Hallmark-brightgreen.svg?style=flat-square" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
</p>

> **MagicBox v4.0** — преміальна розумна лампа-медіахаб на базі **ESP32-S3**, оснащена круглим IPS-дисплеєм GC9A01 з естетикою швейцарської хорології за системою **Hallmark**, процедурним звуковим синтезатором I2S без затримок циклу, нульовим навантаженням на процесор у режимі очікування для **Spotify Vinyl Deck**, квантуванням кольорів **Bayer 4×4 Ordered Dithering**, незалежним захистом прошивки **Standalone Remote OTA** та енергоефективним бекендом **FastAPI** на Oracle Cloud.

---

## ✨ Ключові інновації версії v4.0

| Модуль | Технічний опис |
|---|---|
| 🪟 **Дворівнева архітектура вікон** | Чітке розділення на **Рівень А: Системні вікна** (`SYS_BOOT_SPLASH` із променем швейцарського хронографа $R=116$, `SYS_ONBOARDING_WIZARD` 3-кроковий майстер налаштування AP із QR, `SYS_NETWORK_DIAG` діагностика за довгим кліком) та **Рівень B: 4 Hallmark Вікна-Заставки** (`CLIMATE`, `LIVING_GIF`, `SMART_HUB`, `BAROMETER`). |
| 🎵 **Zero-Idle Demand Spotify Deck** | Повна інтеграція зі Spotify Web API (OAuth2 PKCE/Code Flow). **0.00% CPU на сервері та 0 запитів у фоні**: сервер опитує Spotify кожні 6 с *виключно тоді*, коли на лампі активне Вікно 2 або світловий режим Spotify AmbiLight (`MODE_SPOTIFY_AMBI = 11`). |
| 💽 **Кругла вінілова платівка (136×136)** | Сервер на льоту нарізає обкладинку альбому в коло діаметром 136 px, накладає шпиндельний отвір і зберігає в RAM-диск `/dev/shm/magicbox_ram/spotify_vinyl.rgb565`. На дисплеї платівка плавно обертається в PSRAM-спрайті зі швидкістю 33⅓ RPM, віртуальним тонармом, інтерпольованою дугою треку та біжучим рядком назви. |
| 🎨 **Bayer 4×4 Ordered Dithering** | Векторизований NumPy алгоритм просторового розмиття помилки квантування під час конвертації в RGB565. Повністю ліквідовує ступінчасті градієнти (color banding) на темних і пастельних зображеннях на 16-бітному IPS-дисплеї. |
| 🔊 **Процедурний I2S PCM Синтезатор** | Чистий математичний синтез гармонік у реальному часі (sine, saw, chirp, envelope) безпосередньо у DMA-буфер I2S порціями по 128 семплів. Миттєвий тактильний клік при дотику, акорд завантаження, кришталевий дзвіночок сповіщень та 5 фірмових мем-звуків. |
| 💾 **LittleFS Постійна Заставка (`/default_saver.mbx`)** | Збереження улюбленої анімації в пам'ять Flash з перевіркою CRC32 та автоматичним завантаженням при старті. Якщо файл відсутній — автоматично працює витончений швейцарський маятник `Hallmark Swiss Pendulum Clock` з можливістю приховування часу. |
| 🛡️ **Ізольований Standalone OTA** | Недоторканний однофайловий модуль `StandaloneOta.h` із захистом від окрирплення, валідацією заголовків `0xE9`, контролем SHA256 на льоту та обов'язковим `confirmBootValid()` при старті. |

---

## 🖥️ Екрани дисплея: Системні вікна та 4 Заставки

### Рівень А. Системні стани (поза основним циклом)
1. **`SYS_BOOT_SPLASH` (2.8 с при старті):**
   - Швейцарський хронограф: промінь послідовно засвічує 60 засічок по контуру ($R = 116$).
   - У центрі плавно проявляється герб `MAGICBOX`, версія `HALLMARK v4.0` та діагностика периферії (`I2C · I2S · PSRAM · OK`).
   - Синхронно звучить м'який процедурний акорд сходження.
2. **`SYS_ONBOARDING_WIZARD` (Перший старт або режим точки доступу AP):**
   - **Крок 1 (Очікування клієнта):** QR-код підключення до Wi-Fi `MagicBox-Setup` та стрілка запрошення.
   - **Крок 2 (Клієнт підключився):** QR-код переходу в веб-студію `http://192.168.4.1/`.
   - **Крок 3 (Збереження):** Анімація підключення до домашньої мережі.
3. **`SYS_NETWORK_DIAG` (Утримання лівої кнопки на 900 мс):**
   - Швидкий виклик діагностики: QR-код для переходу в Studio за поточною IP-адресою, ім'я мережі SSID, рівень сигналу RSSI (dBm), стан пам'яті PSRAM та температура процесора.

### Рівень B. Основне кільце з 4 Вікон-Заставок (Короткий клік лівої кнопки)
```text
[ 0: CLIMATE ] ──► [ 1: LIVING GIF ] ──► [ 2: SMART HUB ] ──► [ 3: BAROMETER ] ──┐
      ▲                                                                          │
      └──────────────────────────────────────────────────────────────────────────┘
```
1. **Вікно 0 · `SCREEN_CLIMATE` (10 FPS):**
   - 60 хорологічних засічок із бігаючою точкою поточної секунди.
   - Тонка дуга вологості з плавною індикацією норми (40–60%).
   - Hero-температура `22.4°C` з акцентним десятковим знаком кольору Alabaster / Terracotta.
   - Дві Bento-колонки Вологість / Тиск, статус хмари та назва активного світлового режиму.
2. **Вікно 1 · `SCREEN_LIVING_GIF` (Динамічний FPS з файлу або 30 FPS):**
   - Відтворення постійної анімації з Flash-пам'яті (`/default_saver.mbx`), призначеної користувачем.
   - Якщо файл відсутній — елегантний швейцарський маятник `Hallmark Swiss Pendulum Clock`.
   - Короткий клік правою кнопкою вмикає/вимикає оверлей годинника.
3. **Вікно 2 · `SCREEN_SMART_HUB` (Динамічний 3-рівневий медіахаб):**
   - **Пріоритет 1 (Непрочитані TG):** Інтелектуальний Inbox із підсвічуванням автора, дати та пагінацією 1/5.
   - **Пріоритет 2 (Spotify Active):** Вінілова вертушка 136×136, тонарм, біжучий рядок пісні та виконавця, дуга прогресу треку (з мілісекундною екстраполяцією) та AmbiLight-синхронізація кольору.
   - **Пріоритет 3 (Idle):** Редакційна картка `INBOX ZERO / ALL CAUGHT UP` у стилі швейцарського плаката.
4. **Вікно 3 · `SCREEN_BAROMETER` (4 FPS · Braun Dieter Rams Style):**
   - Великий контрастний тиск у міліметрах ртутного стовпчика: `750 mmHg`.
   - Векторна стрілка барометричного тренду ($\uparrow, \downarrow, \rightarrow$) та метеорологічний вердикт.
   - 24-годинний стовпчиковий Sparkline із базовою лінією на 750 мм та теракотовим останнім стовпчиком.

---

## 🔘 Сенсорне керування (Touch Controls v4.0)

Всі натискання супроводжуються миттєвим апаратним процедурним кліком через I2S:

| Кнопка | Короткий дотик | Утримання (900 мс) |
|---|---|---|
| **Ліва (Touch Left / GPIO 4)** | Наступне вікно-заставка ($0 \to 1 \to 2 \to 3 \to 0$) | Виклик / закриття вікна діагностики (`SYS_NETWORK_DIAG`) |
| **Кришка (Touch Top / GPIO 6)** | Наступний режим світла ($0..11$) / Скинути сповіщення | Увімкнути / Вимкнути підсвітку |
| **Права (Touch Right / GPIO 7)** | **На Вікні 1 (Living GIF):** Перемикання годинника поверх заставки<br>**На Вікні 2 (Telegram):** Наступне повідомлення ($1..5$)<br>**На Вікні 2 (Spotify):** Пауза / Відтворення треку (`play/pause`)<br>**На інших вікнах:** Яскравість (Авто $\to$ 15% $\to$ 35% $\to$ 65% $\to$ 100%) | **На Вікні 2 (Spotify):** Наступний трек (`next`)<br>**На інших вікнах:** Запуск 5 мем-реакцій (🔥 $\to$ ⚡ $\to$ 🐱 $\to$ 👽 $\to$ 👑) зі звуком |

---

## 💡 Світловий рушій (12 режимів SK6812 RGBW)

1. `MODE_WARM_WHITE` — Тепле затишне світло (2700K)
2. `MODE_COLD_WHITE` — Робоче нейтральне світло
3. `MODE_SUNSET` — Захід сонця з плавним переливом
4. `MODE_FIREPLACE` — Мерехтіння каміна (процедурний вогонь)
5. `MODE_AURORA` — Північне сяйво (смарагдово-індигові хвилі)
6. `MODE_NEON_PULSE` — Пульсуючий кіберпанк-неон
7. `MODE_OCEAN` — Глибинний бірюзовий океан
8. `MODE_FOREST` — Спокійний лісовий мох
9. `MODE_BREATHING` — Медитативне дихання
10. `MODE_RAINBOW` — Шовковий спектральний градієнт
11. `MODE_CANDLE` — Жива тремтяча свічка
12. **`MODE_SPOTIFY_AMBI` (НОВЕ v4.0)** — Реактивне підсвічування в тон поточного альбому Spotify із м'яким пульсуванням

---

## 📦 Структура репозиторію

```text
📦 SmartLamp
 ┣ 📂 include
 ┃ ┣ 📜 Config.h                     // Константи (FW_VERSION v4.0, VINYL_DIAMETER 136, SAVER_FILE_PATH)
 ┃ ┣ 📜 Secrets.h                    // (У .gitignore) Токени TG, Spotify та ключі Oracle
 ┃ ┣ 📜 Secrets.example.h            // Шаблон конфігурації
 ┃ ┃
 ┃ ┣ 📂 core                         // Ядро системи
 ┃ ┃ ┣ 📜 Types.h                    // SystemUiState, ScreenMode, SpotifyTrackInfo, LightModeId
 ┃ ┃ ┣ 📜 AppState.h                 // Потокобезпечна шина даних (Spinlock Mux + Spotify + LittleFS)
 ┃ ┃ ┣ 📜 CommandRouter.h            // Диспетчер команд для кнопок, Web, TG та Oracle
 ┃ ┃ ┗ 📜 StorageService.h           // Менеджер NVS та LittleFS (/default_saver.mbx)
 ┃ ┃
 ┃ ┣ 📂 hardware                     // Сенсори, Звук та Ввід
 ┃ ┃ ┣ 📜 InputService.h             // Драйвер ємнісних сенсорів (Short / Long press)
 ┃ ┃ ┣ 📜 SensorService.h            // Неблокуючий SHT31 + BMP280 + АЦП LDR
 ┃ ┃ ┗ 📜 AudioService.h             // Неблокуючий I2S DSP синтезатор + MP3/WAV плеєр
 ┃ ┃
 ┃ ┣ 📂 led                          // Світловий рушій SK6812 (60 діодів)
 ┃ ┃ ┣ 📜 LedEffects.h               // 12 математичних шейдерів (включно зі Spotify Ambi)
 ┃ ┃ ┗ 📜 LedService.h               // Контролер яскравості та авто-димінгу
 ┃ ┃
 ┃ ┣ 📂 ui                           // Графічний рушій GC9A01 (240x240)
 ┃ ┃ ┣ 📜 DisplayDriver.h            // SPI 80 МГц + DMA
 ┃ ┃ ┣ 📜 TextUtils.h                // UTF-8 розбиття тексту
 ┃ ┃ ┣ 📜 ReactionRenderer.h         // Векторні реакції (🔥, ⚡, 🐱, 👽, 👑)
 ┃ ┃ ┣ 📜 ScreenRenderer.h           // Рендерер Swiss Chronograph, Wizard, 4 Заставок та Spotify Deck
 ┃ ┃ ┗ 📜 DisplayService.h           // Мультистейт рендеринг, PSRAM vinylSprite та LittleFS заставки
 ┃ ┃
 ┃ ┣ 📂 network                      // Локальна мережа та Web-портал
 ┃ ┃ ┣ 📜 WifiScanner.h              // Pre-Scan ефіру та генерація JSON
 ┃ ┃ ┣ 📜 WebPortalHtml.h            // Hallmark CSS-токени, Atelier Dark тема
 ┃ ┃ ┣ 📜 PortalServer.h             // REST API + Captive Portal + CORS
 ┃ ┃ ┗ 📜 NetworkService.h           // Wi-Fi стейт-машина та Wizard тригери
 ┃ ┃
 ┃ ┗ 📂 cloud                        // Хмарні сервіси
 ┃   ┣ 📜 StandaloneOta.h            // Недоторканний zero-dependency single-header OTA flasher
 ┃   ┣ 📜 CloudOtaService.h          // Інтеграція StandaloneOta з AppState
 ┃   ┣ 📜 TgJsonUtils.h              // Парсер JSON, URL-кодер та Unicode-декодер
 ┃   ┣ 📜 TgKeyboards.h              // Генератор клавіатур бота
 ┃   ┣ 📜 TelegramService.h          // Telegram Bot API (TLS Session)
 ┃   ┗ 📜 OracleCloudService.h       // Long-Poll синхронізація, Spotify стан, завантаження vinyl.rgb565
 ┃
 ┣ 📂 src
 ┃ ┗ 📜 main.cpp                     // setup() з confirmBootValid() та диспетчеризація FreeRTOS
 ┃
 ┗ 📂 oracle_server                  // Сервер для Oracle Cloud VM (< 28 МБ RAM)
   ┣ 📜 deploy_native.sh             // 1-клік інсталятор для Linux (systemd + venv)
   ┣ 📜 requirements.txt             // Залежності (fastapi, pillow, numpy, httpx, orjson)
   ┣ 📜 main.py                      // Точка входу FastAPI + Lifespan Watchdog
   ┗ 📂 app
     ┣ 📂 core                       // config.py, memory_guard.py, security.py
     ┣ 📂 domain                     // schemas.py (валідація Pydantic v2)
     ┣ 📂 services                   // spotify_service.py (Zero-Idle), media_service (Bayer), ota_service
     ┣ 📂 api                        // router.py, endpoints_box.py, endpoints_control.py
     ┗ 📂 web                        // portal_html.py (Hallmark Web Studio + Spotify Deck)
```

---

## 🛠️ Апаратне забезпечення (Pinout)

- **ESP32-S3-DevKitC-1-N16R8** (16 МБ Flash, 8 МБ OPI PSRAM)
- **GC9A01 240×240 IPS**: CS: 10, MOSI: 11, SCLK: 12, DC: 13, RST: 14 (SPI 80 МГц DMA)
- **MAX98357A I2S DAC**: BCLK: 15, LRC: 16, DOUT: 17
- **SK6812 RGBW стрічка**: Data: 5 (60 діодів)
- **Сенсорні кнопки**: Left: 4, Top: 6, Right: 7
- **I2C шина**: SDA: 8, SCL: 9 (SHT31 + BMP280)
- **Фоторезистор LDR**: ADC: 1

---

## 🚀 Збірка та прошивка

### Збірка мікропрограми (PlatformIO):
```bash
pio run
pio run -t upload
```

### Запуск Oracle сервера локально:
```bash
cd oracle_server
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000
```
