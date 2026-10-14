#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Config.h"

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
    SCREEN_CLIMATE = 0,
    SCREEN_BAROMETER = 1,
    SCREEN_TELEGRAM = 2,
    SCREEN_NETWORK = 3,
    SCREEN_COUNT = 4
};

// Тип світлового спец-ефекту сповіщення
enum NotifyLedType : uint8_t {
    NOTIFY_NONE = 0,
    NOTIFY_CYAN_WAVE = 1, // Вхідне повідомлення
    NOTIFY_LOVE_PULSE = 2 // Рожево-червоний пульс сердечка
};

struct AppState {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    // Кліматичні дані
    volatile float temperature = 22.0f;
    volatile float humidity    = 50.0f;
    volatile float pressure    = 750.0f;
    volatile bool  sht_connected = false;
    volatile bool  bmp_connected = false;

    // Освітленість та яскравість
    volatile int     raw_ldr         = 2000;
    volatile float   smooth_ldr      = 2000.0f;
    volatile int     light_percent   = 50;
    volatile bool    auto_brt        = true;
    volatile uint8_t manual_brt_idx  = 1;
    volatile uint8_t current_led_brt = 50;
    volatile uint8_t current_lcd_brt = 200;

    // Режими екрана та підсвітки
    volatile uint8_t screen_mode = SCREEN_CLIMATE;
    volatile uint8_t light_mode  = 0;
    static constexpr uint8_t LIGHT_MODE_COUNT = 6;

    // Активність користувача (Adaptive FPS)
    volatile uint32_t last_user_activity_ms = 0;
    volatile bool     ui_force_dirty        = true;

    // Відкладене збереження в NVS
    volatile bool     settings_dirty     = false;
    volatile uint32_t settings_change_ms = 0;
    volatile bool     tg_config_dirty    = false; // FIX: volatile — пишеться з Core0/TG, читається з Core0/Net

    // Стан сенсорних кнопок
    volatile bool btn_l_pressed = false;
    volatile bool btn_t_pressed = false;
    volatile bool btn_r_pressed = false;

    // Мережа та режим блекауту
    volatile bool    wifi_connected      = false;
    volatile bool    ap_mode_active      = false;
    volatile uint8_t ap_clients          = 0;
    volatile bool    wifi_sleeping       = false; // BLACKOUT_ECO режим
    volatile int8_t  wifi_rssi           = -60;
    volatile bool    cmd_start_ap        = false;
    volatile bool    time_synced         = false;
    volatile bool    had_blackout_drop   = false; // Чи втрачався зв'язок через блекаут

    char wifi_ssid[33]  = "Connecting...";
    char ip_address[20] = "0.0.0.0";
    char time_str[10]   = "--:--";
    char date_str[16]   = "---";

    // OTA статус
    volatile bool    ota_in_progress = false;
    volatile uint8_t ota_progress    = 0;

    // Telegram двосторонній стан (Admin + Girl)
    char tg_token[64]    = "";
    char tg_admin_id[20] = ""; // Твій ID (Ярослав)
    char tg_girl_id[20]  = ""; // Її ID (Настя)
    char tg_sender[32]   = "Yaroslav";
    char tg_message[64]  = "Have a magical evening!";

    volatile bool    tg_unread              = true;
    volatile uint8_t led_notify_type        = NOTIFY_NONE;
    volatile bool    cmd_send_tg_ack        = false; // Вона прочитала повідомлення (торкнулася кришки)
    volatile bool    cmd_send_love_ping     = false; // Вона затиснула праву кнопку (надіслала сердечко)
    volatile bool    cmd_send_power_restore = false; // Сповіщення про відновлення світла/Wi-Fi

    QueueHandle_t eventQueue = nullptr;

    void initQueue() {
        eventQueue = xQueueCreate(16, sizeof(AppEvent));
        last_user_activity_ms = millis();
        strncpy(tg_token, Config::DEFAULT_TG_TOKEN, sizeof(tg_token) - 1);
        strncpy(tg_admin_id, Config::DEFAULT_ADMIN_ID, sizeof(tg_admin_id) - 1);
        strncpy(tg_girl_id, Config::DEFAULT_GIRL_ID, sizeof(tg_girl_id) - 1);
    }

    void registerActivity() {
        last_user_activity_ms = millis();
        ui_force_dirty = true;
    }

    void triggerLedNotification(NotifyLedType type) {
        led_notify_type = type;
        registerActivity();
    }

    void sendEvent(EventType type) {
        if (eventQueue) {
            AppEvent ev{type};
            xQueueSend(eventQueue, &ev, 0);
            registerActivity();
        }
    }

    void markSettingsDirty() {
        settings_dirty = true;
        settings_change_ms = millis();
        registerActivity();
    }

    void setTimeStrings(const char* t_in, const char* d_in) {
        portENTER_CRITICAL(&mux);
        strncpy(time_str, t_in, sizeof(time_str) - 1);
        time_str[sizeof(time_str) - 1] = '\0';
        strncpy(date_str, d_in, sizeof(date_str) - 1);
        date_str[sizeof(date_str) - 1] = '\0';
        time_synced = true;
        portEXIT_CRITICAL(&mux);
    }

    void getTimeString(char* t_out, size_t t_len) {
        portENTER_CRITICAL(&mux);
        strncpy(t_out, time_str, t_len - 1);
        t_out[t_len - 1] = '\0';
        portEXIT_CRITICAL(&mux);
    }

    void setNetworkInfo(const char* ssid, const char* ip) {
        portENTER_CRITICAL(&mux);
        strncpy(wifi_ssid, ssid, sizeof(wifi_ssid) - 1);
        wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
        strncpy(ip_address, ip, sizeof(ip_address) - 1);
        ip_address[sizeof(ip_address) - 1] = '\0';
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
    }

    void getNetworkInfo(char* ssid_out, size_t ssid_len, char* ip_out, size_t ip_len) {
        portENTER_CRITICAL(&mux);
        strncpy(ssid_out, wifi_ssid, ssid_len - 1);
        ssid_out[ssid_len - 1] = '\0';
        strncpy(ip_out, ip_address, ip_len - 1);
        ip_out[ip_len - 1] = '\0';
        portEXIT_CRITICAL(&mux);
    }

    void setTelegramConfig(const char* tok, const char* adm, const char* grl) {
        portENTER_CRITICAL(&mux);
        if (tok && strlen(tok) > 0) {
            strncpy(tg_token, tok, sizeof(tg_token) - 1);
            tg_token[sizeof(tg_token) - 1] = '\0';
        }
        if (adm) {
            strncpy(tg_admin_id, adm, sizeof(tg_admin_id) - 1);
            tg_admin_id[sizeof(tg_admin_id) - 1] = '\0';
        }
        if (grl) {
            strncpy(tg_girl_id, grl, sizeof(tg_girl_id) - 1);
            tg_girl_id[sizeof(tg_girl_id) - 1] = '\0';
        }
        portEXIT_CRITICAL(&mux);
    }

    void getTelegramConfig(char* tok_out, size_t t_len, char* adm_out, size_t a_len, char* grl_out, size_t g_len) {
        portENTER_CRITICAL(&mux);
        if (tok_out) { strncpy(tok_out, tg_token, t_len - 1); tok_out[t_len - 1] = '\0'; }
        if (adm_out) { strncpy(adm_out, tg_admin_id, a_len - 1); adm_out[a_len - 1] = '\0'; }
        if (grl_out) { strncpy(grl_out, tg_girl_id, g_len - 1); grl_out[g_len - 1] = '\0'; }
        portEXIT_CRITICAL(&mux);
    }

    void setTelegramData(const char* sender, const char* msg) {
        portENTER_CRITICAL(&mux);
        strncpy(tg_sender, sender, sizeof(tg_sender) - 1);
        tg_sender[sizeof(tg_sender) - 1] = '\0';
        strncpy(tg_message, msg, sizeof(tg_message) - 1);
        tg_message[sizeof(tg_message) - 1] = '\0';
        tg_unread = true;
        portEXIT_CRITICAL(&mux);
        registerActivity();
    }

    void getTelegramData(char* sender_out, size_t s_len, char* msg_out, size_t m_len) {
        portENTER_CRITICAL(&mux);
        strncpy(sender_out, tg_sender, s_len - 1);
        sender_out[s_len - 1] = '\0';
        strncpy(msg_out, tg_message, m_len - 1);
        msg_out[m_len - 1] = '\0';
        portEXIT_CRITICAL(&mux);
    }
};