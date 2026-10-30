#pragma once
#include <Arduino.h>

enum class EventType : uint8_t {
    BTN_LEFT_SHORT,
    BTN_LEFT_LONG,
    BTN_TOP_SHORT,
    BTN_TOP_LONG,
    BTN_RIGHT_SHORT,
    BTN_RIGHT_LONG,
    NEW_TELEGRAM_MSG
};

struct AppEvent {
    EventType type;
};

// Рівень А: Системні вікна (поза циклом перемикання кнопок)
enum class SystemUiState : uint8_t {
    SYS_BOOT_SPLASH,        // Вітальний екран зі швейцарським хронографом на старті (2.8 с)
    SYS_ONBOARDING_WIZARD,  // Інтерактивний 6-кроковий майстер налаштування (Звук, Wi-Fi, Spotify)
    SYS_NORMAL,             // Звичайний робочий стан (основні 5 екранів)
    SYS_NETWORK_DIAG,       // Сервісний екран QR-коду Web Studio та системної діагностики
    SYS_BIRTHDAY_CELEBRATE  // Святковий екран привітання Насті з Днем Народження (05.11 або ручний тригер)
};

// Покроковий сценарій першого налаштування (Onboarding Wizard v5.5 Sovereign Edition)
enum class OnboardingStep : uint8_t {
    WIZ_STEP_WELCOME_SOUND     = 0, // 0: Привітання та вибір профілю звуку (Тиша / Зі звуком)
    WIZ_STEP_TG_BOT            = 1, // 1: QR Telegram-бота з deep-link claim (c_<claim_secret>)
    WIZ_STEP_WIFI_AP           = 2, // 2: QR точки доступу MagicBox-AP (192.168.4.1)
    WIZ_STEP_CONNECTING        = 3, // 3: Анімація з'єднання з Wi-Fi та хмарним сервером
    WIZ_STEP_WAITING_CHECKLIST = 4, // 4: Очікування налаштування в боті + чекліст su
    WIZ_STEP_DONE              = 5  // 5: Завершення налаштування («Готово ✨»)
};

// Рівень Б: Основні 5 вікон-заставок (перемикаються лівою кнопкою)
enum ScreenMode : uint8_t {
    SCREEN_CLIMATE    = 0,  // Вікно 1: Hallmark Horology & Climate Desk Clock (10 FPS)
    SCREEN_LIVING_GIF = 1,  // Вікно 2: Постійна Жива GIF-Заставка з кешем у LittleFS
    SCREEN_SMART_HUB  = 2,  // Вікно 3: Інтелектуальний Хаб (Telegram Inbox ⇄ Spotify Vinyl Player)
    SCREEN_BAROMETER  = 3,  // Вікно 4: Барограф Braun & Метео-тренд (4 FPS)
    SCREEN_POMODORO   = 4,  // Вікно 5: Hallmark Focus Dial & Pomodoro Timer (регулювання ◄/►)
    SCREEN_COUNT      = 5
};

enum LightModeId : uint8_t {
    MODE_WARM_COZY    = 0,
    MODE_CANDLE       = 1,
    MODE_AMBER_EMBER  = 2,
    MODE_FIREPLACE    = 3,
    MODE_SUNSET_VIBE  = 4,
    MODE_SAKURA_NEON  = 5,
    MODE_AURORA_WAVE  = 6,
    MODE_OCEAN_BREEZE = 7,
    MODE_CYBER_RUNNER = 8,
    MODE_RAINBOW_FLOW = 9,
    MODE_LIGHT_OFF    = 10,
    MODE_SPOTIFY_AMBI = 11, // Динамічний AmbiLight під колір обкладинки Spotify
    TOTAL_LIGHT_MODES = 12
};

enum NotifyLedType : uint8_t {
    NOTIFY_NONE      = 0,
    NOTIFY_CYAN_WAVE = 1,
    NOTIFY_FIRE      = 2,
    NOTIFY_LIGHTNING = 3,
    NOTIFY_CAT       = 4,
    NOTIFY_UFO       = 5,
    NOTIFY_CROWN     = 6,
    NOTIFY_AMBILIGHT = 7   // Динамічна підсвітка під колір активної гіфки/наліпки
};

// Заголовок бінарного пакета анімації .mbx (рівно 16 байт)
struct __attribute__((packed)) MbxHeader {
    char     magic[4];    // "MBX5"
    uint8_t  version;     // 5
    uint8_t  num_frames;  // 1..36
    uint8_t  fps;         // 6..25
    uint8_t  reserved0;
    uint8_t  ambi_r;
    uint8_t  ambi_g;
    uint8_t  ambi_b;
    uint8_t  reserved1;
    uint32_t crc32;
};

struct InboxMessage {
    char sender[36];
    char text[128];       // Підтримка двобайтової UTF-8 кирилиці без обрізання
    char time_received[8];
};

struct SpotifyTrackInfo {
    bool     playing = false;
    uint32_t progress_ms = 0;
    uint32_t duration_ms = 0;
    uint32_t last_sync_ms = 0;
    char     title[64] = {0};
    char     artist[48] = {0};
    char     track_id[36] = {0};
    uint8_t  ambi_r = 56;
    uint8_t  ambi_g = 189;
    uint8_t  ambi_b = 248;
    bool     vinyl_ready = false;
};
