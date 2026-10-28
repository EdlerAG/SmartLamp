# 🪄 SmartLamp — MagicBox v5.0 (Nastya Edition • Routines, Calendar & Sovereign Care)

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg?style=flat-square&logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/Framework-Arduino%20%2F%20FreeRTOS-blue.svg?style=flat-square&logo=arduino" alt="Framework" />
  <img src="https://img.shields.io/badge/Display-GC9A01%20240x240%20IPS-purple.svg?style=flat-square" alt="Display" />
  <img src="https://img.shields.io/badge/Design-Hallmark%20%2F%20Horology%20Deck-E07A5F.svg?style=flat-square" alt="Design" />
  <img src="https://img.shields.io/badge/Care-Nastya%20Routines%20%26%20Pomodoro-e63946.svg?style=flat-square" alt="Care" />
  <img src="https://img.shields.io/badge/Privacy-Google%20Calendar%20Zero--Knowledge-4EA8DE.svg?style=flat-square" alt="Calendar" />
  <img src="https://img.shields.io/badge/Audio-Volume%20%26%20Voice%20Clone-yellow.svg?style=flat-square" alt="Audio" />
  <img src="https://img.shields.io/badge/Infographics-Hallmark%20800x500%20PNG-4EA8DE.svg?style=flat-square" alt="Infographics" />
  <img src="https://img.shields.io/badge/Spotify-Zero--Idle%20Demand-1DB954.svg?style=flat-square&logo=spotify" alt="Spotify" />
  <img src="https://img.shields.io/badge/OTA-Standalone%20Single--Header-52B788.svg?style=flat-square" alt="OTA" />
  <img src="https://img.shields.io/badge/Backend-FastAPI%20%2F%20Oracle%20Cloud-red.svg?style=flat-square&logo=fastapi" alt="Backend" />
  <img src="https://img.shields.io/badge/Version-5.0%20Nastya%20Edition-brightgreen.svg?style=flat-square" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
</p>

> **MagicBox v5.0 (Nastya Edition)** — персоналізована розумна лампа-медіахаб на базі **ESP32-S3**, круглому IPS-дисплеї GC9A01 (дизайн **Hallmark Horology**), розроблена як теплий подарунок на день народження для Насті. Лампа поєднує турботу про режим сну, навчання Pomodoro, персональний Google Календар із нульовим доступом третіх осіб (Zero-Knowledge Privacy), автоматичний розумний світанок, процедурний акорд сходження сонця, погодні та одежні поради, а також надійні інструменти автоматичного бекапу та системного моніторингу для Ярослава.

---

## ✨ Ключові можливості версії v5.0

| Модуль | Технічний опис |
|---|---|
| 🌸 **Турбота про затишок Насті (Phase 4)** | **Тихі години (23:00–08:00):** авто-приглушення яскравості до 15% та повна тиша (Mute). **Розумний світанок:** за 20 хв до підйому плавне наростання сонячного світла (1% → 100%) та мелодійний ранковий акорд. **Ранковий гід:** персоналізований звіт про погоду та практична порада щодо одягу й парасольки. |
| 📚 **Навчальний фокус-таймер Pomodoro** | 25 хв глибокого фокусу під затишне бурштинове світло без зайвих сповіщень + 5 хв відпочинку під спокійний смарагдовий бриз із м'яким звуковим сигналом завершення. Керування через веб, Telegram або фізичні кнопки. |
| 📅 **Приватний Google Календар (Zero-Knowledge)** | Незалежна OAuth2-авторизація для Насті через Web Studio. Сервер запитує виключно 1 найближчу подію (`events.list?maxResults=1`), коли до неї лишається $\le 15$ хв, і виводить нагадування на Smart Hub. У панелі Ярослава події календаря не зберігаються і не відображаються. |
| 🩺 **Діагностика та щотижневі бекапи для Ярослава (Phase 6)** | Неблокуюче створення резервних копій SQLite через `VACUUM INTO` із щотижневою відправкою архіву в особисті повідомлення Telegram (`TG_ADMIN_ID`). Ендпоінт `/api/v5/admin/health-report` та команди `/health`, `/backup` для повної діагностики живлення, пам'яті та телеметрії лампи. |
| 🎂 **Захищений вхід за днем народження (PIN `0511`)** | Веб-студія підтримує швидкий та зручний вхід за датою дня народження Насті (`0511` — 5 листопада) з захистом від підбору паролів (Rate Limit: 5 спроб на 15 хв) та криптографічними сесійними токенами в базі даних. |
| ⚡ **Енергозбереження та тепловий баланс ESP32-S3 (Phase 3)** | Динамічне масштабування частоти CPU: 80 МГц у стані спокою на статичних екранах (мінімальний нагрів датчика SHT31/BMP280) та 240 МГц під час відтворення GIF/аудіо. Режим DND за довгим утриманням сенсора кришки. |
| 🛡️ **Zero-Zombie Command TTL (Phase 2)** | TTL для команд: 180 с для аудіо, мовлення й реакцій, 24 год для системних налаштувань. Застарілі команди автоматично скидаються, запобігаючи несподіваному пробудженню лампи після блекауту. |

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
