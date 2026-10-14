#pragma once
#include <Arduino.h>

namespace Config {
    constexpr const char* DEVICE_NAME = "magicbox";
    constexpr const char* AP_SSID     = "MagicBox-Setup";

    // --- НАЛАШТУВАННЯ TELEGRAM ---
#if __has_include("Secrets.h")
    #include "Secrets.h"
    constexpr const char* DEFAULT_TG_TOKEN = Secrets::TG_TOKEN;
    constexpr const char* DEFAULT_ADMIN_ID = Secrets::ADMIN_ID;
    constexpr const char* DEFAULT_GIRL_ID  = Secrets::GIRL_ID;
#else
    constexpr const char* DEFAULT_TG_TOKEN = "";
    constexpr const char* DEFAULT_ADMIN_ID = "";
    constexpr const char* DEFAULT_GIRL_ID  = "";
#endif
    constexpr uint32_t TG_POLL_MS = 2500;

    // --- КАЛІБРУВАННЯ ДАТЧИКІВ У КОРПУСІ ---
    // Якщо всередині дерев'яної шкатулки датчик трохи гріється від плати, можна виставити -1.2f
    constexpr float TEMP_OFFSET_C = 0.0f;

    // --- ТАЙМІНГИ ДЛЯ ВІДКЛЮЧЕНЬ СВІТЛА (BLACKOUT) ---
    constexpr uint32_t WIFI_BOOT_TIMEOUT_MS    = 8000;
    constexpr uint32_t AP_PORTAL_TIMEOUT_MS    = 120000;
    constexpr uint32_t BLACKOUT_PROBE_EVERY_MS = 30000;

    // --- ПІНИ ПЕРИФЕРІЇ ---
    constexpr uint8_t PIN_LDR         = 1;  // ADC1_CH0 (фоторезистор)
    constexpr uint8_t PIN_TOUCH_LEFT  = 4;  // Ліва кнопка (Екрани / AP)
    constexpr uint8_t PIN_LED         = 5;  // Стрічка SK6812 DIN
    constexpr uint8_t PIN_TOUCH_TOP   = 6;  // Кришка (Світло / Прочитано)
    constexpr uint8_t PIN_TOUCH_RIGHT = 7;  // Права кнопка (Яскравість або гортання Inbox / Мем-реакція)
    constexpr uint8_t PIN_I2C_SDA     = 8;  // SHT31 + BMP280 SDA
    constexpr uint8_t PIN_I2C_SCL     = 9;  // SHT31 + BMP280 SCL

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