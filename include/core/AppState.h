#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Config.h"
#include "core/Types.h"

struct AppState {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    // Кліматичні дані + тренд барометра
    volatile float temperature   = 22.0f;
    volatile float humidity      = 50.0f;
    volatile float pressure      = 750.0f;
    volatile float pressure_diff = 0.0f;
    volatile bool  sht_connected = false;
    volatile bool  bmp_connected = false;

    float   pressure_hist[24]   = {0};
    uint8_t pressure_hist_count = 0;

    // Освітленість та яскравість
    volatile int     raw_ldr         = 2000;
    volatile float   smooth_ldr      = 2000.0f;
    volatile int     light_percent   = 50;
    volatile bool    auto_brt        = true;
    volatile uint8_t manual_brt_idx  = 2;
    volatile uint8_t current_led_brt = 65;
    volatile uint8_t current_lcd_brt = 200;

    // Таймер сну
    volatile uint32_t sleep_timer_end_ms   = 0;
    volatile uint32_t sleep_timer_total_ms = 0;
    volatile bool     sleep_timer_active   = false;

    // Режими екрана та підсвітки
    volatile uint8_t screen_mode = SCREEN_CLIMATE;
    volatile uint8_t light_mode  = MODE_WARM_COZY;
    static constexpr uint8_t LIGHT_MODE_COUNT = TOTAL_LIGHT_MODES;

    // Активність користувача та мережі
    volatile uint32_t last_user_activity_ms = 0;
    volatile uint32_t last_net_activity_ms  = 0;
    volatile bool     ui_force_dirty        = true;

    // Повноекранний оверлей мем-реакції або кастомної наліпки/фото з сервера
    volatile uint8_t  screen_reaction_type  = NOTIFY_NONE;
    volatile uint32_t screen_reaction_until = 0;
    volatile uint32_t custom_frame_until    = 0;
    uint8_t*          shared_canvas_buf     = nullptr; // Вказує на буфер 240x240 RGB565 (0 байт зайвої RAM!)

    // Відкладене збереження в NVS
    volatile bool     settings_dirty     = false;
    volatile uint32_t settings_change_ms = 0;
    volatile bool     tg_config_dirty    = false;

    // Стан сенсорних кнопок
    volatile bool btn_l_pressed = false;
    volatile bool btn_t_pressed = false;
    volatile bool btn_r_pressed = false;

    // Мережа, TLS-захист та Oracle Cloud
    volatile bool    wifi_connected     = false;
    volatile bool    ap_mode_active     = false;
    volatile uint8_t ap_clients         = 0;
    volatile bool    wifi_sleeping      = false;
    volatile int8_t  wifi_rssi          = -60;
    volatile bool    cmd_start_ap       = false;
    volatile bool    time_synced        = false;
    volatile bool    had_blackout_drop  = false;
    volatile bool    tls_in_flight      = false;
    volatile bool    oracle_online      = false;
    volatile bool    oracle_manages_bot = false;

    char wifi_ssid[33]  = "Connecting...";
    char ip_address[20] = "0.0.0.0";
    char time_str[10]   = "--:--";
    char date_str[16]   = "---";

    // Черги URL для OTA, Аудіо (I2S) та Картинок (RGB565)
    volatile bool    ota_in_progress = false;
    volatile uint8_t ota_progress    = 0;
    volatile uint8_t audio_volume    = Config::DEFAULT_VOLUME;
    char pending_ota_url[160]        = "";
    char pending_audio_url[160]      = "";
    char pending_img_url[160]        = "";

    // Telegram та Історія повідомлень (Inbox на 5 повідомлень)
    char tg_token[64]    = "";
    char tg_admin_id[20] = "";
    char tg_girl_id[20]  = "";

    static constexpr uint8_t INBOX_CAPACITY = 5;
    InboxMessage inbox[INBOX_CAPACITY];
    volatile uint8_t inbox_count    = 1;
    volatile uint8_t inbox_view_idx = 0;

    volatile bool    tg_unread              = false;
    volatile uint8_t led_notify_type        = NOTIFY_NONE;
    volatile bool    cmd_send_tg_ack        = false;
    volatile uint8_t cmd_send_meme_ping     = 0;
    volatile bool    cmd_send_power_restore = false;

    QueueHandle_t eventQueue = nullptr;

    void initQueue() {
        eventQueue = xQueueCreate(16, sizeof(AppEvent));
        uint32_t now = millis();
        last_user_activity_ms = now;
        last_net_activity_ms  = now;

        strncpy(tg_token, Config::DEFAULT_TG_TOKEN, sizeof(tg_token) - 1);
        strncpy(tg_admin_id, Config::DEFAULT_ADMIN_ID, sizeof(tg_admin_id) - 1);
        strncpy(tg_girl_id, Config::DEFAULT_GIRL_ID, sizeof(tg_girl_id) - 1);

        strncpy(inbox[0].sender, "Yaroslav", sizeof(inbox[0].sender) - 1);
        strncpy(inbox[0].text, "MagicBox v4.5 Ready!", sizeof(inbox[0].text) - 1);
        strncpy(inbox[0].time_received, "--:--", sizeof(inbox[0].time_received) - 1);
        for (int i = 0; i < 24; i++) pressure_hist[i] = 750.0f;
    }

    void registerActivity() {
        last_user_activity_ms = millis();
        ui_force_dirty = true;
    }

    void registerNetActivity() {
        last_net_activity_ms = millis();
    }

    bool isReactionActive(uint32_t now) const {
        return (screen_reaction_type >= NOTIFY_FIRE) &&
               ((int32_t)(screen_reaction_until - now) > 0);
    }

    bool isCustomFrameActive(uint32_t now) const {
        return ((int32_t)(custom_frame_until - now) > 0);
    }

    void triggerLedNotification(NotifyLedType type, bool show_screen_overlay = false) {
        led_notify_type = type;
        if (show_screen_overlay && type >= NOTIFY_FIRE) {
            screen_reaction_type  = type;
            screen_reaction_until = millis() + 4500;
        }
        registerActivity();
    }

    uint8_t getLedBrightnessPercent() const {
        if (light_mode == MODE_LIGHT_OFF) return 0;
        int pct = ((int)current_led_brt * 100) / Config::MAX_SAFE_BRT;
        return (uint8_t)constrain(pct, 1, 100);
    }

    uint16_t getSleepTimerRemainingMin() const {
        if (!sleep_timer_active) return 0;
        int32_t rem = (int32_t)(sleep_timer_end_ms - millis());
        if (rem <= 0) return 0;
        return (uint16_t)((rem + 59999) / 60000);
    }

    void addPressureSample(float p) {
        portENTER_CRITICAL(&mux);
        if (pressure_hist_count == 0) {
            for (int i = 0; i < 24; i++) pressure_hist[i] = p;
            pressure_hist_count = 1;
        } else {
            for (int i = 0; i < 23; i++) pressure_hist[i] = pressure_hist[i + 1];
            pressure_hist[23] = p;
            if (pressure_hist_count < 24) pressure_hist_count++;
        }
        pressure_diff = p - pressure_hist[0];
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
    }

    void getPressureHistory(float* out24, uint8_t& count_out, float& diff_out) {
        portENTER_CRITICAL(&mux);
        count_out = pressure_hist_count;
        diff_out  = pressure_diff;
        for (int i = 0; i < 24; i++) out24[i] = pressure_hist[i];
        portEXIT_CRITICAL(&mux);
    }

    void sendEvent(EventType type) {
        if (eventQueue) {
            AppEvent ev{type};
            xQueueSend(eventQueue, &ev, 0);
            custom_frame_until = 0; // Будь-який клік кнопки закриває повноекранну картинку
            registerActivity();
        }
    }

    void markSettingsDirty() {
        settings_dirty = true;
        settings_change_ms = millis();
        registerActivity();
    }

    void setTimeStrings(const char* t_in, const char* d_in) {
        bool minute_changed = false;
        portENTER_CRITICAL(&mux);
        if (strncmp(time_str, t_in, sizeof(time_str) - 1) != 0) {
            strncpy(time_str, t_in, sizeof(time_str) - 1);
            time_str[sizeof(time_str) - 1] = '\0';
            minute_changed = true;
        }
        strncpy(date_str, d_in, sizeof(date_str) - 1);
        date_str[sizeof(date_str) - 1] = '\0';
        time_synced = true;
        portEXIT_CRITICAL(&mux);

        if (minute_changed) ui_force_dirty = true;
    }

    void getTimeString(char* t_out, size_t t_len) {
        portENTER_CRITICAL(&mux);
        strncpy(t_out, time_str, t_len - 1);
        t_out[t_len - 1] = '\0';
        portEXIT_CRITICAL(&mux);
    }

    void setPendingOtaUrl(const char* url) {
        portENTER_CRITICAL(&mux);
        strncpy(pending_ota_url, url, sizeof(pending_ota_url) - 1);
        pending_ota_url[sizeof(pending_ota_url) - 1] = '\0';
        portEXIT_CRITICAL(&mux);
    }

    bool popPendingOtaUrl(char* url_out, size_t max_len) {
        bool has = false;
        portENTER_CRITICAL(&mux);
        if (strlen(pending_ota_url) > 10) {
            strncpy(url_out, pending_ota_url, max_len - 1);
            url_out[max_len - 1] = '\0';
            pending_ota_url[0] = '\0';
            has = true;
        }
        portEXIT_CRITICAL(&mux);
        return has;
    }

    void setPendingAudioUrl(const char* url) {
        portENTER_CRITICAL(&mux);
        strncpy(pending_audio_url, url, sizeof(pending_audio_url) - 1);
        pending_audio_url[sizeof(pending_audio_url) - 1] = '\0';
        portEXIT_CRITICAL(&mux);
        registerNetActivity();
    }

    bool popPendingAudioUrl(char* url_out, size_t max_len) {
        bool has = false;
        portENTER_CRITICAL(&mux);
        if (strlen(pending_audio_url) > 8) {
            strncpy(url_out, pending_audio_url, max_len - 1);
            url_out[max_len - 1] = '\0';
            pending_audio_url[0] = '\0';
            has = true;
        }
        portEXIT_CRITICAL(&mux);
        return has;
    }

    void setPendingImageUrl(const char* url) {
        portENTER_CRITICAL(&mux);
        strncpy(pending_img_url, url, sizeof(pending_img_url) - 1);
        pending_img_url[sizeof(pending_img_url) - 1] = '\0';
        portEXIT_CRITICAL(&mux);
        registerNetActivity();
    }

    bool popPendingImageUrl(char* url_out, size_t max_len) {
        bool has = false;
        portENTER_CRITICAL(&mux);
        if (strlen(pending_img_url) > 8) {
            strncpy(url_out, pending_img_url, max_len - 1);
            url_out[max_len - 1] = '\0';
            pending_img_url[0] = '\0';
            has = true;
        }
        portEXIT_CRITICAL(&mux);
        return has;
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
        for (int i = INBOX_CAPACITY - 1; i > 0; i--) {
            inbox[i] = inbox[i - 1];
        }
        strncpy(inbox[0].sender, sender, sizeof(inbox[0].sender) - 1);
        inbox[0].sender[sizeof(inbox[0].sender) - 1] = '\0';
        strncpy(inbox[0].text, msg, sizeof(inbox[0].text) - 1);
        inbox[0].text[sizeof(inbox[0].text) - 1] = '\0';
        strncpy(inbox[0].time_received, time_str, sizeof(inbox[0].time_received) - 1);
        inbox[0].time_received[sizeof(inbox[0].time_received) - 1] = '\0';

        if (inbox_count < INBOX_CAPACITY) inbox_count++;
        inbox_view_idx = 0;
        tg_unread = true;
        portEXIT_CRITICAL(&mux);
        registerActivity();
    }

    void getTelegramData(char* sender_out, size_t s_len, char* msg_out, size_t m_len, char* time_out = nullptr, size_t t_len = 0) {
        portENTER_CRITICAL(&mux);
        uint8_t idx = (inbox_view_idx < inbox_count) ? inbox_view_idx : 0;
        strncpy(sender_out, inbox[idx].sender, s_len - 1);
        sender_out[s_len - 1] = '\0';
        strncpy(msg_out, inbox[idx].text, m_len - 1);
        msg_out[m_len - 1] = '\0';
        if (time_out && t_len > 0) {
            strncpy(time_out, inbox[idx].time_received, t_len - 1);
            time_out[t_len - 1] = '\0';
        }
        portEXIT_CRITICAL(&mux);
    }
};