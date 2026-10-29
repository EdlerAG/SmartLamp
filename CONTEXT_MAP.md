# SmartLamp / MagicBox Sovereign v5.2 — Architectural Context Map (`CONTEXT_MAP.md`)

> **Призначення:** Карта кодової бази для миттєвої навігації без зайвого сканування файлів.

---

## 1. Апаратна прошивка ESP32-S3 (`src/` & `include/`)
*Платформа: ESP32-S3 (16MB Flash, 8MB OPI PSRAM), LovyanGFX (GC9A01 240x240), NeoPixel SK6812 RGBW, MAX98357A I2S, SHT31 + BMP280.*

| Модуль / Файл | Відповідальність та ключові точки |
| :--- | :--- |
| [`src/main.cpp`](file:///d:/Vlasne/code/SmartLamp/src/main.cpp) | Точка входу (`setup`, `loop`), розподіл ядер FreeRTOS (`Core 1`: UI/LED/Buttons, `Core 0`: Cloud/Audio/WiFi), обробник подій кнопок `processEvents()` (рядки 155–296) та майстра `handleOnboardingButton()` (рядки 53–153). |
| [`include/Config.h`](file:///d:/Vlasne/code/SmartLamp/include/Config.h) | Глобальні константи, піни периферії (`PIN_LED=5`, `PIN_TFT_*`, `PIN_I2S_*`, `PIN_TOUCH_*`), таймінги Long-Poll, ліміти PSRAM (`MBX_MAX_FRAMES=36`, `VINYL_DIAMETER=136`). |
| [`include/core/Types.h`](file:///d:/Vlasne/code/SmartLamp/include/core/Types.h) | Перерахування екранів (`SCREEN_CLIMATE=0`, `SCREEN_LIVING_GIF=1`, `SCREEN_SMART_HUB=2`, `SCREEN_BAROMETER=3`, `SCREEN_POMODORO=4`), режимів світла (`LightMode`), подій (`EventType`) та заголовка `MbxHeader`. |
| [`include/core/AppState.h`](file:///d:/Vlasne/code/SmartLamp/include/core/AppState.h) | Потокобезпечний стан системи (`portMUX_TYPE mux`), буфери PSRAM (`psram_anim_buf`, `psram_saver_buf`), черга офлайн-телеметрії `offline_buf[144]`, стани Spotify, Календаря, Pomodoro, Будильника та DND. |
| [`include/core/CommandRouter.h`](file:///d:/Vlasne/code/SmartLamp/include/core/CommandRouter.h) | Маршрутизатор команд керування станом (яскравість, режими світла, звук, генерація JSON-телеметрії `buildStatusJson`). |
| [`include/core/StorageService.h`](file:///d:/Vlasne/code/SmartLamp/include/core/StorageService.h) | Робота з NVS (`Preferences`) та файловою системою `LittleFS` (збереження `/default_saver.mbx`, конфігів Wi-Fi, аудіо, токенів). |
| [`include/cloud/OracleCloudService.h`](file:///d:/Vlasne/code/SmartLamp/include/cloud/OracleCloudService.h) | Синхронізація з хмарним сервером (`/api/v5/box/sync?wait=6.0`), парсинг команд `processServerResponse()`, завантаження `.mbx` анімацій (`downloadMbxAnimation`), вінілу (`downloadSpotifyVinylArt`) та вивантаження офлайн-телеметрії (`flushOfflineTelemetry`). |
| [`include/cloud/CloudOtaService.h`](file:///d:/Vlasne/code/SmartLamp/include/cloud/CloudOtaService.h) | Безпечне хмарне OTA-оновлення прошивки з перевіркою SHA-256. |
| [`include/ui/DisplayService.h`](file:///d:/Vlasne/code/SmartLamp/include/ui/DisplayService.h) | Керування дисплеєм GC9A01, динамічне масштабування частоти CPU (`240 MHz` ⇄ `80 MHz`, рядки 123–132), адаптивний FPS (4–30 FPS), спрайти `canvas`, `vinylSprite`, `qr_cache`. |
| [`include/ui/ScreenRenderer.h`](file:///d:/Vlasne/code/SmartLamp/include/ui/ScreenRenderer.h) | Вся графіка Hallmark Horology: Boot Splash, Onboarding Wizard, Клімат, Маятник/GIF-заставка, Smart Hub (Повідомлення + Вініл Spotify + Календар Насті), Барометр, Pomodoro. |
| [`include/ui/HallmarkTheme.h`](file:///d:/Vlasne/code/SmartLamp/include/ui/HallmarkTheme.h) | Дизайн-токени кольорів RGB565 (Dieter Rams / Braun horology palette). |
| [`include/led/LedService.h`](file:///d:/Vlasne/code/SmartLamp/include/led/LedService.h) & [`LedEffects.h`](file:///d:/Vlasne/code/SmartLamp/include/led/LedEffects.h) | Керування 60 світлодіодами SK6812 RGBW, плавний RGB-lerp перехід кольорів (~350мс), автояскравість за фоторезистором LDR, нічне приглушення екрана. |
| [`include/hardware/AudioService.h`](file:///d:/Vlasne/code/SmartLamp/include/hardware/AudioService.h) | Відтворення MP3/OGG стрімів та процедурний математичний синтезатор звуків (кліки, акорди, дзвіночки, реакції) через I2S DMA. |
| [`include/hardware/InputService.h`](file:///d:/Vlasne/code/SmartLamp/include/hardware/InputService.h) | Опитування 3 сенсорних кнопок TTP223 (Left, Top, Right), детекція коротких/довгих натискань. |
| [`include/hardware/SensorService.h`](file:///d:/Vlasne/code/SmartLamp/include/hardware/SensorService.h) | Опитування SHT31 (температура/вологість), BMP280 (тиск + тренд), фоторезистора LDR та запис у «чорну скриньку» при блекауті. |
| [`include/network/NetworkService.h`](file:///d:/Vlasne/code/SmartLamp/include/network/NetworkService.h) | Wi-Fi менеджер, Captive Portal (`192.168.4.1`), синхронізація часу NTP, автономний Telegram fallback під час відсутності зв'язку з Oracle. |

---

## 2. Хмарний Сервер (`oracle_server/`)
*Стек: Python 3.12, FastAPI, Uvicorn (`uvloop`), SQLite WAL, HTTPX, Pillow + NumPy.*
*VPS: `ubuntu@89.168.126.103`, директорія `/home/ubuntu/SmartLamp/oracle_server`, systemd: `smartlamp.service`.*

| Модуль / Файл | Відповідальність та ключові точки |
| :--- | :--- |
| [`oracle_server/main.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/main.py) | Ініціалізація FastAPI, `GZipMiddleware`, супервізор циклу Telegram-бота `_supervised_bot_loop()`. |
| [`oracle_server/tunnel_manager.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/tunnel_manager.py) | Підтримка постійного статичного тунелю Ngrok (`yearling-shy-reviving.ngrok-free.dev`) та автооновлення кнопки `📱 Studio` в Telegram. |
| [`oracle_server/app/core/security.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/core/security.py) | Авторизація `verify_access` та `verify_admin_access` (сесійні куки `mb_session`, `X-Device-Key` для плати, `Bearer` токен). |
| [`oracle_server/app/api/endpoints_box.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/api/endpoints_box.py) | Шлюз для плати ESP32: `/api/v5/box/sync` (Long-Poll), WebSocket `/ws/box`, роздача `/api/v5/spotify/vinyl.rgb565`, `.mbx` анімацій, аудіо та OTA. |
| [`oracle_server/app/api/endpoints_control.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/api/endpoints_control.py) | REST API для Web Studio та інтеграцій: Zero-Click Telegram Auth (`/api/v5/auth/tg-webapp`), `/api/v5/state`, керування світлом, звуком, Spotify OAuth, Google Calendar OAuth (`/api/v5/google/*`). |
| [`oracle_server/app/web/portal_html.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/web/portal_html.py) | Веб-застосунок **MagicBox Sovereign Studio v5.2.0** (5 вкладок: Музика, Світло, Клімат, Рутина, Система; 0ms LocalStorage Hydration; Lazy-Load Plotly). |
| [`oracle_server/app/services/bot_service.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/services/bot_service.py) | Telegram-бот (`@SmartLampEdlerAGBot`), клавіатури `ADMIN_KB` та `GIRL_KB`, обробка голосових, відео/гіфок/стікерів, AI-чат та адмін-команди. |
| [`oracle_server/app/services/spotify_service.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/services/spotify_service.py) | OAuth2 клієнт Spotify, Demand-Driven опитування, генерація круглої платівки 136x136 RGB565 з Bayer 4x4 дизерингом та витягнення кольору AmbiLight. |
| [`oracle_server/app/services/google_calendar_service.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/services/google_calendar_service.py) | Синхронізація подій Google Calendar (OAuth2 + iCal + локальна БД), сповіщення на екран лампи за 15 хв до події. |
| [`oracle_server/app/services/nastya_routine_service.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/services/nastya_routine_service.py) | Світанковий будильник (Sunrise simulation за 15 хв до підйому), тихі години (Quiet Hours), двостороння синхронізація Pomodoro. |
| [`oracle_server/app/services/media_service.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/services/media_service.py) | Конвертація стікерів/GIF/відео у бінарний формат `MBX5` (RGB565 з дизерингом у RAM-диск `/dev/shm`). |
| [`oracle_server/app/services/database_service.py`](file:///d:/Vlasne/code/SmartLamp/oracle_server/app/services/database_service.py) | SQLite WAL база даних: налаштування, сесії, історія клімату, блекаути, медіа-бібліотека, календар. |
