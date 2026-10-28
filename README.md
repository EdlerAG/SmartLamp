# 🪄 SmartLamp — MagicBox v4.5 (Interactive Onboarding, Persistent Timed Media, Volume Control & Voice Clone Edition)

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/Framework-Arduino%20%2F%20FreeRTOS-blue.svg?style=flat-square&logo=arduino" alt="Framework" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/Design-Hallmark%20%2F%20Horology%20Deck-E07A5F.svg?style=flat-square" alt="Design" />
  <img src="https://img.shields.io/badge/Audio-Volume%20%26%20Voice%20Clone-yellow.svg?style=flat-square" alt="Audio" />
  <img src="https://img.shields.io/badge/Infographics-Hallmark%20800x500%20PNG-4EA8DE.svg?style=flat-square" alt="Infographics" />
  <img src="https://img.shields.io/badge/Spotify-Zero--Idle%20Demand-1DB954.svg?style=flat-square&logo=spotify" alt="Spotify" />
  <img src="https://img.shields.io/badge/OTA-Standalone%20Single--Header-52B788.svg?style=flat-square" alt="OTA" />
  <img src="https://img.shields.io/badge/LEDs-SK6812%20RGBW-green.svg?style=flat-square" alt="LEDs" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20Oracle%20Cloud-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/Version-4.5%20Hallmark-brightgreen.svg?style=flat-square" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
</p>

> **MagicBox v4.5** — флагманська розумна лампа-медіахаб на базі **ESP32-S3**, круглому IPS-дисплеї GC9A01 (дизайн **Hallmark Horology**), процедурному звуковому синтезаторі I2S, підтримці клонованого голосу (ElevenLabs, Fish Audio, Edge-TTS), автономному відтворенні зациклених медіа з таймерами, інтерактивному покроковому майстрі налаштування з кнопками «НІ / ТАК», генераторі 800×500 метео-інфографіки та енергоефективному хмарному ядрі **FastAPI** на Oracle VM.

---

## ✨ Ключові інновації версії v4.5

| Модуль | Технічний опис |
|---|---|
| 🧭 **Інтерактивний Onboarding Wizard (6 кроків)** | Покроковий майстер запуску `SYS_ONBOARDING_WIZARD` з прямими підказками на екрані: **`[ ◄ НІ ]` (Left)** та **`[ ТАК ► ]` (Right)**. Кроки: 1) Тест звуку колонки; 2) QR-код точки доступу Wi-Fi; 3) QR-код домашнього вебкабінету; 4) Питання підключення Spotify; 5) Компактний QR-код авторизації через швидкий HTTP 307 Redirect; 6) Фінальний екран готовності `SYSTEM READY`. |
| 🎞️ **Автономні зациклені медіа з таймерами** | Завантажені GIF, наліпки та фото крутяться **безкінечно за замовчуванням** (або за таймером 15 хв, 1 год, 3 год) без обов'язкової прив'язки до 5 реакцій. Дотик до кришки або команда в Telegram / Web миттєво зупиняє медіа та повертає попереднє вікно. На екрані відображається витончений бейдж зворотного відліку `⏱ 14m`. |
| 🔊 **Наскрізне регулювання гучності та Mute** | 22-ступенева шкала гучності (`0..21`), збереження в NVS (`box_state`), 100% апаратне блокування звуку в режимі `MUTED`, пропорційне квадратичне масштабування амплітуди для процедурного синтезатора I2S та окремий перемикач звукового кліку кнопок (`touch_beep`). |
| 📊 **Hallmark Visual Infographics (800×500 PNG)** | Легкий серверний генератор інфографіки на базі Pillow та NumPy в естетиці Hallmark Obsidian (`#08090C`). Відображає кімнатний мікроклімат, вуличну погоду з Open-Meteo, 24-годинний плавний графік температури й барометра з маркерами Min/Max та 7-денну аналітику блекаутів (відсоток аптайму й таймлайн відключень). |
| 🎙️ **Нейро-клонування голосу та мульти-провайдерний TTS** | Підтримка синтезу мовлення через ElevenLabs (`eleven_multilingual_v2`), Fish Audio та безкоштовний Edge-TTS із можливістю швидкого перемикання між голосами Остапа (👨) та Поліни (👩), збереженням персоналізованих голосових пресетів та їх миттєвим озвученням на лампі. |
| 📱 **Нова 3-рядкова клавіатура Telegram** | Лаконічні та ергономічні головні клавіатури для адміністратора та користувача: швидкий вибір світла, статус, вимикання, наліпки, гучність, таймери медіа та надсилання інфографіки. |
| 💌 **Чиста маршрутизація повідомлень** | Виправлено логіку вхідних повідомлень: текст більше не перетворюється примусово на мем-реакцію Котика, а коректно транслюється в Telegram та виводиться на картку Inbox екрана лампи. |
| 🛡️ **Ізольований Standalone OTA** | Недоторканний однофайловий модуль `StandaloneOta.h` із валідацією заголовків `0xE9`, контролем SHA256 та обов'язковим `confirmBootValid()` при старті. |

---

## 🖥️ Екрани дисплея: Системні вікна та 4 Заставки

### Рівень А. Системні стани (поза основним циклом)
1. **`SYS_BOOT_SPLASH` (2.8 с при старті):**
   - Швейцарський хронограф: промінь послідовно засвічує 60 засічок по контуру ($R = 116$).
   - У центрі проявляється герб `MAGICBOX`, версія `HALLMARK v4.5` та діагностика периферії (`I2C · I2S · PSRAM · OK`).
   - Синхронно звучить м'який процедурний акорд сходження (з урахуванням збереженої гучності).
2. **`SYS_ONBOARDING_WIZARD` (Інтерактивний майстер налаштування):**
   - **Крок 1 (`WIZ_STEP_WELCOME_SOUND`):** Вітання `WELCOME TO MAGICBOX`, питання `УВІМКНУТИ ЗВУК КОЛОНКИ?`, перевірочний акорд, кнопки `[ ◄ ТИША ]` / `[ ЗВУК ► ]`.
   - **Крок 2 (`WIZ_STEP_WIFI_AP`):** QR-код підключення до Wi-Fi `MagicBox-Setup`, кнопка `[ ПРОПУСТИТИ ► ]`.
   - **Крок 3 (`WIZ_STEP_WIFI_PORTAL`):** QR-код переходу в веб-студію `http://192.168.4.1/`.
   - **Крок 4 (`WIZ_STEP_SPOTIFY_ASK`):** Питання `ПІДКЛЮЧИТИ SPOTIFY?`, кнопки `[ ◄ НІ ]` / `[ ТАК ► ]`.
   - **Крок 5 (`WIZ_STEP_SPOTIFY_QR`):** Компактний QR-код швидкої авторизації (через `GET /api/v5/spotify/login_redirect`).
   - **Крок 6 (`WIZ_STEP_DONE`):** Повідомлення `SYSTEM READY` та плавний перехід у робочий цикл.
3. **`SYS_NETWORK_DIAG` (Утримання лівої кнопки на 900 мс):**
   - QR-код для переходу в Studio за поточною IP-адресою, SSID, RSSI (dBm), PSRAM, температура процесора та стан аудіо.

### Рівень B. Основне кільце з 4 Вікон-Заставок (Короткий клік лівої кнопки)
```text
[ 0: CLIMATE ] ──► [ 1: LIVING GIF ] ──► [ 2: SMART HUB ] ──► [ 3: BAROMETER ] ──┐
      ▲                                                                          │
      └──────────────────────────────────────────────────────────────────────────┘
```
1. **Вікно 0 · `SCREEN_CLIMATE` (10 FPS):**
   - 60 хорологічних засічок із бігаючою точкою поточної секунди.
   - Тонка дуга вологості (40–60%).
   - Hero-температура `22.4°C` з акцентним десятковим знаком Alabaster / Terracotta.
   - Bento-колонки Вологість / Тиск, статус хмари та індикатор Mute у нижньому колонтитулі.
2. **Вікно 1 · `SCREEN_LIVING_GIF` (Динамічний FPS з файлу або 30 FPS):**
   - Відтворення активної зацикленої анімації або постійної заставки з Flash (`/default_saver.mbx`).
   - Якщо активний таймер — відображається витончений бейдж `⏱ 14m`.
   - За відсутності файлу — швейцарський маятник `Hallmark Swiss Pendulum Clock` з можливістю приховування годинника.
3. **Вікно 2 · `SCREEN_SMART_HUB` (Динамічний 3-рівневий медіахаб):**
   - **Пріоритет 1 (Непрочитані TG):** Інтелектуальний Inbox із підсвічуванням автора, дати та пагінацією 1/5.
   - **Пріоритет 2 (Spotify Active):** Вінілова вертушка 136×136, тонарм, біжучий рядок треку/виконавця, дуга прогресу та AmbiLight-синхронізація.
   - **Пріоритет 3 (Idle):** Редакційна картка `INBOX ZERO / ALL CAUGHT UP`.
4. **Вікно 3 · `SCREEN_BAROMETER` (4 FPS · Braun Dieter Rams Style):**
   - Великий контрастний тиск у мм рт. ст.: `750 mmHg`.
   - Векторна стрілка тренду ($\uparrow, \downarrow, \rightarrow$) та метеорологічний вердикт.
   - 24-годинний стовпчиковий Sparkline із базовою лінією на 750 мм.

---

## 🔘 Сенсорне керування (Touch Controls v4.5)

| Кнопка | Режим Onboarding Wizard | Робочий режим (Короткий дотик) | Робочий режим (Утримання 900 мс) |
|---|---|---|---|
| **Ліва (GPIO 4)** | **`[ ◄ НІ / ПРОПУСТИТИ ]`** (Назад / відмова) | Наступне вікно ($0 \to 1 \to 2 \to 3 \to 0$) | Вікно діагностики (`SYS_NETWORK_DIAG`) |
| **Кришка (GPIO 6)** | Повтор тестового звуку / перемикання | **Зупинити активне медіа** (якщо грає) / Наступний режим світла ($0..11$) | Увімкнути / Вимкнути підсвітку |
| **Права (GPIO 7)** | **`[ ТАК / ДАЛІ ► ]`** (Підтвердження) | Годинник на заставці / Наступний TG Inbox / Play-Pause Spotify / Яскравість | Наступний трек Spotify / Запуск реакцій зі звуком |

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
12. `MODE_SPOTIFY_AMBI` — Реактивне підсвічування в тон поточного альбому Spotify

---

## 📦 Структура репозиторію

```text
📦 SmartLamp
 ┣ 📂 include
 ┃ ┣ 📜 Config.h                     // FW_VERSION v4.5-Hallmark, DEFAULT_VOLUME 12
 ┃ ┣ 📜 Secrets.h                    // (У .gitignore) Токени TG, Spotify та ключі Oracle
 ┃ ┣ 📜 Secrets.example.h            // Шаблон конфігурації
 ┃ ┃
 ┃ ┣ 📂 core                         // Ядро системи
 ┃ ┃ ┣ 📜 Types.h                    // OnboardingStep, SystemUiState, ScreenMode, LightModeId
 ┃ ┃ ┣ 📜 AppState.h                 // Гучність, Mute, TouchBeep, Медіа-таймери, Spinlock
 ┃ ┃ ┣ 📜 CommandRouter.h            // Обробка гучності, Mute, зупинки медіа та таймерів
 ┃ ┃ ┗ 📜 StorageService.h           // Збереження a_vol, a_mut, t_beep в NVS та Flash заставки
 ┃ ┃
 ┃ ┣ 📂 hardware                     // Сенсори, Звук та Ввід
 ┃ ┃ ┣ 📜 InputService.h             // Драйвер ємнісних сенсорів (Short / Long press)
 ┃ ┃ ┣ 📜 SensorService.h            // SHT31 + BMP280 + АЦП LDR
 ┃ ┃ ┗ 📜 AudioService.h             // Неблокуючий I2S DSP синтезатор із регулюванням гучності й MUTE
 ┃ ┃
 ┃ ┣ 📂 led                          // Світловий рушій SK6812 (60 діодів)
 ┃ ┃ ┣ 📜 LedEffects.h               // 12 математичних шейдерів (включно зі Spotify Ambi)
 ┃ ┃ ┗ 📜 LedService.h               // Контролер яскравості та авто-димінгу
 ┃ ┃
 ┃ ┣ 📂 ui                           // Графічний рушій GC9A01 (240x240)
 ┃ ┃ ┣ 📜 DisplayDriver.h            // SPI 80 МГц + DMA
 ┃ ┃ ┣ 📜 TextUtils.h                // UTF-8 розбиття тексту
 ┃ ┃ ┣ 📜 ReactionRenderer.h         // Векторні реакції (🔥, ⚡, 🐱, 👽, 👑)
 ┃ ┃ ┣ 📜 ScreenRenderer.h           // Рендерер 6-крокового Wizard, заставок і бейджів таймера
 ┃ ┃ ┗ 📜 DisplayService.h           // Стейт-машина дисплея та авто-скидання таймерів медіа
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
 ┃   ┣ 📜 TgKeyboards.h              // Генератор клавіатур бота (3-рядкова ергономіка v4.5)
 ┃   ┣ 📜 TelegramService.h          // Telegram Bot API (TLS Session)
 ┃   ┗ 📜 OracleCloudService.h       // Long-Poll синхронізація, телеметрія гучності та Spotify
 ┃
 ┣ 📂 src
 ┃ ┗ 📜 main.cpp                     // setup() з confirmBootValid() та маршрутизація кнопок Onboarding
 ┃
 ┗ 📂 oracle_server                  // Сервер для Oracle Cloud VM (< 28 МБ RAM)
   ┣ 📜 deploy_native.sh             // 1-клік інсталятор для Linux (systemd + venv)
   ┣ 📜 requirements.txt             // Залежності (fastapi, pillow, numpy, httpx, orjson, edge-tts)
   ┣ 📜 main.py                      // Точка входу FastAPI + Lifespan Watchdog
   ┗ 📂 app
     ┣ 📂 core                       // config.py, memory_guard.py, security.py
     ┣ 📂 domain                     // schemas.py (валідація Pydantic v2, гучність, голос)
     ┣ 📂 services                   // infographic_service.py (800x500 PNG), audio_service (Voice Clone),
     ┃                               // media_service (Bayer Dithering + Timers), spotify_service (Zero-Idle)
     ┣ 📂 api                        // router.py, endpoints_box.py, endpoints_control.py
     ┗ 📂 web                        // portal_html.py (Hallmark Web Studio v4.5)
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
