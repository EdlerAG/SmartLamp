#pragma once
#include <Arduino.h>

namespace Config {
    constexpr const char* FW_VERSION  = "v4.5-Hallmark";
    constexpr const char* DEVICE_NAME = "magicbox";
    constexpr const char* AP_SSID     = "MagicBox-AP";

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
    constexpr const char* ORACLE_API_KEY    = "smartlamp-secret-key";
#endif

    // У v4.5 використовується миттєвий Long-Poll (сервер тримає запит до 12 с і відповідає за 15 мс!)
    constexpr uint32_t TG_POLL_MS          = 2500;
    constexpr uint32_t ORACLE_SYNC_IDLE_MS = 250;   // Пауза між Long-Poll сесіями
    constexpr uint32_t ORACLE_SYNC_MS      = 250;
    constexpr uint32_t ORACLE_LONGPOLL_SEC = 10;    // Час очікування миттєвої команди на сервері

    // Параметри анімаційного рушія PSRAM (8 MB OPI PSRAM)
    constexpr uint8_t  MBX_MAX_FRAMES      = 36;    // До 36 кадрів 240x240 RGB565
    constexpr size_t   FRAME_BYTES_RGB565  = 240 * 240 * 2; // 115200 байт

    // Параметри Spotify Vinyl Deck
    constexpr uint8_t  VINYL_DIAMETER      = 136;   // Діаметр круглої вінілової платівки
    constexpr size_t   VINYL_BYTES_RGB565  = 136 * 136 * 2; // 36992 байти
    constexpr const char* SAVER_FILE_PATH  = "/default_saver.mbx";

    // Калібрування датчиків у корпусі
    constexpr float TEMP_OFFSET_C = 0.0f;

    // Таймінги для відключень світла (Blackout) та Onboarding
    constexpr uint32_t WIFI_BOOT_TIMEOUT_MS    = 8000;
    constexpr uint32_t AP_PORTAL_TIMEOUT_MS    = 120000;
    constexpr uint32_t BLACKOUT_PROBE_EVERY_MS = 25000;
    constexpr uint32_t BOOT_SPLASH_MS          = 2800;  // Тривалість вітального екрана

    // Піни периферії
    constexpr uint8_t PIN_LDR         = 1;
    constexpr uint8_t PIN_TOUCH_LEFT  = 4;
    constexpr uint8_t PIN_LED         = 5;
    constexpr uint8_t PIN_TOUCH_TOP   = 6;
    constexpr uint8_t PIN_TOUCH_RIGHT = 7;
    constexpr uint8_t PIN_I2C_SDA     = 8;
    constexpr uint8_t PIN_I2C_SCL     = 9;

    // Піни дисплея GC9A01 (SPI 80 MHz DMA)
    constexpr int8_t PIN_TFT_CS       = 10;
    constexpr int8_t PIN_TFT_MOSI     = 11;
    constexpr int8_t PIN_TFT_SCLK     = 12;
    constexpr int8_t PIN_TFT_DC       = 13;
    constexpr int8_t PIN_TFT_RST      = 14;

    // Піни цифрового аудіо I2S (MAX98357A)
    constexpr int8_t  PIN_I2S_BCLK    = 15;
    constexpr int8_t  PIN_I2S_LRC     = 16;
    constexpr int8_t  PIN_I2S_DOUT    = 17;
    constexpr uint8_t DEFAULT_VOLUME  = 12; // 0..21 (комфортний рівень за замовчуванням v4.5)

    // Параметри LED стрічки SK6812 RGBW
    constexpr uint16_t NUM_LEDS       = 60;
    constexpr uint8_t  MAX_SAFE_BRT   = 145;
    constexpr uint8_t  MIN_SAFE_BRT   = 14;

    // Часовий пояс (Київ)
    constexpr const char* TZ_INFO     = "EET-2EEST,M3.5.0/3,M10.5.0/4";
    constexpr const char* NTP_SERVER  = "pool.ntp.org";
}
