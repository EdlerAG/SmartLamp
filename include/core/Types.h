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

enum ScreenMode : uint8_t {
    SCREEN_CLIMATE   = 0,
    SCREEN_BAROMETER = 1,
    SCREEN_TELEGRAM  = 2,
    SCREEN_NETWORK   = 3,
    SCREEN_COUNT     = 4
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
    TOTAL_LIGHT_MODES = 11
};

enum NotifyLedType : uint8_t {
    NOTIFY_NONE      = 0,
    NOTIFY_CYAN_WAVE = 1,
    NOTIFY_FIRE      = 2,
    NOTIFY_LIGHTNING = 3,
    NOTIFY_CAT       = 4,
    NOTIFY_UFO       = 5,
    NOTIFY_CROWN     = 6
};

struct InboxMessage {
    char sender[28];
    char text[64];
    char time_received[8];
};