#pragma once
#include <Arduino.h>

namespace Config {
    constexpr const char* DEVICE_NAME = "magicbox";
    constexpr const char* AP_SSID     = "MagicBox-Setup";

    // Піни периферії
    constexpr uint8_t PIN_LDR         = 1;  // ADC1_CH0 (фоторезистор)
    constexpr uint8_t PIN_TOUCH_LEFT  = 4;  // Ліва кнопка
    constexpr uint8_t PIN_LED         = 5;  // Стрічка SK6812 DIN
    constexpr uint8_t PIN_TOUCH_TOP   = 6;  // Кнопка на кришці
    constexpr uint8_t PIN_TOUCH_RIGHT = 7;  // Права кнопка
    constexpr uint8_t PIN_I2C_SDA     = 8;  // SHT31 + BMP280 SDA
    constexpr uint8_t PIN_I2C_SCL     = 9;  // SHT31 + BMP280 SCL

    // Піни дисплея GC9A01 (SPI)
    constexpr int8_t PIN_TFT_CS       = 10;
    constexpr int8_t PIN_TFT_MOSI     = 11;
    constexpr int8_t PIN_TFT_SCLK     = 12;
    constexpr int8_t PIN_TFT_DC       = 13;
    constexpr int8_t PIN_TFT_RST      = 14;

    // Параметри LED стрічки
    constexpr uint16_t NUM_LEDS       = 60;
    constexpr uint8_t  MAX_SAFE_BRT   = 130; // Збільшимо до 220 після підключення MH-CD42
    constexpr uint8_t  MIN_SAFE_BRT   = 15;

    // Часовий пояс (Київ)
    constexpr const char* TZ_INFO     = "EET-2EEST,M3.5.0/3,M10.5.0/4";
    constexpr const char* NTP_SERVER  = "pool.ntp.org";
}