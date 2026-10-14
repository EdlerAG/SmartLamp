#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class EventType : uint8_t {
    BTN_LEFT_SHORT,
    BTN_LEFT_LONG,
    BTN_TOP_SHORT,
    BTN_TOP_LONG,
    BTN_RIGHT_SHORT,
    BTN_RIGHT_LONG,
    NEW_TELEGRAM_MSG,
    WIFI_CONNECTED      // WiFi щойно підключився → можна реагувати (LED-нотифікація тощо)
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

struct AppState {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    // Кліматичні показники
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

    // Стан сенсорних кнопок
    volatile bool btn_l_pressed = false;
    volatile bool btn_t_pressed = false;
    volatile bool btn_r_pressed = false;

    // Мережа
    volatile bool    wifi_connected = false;
    volatile bool    ap_mode_active = false;
    volatile uint8_t ap_clients     = 0;
    volatile bool    wifi_sleeping  = false;
    volatile int8_t  wifi_rssi      = -60;
    volatile bool    cmd_start_ap   = false;
    volatile bool    time_synced    = false;

    char wifi_ssid[33]  = "Connecting...";
    char ip_address[20] = "0.0.0.0";
    char time_str[10]   = "--:--";
    char date_str[16]   = "---";

    // OTA статус
    volatile bool    ota_in_progress = false;
    volatile uint8_t ota_progress    = 0;

    // Повідомлення
    char tg_sender[32]  = "Anastasiia";
    char tg_message[64] = "Have a magical evening!";
    volatile bool tg_unread          = true;
    volatile bool led_notify_trigger = false;

    QueueHandle_t eventQueue = nullptr;

    void initQueue() {
        eventQueue = xQueueCreate(16, sizeof(AppEvent));
        last_user_activity_ms = millis();
    }

    void registerActivity() {
        last_user_activity_ms = millis();
        ui_force_dirty = true;
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