#pragma once
#include <Arduino.h>

namespace Config {
    constexpr const char* FW_VERSION  = "v4.1-ZeroIdle";
    constexpr const char* DEVICE_NAME = "magicbox";
    constexpr const char* AP_SSID     = "MagicBox-Setup";

    // --- СЕКРЕТИ TELEGRAM ТА ORACLE CLOUD ---
#if __has_include("Secrets.h")
    #include "Secrets.h"
    constexpr const char* DEFAULT_TG_TOKEN  = Secrets::TG_TOKEN;
    constexpr const char* DEFAULT_ADMIN_ID  = Secrets::ADMIN_ID;
    constexpr const char* DEFAULT_GIRL_ID   = Secrets::GIRL_ID;
    constexpr const char* ORACLE_SERVER_URL = Secrets::ORACLE_SERVER_URL;
    constexpr const char* ORACLE_API_KEY    = Secrets::ORACLE_API_KEY;
#else
    constexpr const char* DEFAULT_TG_TOKEN  = "";
    constexpr const char* DEFAULT_ADMIN_ID  = "";
    constexpr const char* DEFAULT_GIRL_ID   = "";
    constexpr const char* ORACLE_SERVER_URL = "";
    constexpr const char* ORACLE_API_KEY    = "magicbox-secret-key";
#endif

    constexpr uint32_t TG_POLL_MS     = 2500;
    constexpr uint32_t ORACLE_SYNC_MS = 5000;

    // --- КАЛІБРУВАННЯ ДАТЧИКІВ У КОРПУСІ ---
    constexpr float TEMP_OFFSET_C = 0.0f;

    // --- ТАЙМІНГИ ДЛЯ ВІДКЛЮЧЕНЬ СВІТЛА (BLACKOUT) ---
    constexpr uint32_t WIFI_BOOT_TIMEOUT_MS    = 8000;
    constexpr uint32_t AP_PORTAL_TIMEOUT_MS    = 120000;
    constexpr uint32_t BLACKOUT_PROBE_EVERY_MS = 30000;

    // --- ПІНИ ПЕРИФЕРІЇ ---
    constexpr uint8_t PIN_LDR         = 1;
    constexpr uint8_t PIN_TOUCH_LEFT  = 4;
    constexpr uint8_t PIN_LED         = 5;
    constexpr uint8_t PIN_TOUCH_TOP   = 6;
    constexpr uint8_t PIN_TOUCH_RIGHT = 7;
    constexpr uint8_t PIN_I2C_SDA     = 8;
    constexpr uint8_t PIN_I2C_SCL     = 9;

    // --- ПІНИ ДИСПЛЕЯ GC9A01 (SPI) ---
    constexpr int8_t PIN_TFT_CS       = 10;
    constexpr int8_t PIN_TFT_MOSI     = 11;
    constexpr int8_t PIN_TFT_SCLK     = 12;
    constexpr int8_t PIN_TFT_DC       = 13;
    constexpr int8_t PIN_TFT_RST      = 14;

    // --- ПАРАМЕТРИ LED СТРІЧКИ ---
    constexpr uint16_t NUM_LEDS       = 60;
    constexpr uint8_t  MAX_SAFE_BRT   = 135;
    constexpr uint8_t  MIN_SAFE_BRT   = 14;

    // --- ЧАСОВИЙ ПОЯС (Київ) ---
    constexpr const char* TZ_INFO     = "EET-2EEST,M3.5.0/3,M10.5.0/4";
    constexpr const char* NTP_SERVER  = "pool.ntp.org";
}