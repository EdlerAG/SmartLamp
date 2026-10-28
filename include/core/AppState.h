#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Config.h"
#include "core/Types.h"

struct AppState {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    // Системні стани інтерфейсу (Рівень А)
    volatile SystemUiState sys_ui_state         = SystemUiState::SYS_BOOT_SPLASH;
    volatile uint32_t      boot_splash_start_ms = 0;
    volatile bool          onboarded            = false;
    volatile OnboardingStep onboarding_step     = OnboardingStep::WIZ_STEP_WELCOME_SOUND;
    volatile uint32_t      onboarding_ready_ms  = 0;
    volatile uint32_t      diag_opened_ms       = 0;

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
    volatile uint8_t current_lcd_brt = 220;

    // Таймер сну
    volatile uint32_t sleep_timer_end_ms   = 0;
    volatile uint32_t sleep_timer_total_ms = 0;
    volatile bool     sleep_timer_active   = false;

    // Режими екрана (Рівень Б: 4 основні вікна) та підсвітки
    volatile uint8_t screen_mode = SCREEN_CLIMATE;
    volatile uint8_t light_mode  = MODE_WARM_COZY;
    static constexpr uint8_t LIGHT_MODE_COUNT = TOTAL_LIGHT_MODES;

    // Активність користувача та мережі
    volatile uint32_t last_user_activity_ms = 0;
    volatile uint32_t last_net_activity_ms  = 0;
    volatile bool     ui_force_dirty        = true;

    // --- МУЛЬТИКАДРОВИЙ ПРОГРАВАЧ ГІФОК ТА НАЛІПОК У PSRAM (v4.5) ---
    uint8_t*          psram_anim_buf        = nullptr; // Буфер під активні стріми (до 36 кадрів)
    uint8_t*          psram_saver_buf       = nullptr; // Окремий буфер під постійну заставку LittleFS
    uint8_t*          shared_canvas_buf     = nullptr;
    volatile uint8_t  anim_frames_count     = 0;
    volatile uint8_t  anim_fps              = 15;
    volatile uint32_t custom_frame_until    = 0;
    volatile uint32_t media_expire_ms       = 0;       // 0: крутиться безперервно поки не скасують; >0: час завершення за таймером
    volatile bool     stop_media_requested  = false;
    volatile bool     canvas_loading        = false;

    // Постійна GIF-Заставка (Вікно 2)
    volatile uint32_t saver_crc             = 0;
    volatile uint8_t  saver_num_frames      = 0;
    volatile uint8_t  saver_fps             = 12;
    volatile size_t   saver_buf_size        = 0;
    volatile bool     saver_loaded          = false;
    volatile bool     saver_show_clock      = true;
    volatile bool     cmd_save_saver        = false;

    // Колір AmbiLight для синхронізації стрічки SK6812 з активною гіфкою або Spotify
    volatile uint8_t  ambi_r                = 56;
    volatile uint8_t  ambi_g                = 189;
    volatile uint8_t  ambi_b                = 248;
    volatile bool     ambi_active           = false;

    // Резервний оверлей реакції
    volatile uint8_t  screen_reaction_type  = NOTIFY_NONE;
    volatile uint32_t screen_reaction_until = 0;

    // --- SPOTIFY VINYL HUB (v4.5) ---
    SpotifyTrackInfo  spotify;
    volatile int8_t   cmd_spotify_action    = 0; // 0: none, 1: next, 2: prev, 3: play_pause
    volatile bool     spotify_vinyl_needed  = false;
    volatile bool     spotify_vinyl_loading = false;
    volatile bool     sp_linked             = false; // Офіційний Spotify акаунт прив'язано до сервера

    // --- ЗВУКОВА ПІДСИСТЕМА I2S ТА МУТЕ-КОНТРОЛЬ (v4.5) ---
    volatile uint8_t  audio_volume          = Config::DEFAULT_VOLUME; // 0..21
    volatile bool     audio_muted           = false; // Повне відключення звуку (100% тиша)
    volatile bool     touch_beep_enabled    = true;  // Тактильні кліки сенсорних кнопок
    volatile bool     audio_settings_dirty  = false;
    volatile bool     dnd_mode              = false; // Режим "Не турбувати" (DND)

    // --- ОСОБИСТИЙ КАЛЕНДАР НАСТІ ТА ТИХІ ГОДИНИ (v5.0) ---
    char              cal_summary[36]       = "";
    char              cal_time[10]          = "";
    volatile bool     cal_is_soon           = false;
    volatile bool     cal_active            = false;
    volatile bool     quiet_hours_active    = false;

    // --- POMODORO ФОКУС-ТАЙМЕР (v5.1) ---
    volatile bool     pomo_active           = false;
    volatile bool     pomo_paused           = false;
    volatile uint8_t  pomo_duration_min     = 25;  // 5..90 хв (налаштовується кнопками ◄/►)
    volatile uint32_t pomo_end_ms           = 0;
    volatile uint32_t pomo_paused_rem_ms    = 0;
    volatile bool     pomo_is_break         = false; // false: фокус, true: перерва

    // --- ПРИВІТАННЯ З ДНЕМ НАРОДЖЕННЯ (v5.1) ---
    volatile bool     birthday_greeted          = false;
    volatile bool     birthday_celebrate_active = false;
    volatile uint32_t birthday_celebrate_end_ms = 0;

    // Процедурні тригери синтезатора
    volatile bool     trig_touch_click      = false;
    volatile bool     trig_boot_chord       = false;
    volatile bool     trig_notify_bell      = false;
    volatile uint8_t  trig_reaction_synth   = 0; // 2..6

    // Відкладене збереження в NVS
    volatile bool     settings_dirty     = false;
    volatile uint32_t settings_change_ms = 0;
    volatile bool     tg_config_dirty    = false;
    volatile bool     cloud_config_dirty = false;

    // Стан сенсорних кнопок
    volatile bool btn_l_pressed = false;
    volatile bool btn_t_pressed = false;
    volatile bool btn_r_pressed = false;

    // Мережа, TLS-захист та Oracle Cloud
    volatile bool    wifi_connected      = false;
    volatile bool    ap_mode_active      = false;
    volatile uint8_t ap_clients          = 0;
    volatile bool    wifi_sleeping       = false;
    volatile int8_t  wifi_rssi           = -60;
    volatile bool    cmd_start_ap        = false;
    volatile bool    cmd_reboot          = false;
    volatile bool    cmd_clear_cache     = false;
    volatile bool    time_synced         = false;
    volatile bool    had_blackout_drop   = false;
    volatile bool    tls_in_flight       = false;
    volatile bool    oracle_online       = false;
    volatile bool    oracle_manages_bot  = false;
    volatile bool    has_more_cloud_cmds = false;
    volatile bool    audio_playing       = false;
    char             reset_reason[16]    = "UNKNOWN";
    volatile uint32_t last_cloud_sync_ms = 0;
    volatile uint16_t cloud_sync_ok_cnt  = 0;
    volatile uint16_t cloud_sync_err_cnt = 0;

    char wifi_ssid[33]  = "Підключення...";
    char ip_address[20] = "0.0.0.0";
    char time_str[10]   = "--:--";
    char date_str[16]   = "---";

    // Черги URL для OTA, Аудіо (I2S) та Анімацій (.mbx / .rgb565)
    volatile bool    ota_in_progress = false;
    volatile uint8_t ota_progress    = 0;

    char pending_ota_url[160]        = "";
    char pending_ota_sha256[65]      = "";
    char pending_audio_url[160]      = "";
    char pending_media_url[160]      = "";
    char pending_media_name[32]      = "";
    char pending_media_crc[16]       = "";
    volatile bool pending_media_is_saver = false;

    // Telegram, Oracle Cloud та Історія повідомлень (Inbox на 5 повідомлень)
    char tg_token[64]    = "";
    char tg_admin_id[20] = "";
    char tg_girl_id[20]  = "";
    char oracle_url[128] = "";
    char oracle_key[64]  = "";

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
        boot_splash_start_ms  = now;

        // Виділяємо буфери у 8 MB PSRAM з урахуванням заголовка MbxHeader
        if (psramFound()) {
            psram_anim_buf  = (uint8_t*)ps_malloc(sizeof(MbxHeader) + Config::MBX_MAX_FRAMES * Config::FRAME_BYTES_RGB565);
            psram_saver_buf = (uint8_t*)ps_malloc(sizeof(MbxHeader) + 18 * Config::FRAME_BYTES_RGB565); // До 18 кадрів постійної заставки (~2.0 MB)
        }

        strncpy(tg_token, Config::DEFAULT_TG_TOKEN, sizeof(tg_token) - 1);
        strncpy(tg_admin_id, Config::DEFAULT_ADMIN_ID, sizeof(tg_admin_id) - 1);
        strncpy(tg_girl_id, Config::DEFAULT_GIRL_ID, sizeof(tg_girl_id) - 1);
        strncpy(oracle_url, Config::ORACLE_SERVER_URL, sizeof(oracle_url) - 1);
        strncpy(oracle_key, Config::ORACLE_API_KEY, sizeof(oracle_key) - 1);

        strncpy(inbox[0].sender, "Ярослав", sizeof(inbox[0].sender) - 1);
        strncpy(inbox[0].text, "Шкатулка MagicBox v4.5 готова! Надішли наліпку чи гіфку ✨", sizeof(inbox[0].text) - 1);
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
        if (canvas_loading || anim_frames_count == 0 || stop_media_requested) return false;
        // Якщо встановлено таймер показу медіа — перевіряємо його закінчення
        if (media_expire_ms > 0 && (int32_t)(now - media_expire_ms) >= 0) return false;
        return true;
    }

    void activateCustomAnimation(uint8_t frames, uint8_t fps, uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms = 0) {
        portENTER_CRITICAL(&mux);
        anim_frames_count     = constrain(frames, (uint8_t)1, Config::MBX_MAX_FRAMES);
        anim_fps              = constrain(fps, (uint8_t)4, (uint8_t)30);
        ambi_r                = r;
        ambi_g                = g;
        ambi_b                = b;
        ambi_active           = true;
        stop_media_requested  = false;
        media_expire_ms       = (duration_ms > 0) ? (millis() + duration_ms) : 0;
        custom_frame_until    = media_expire_ms;
        canvas_loading        = false;
        portEXIT_CRITICAL(&mux);
        led_notify_type = NOTIFY_AMBILIGHT;
        registerActivity();
    }

    void stopActiveMedia() {
        portENTER_CRITICAL(&mux);
        anim_frames_count    = 0;
        custom_frame_until   = 0;
        media_expire_ms      = 0;
        stop_media_requested = true;
        ambi_active          = false;
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
        registerActivity();
    }

    void setMediaTimer(uint16_t minutes) {
        portENTER_CRITICAL(&mux);
        if (minutes == 0) {
            media_expire_ms = 0; // Постійно
        } else {
            media_expire_ms = millis() + ((uint32_t)minutes * 60000UL);
        }
        custom_frame_until = media_expire_ms;
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
        registerActivity();
    }

    void setVolume(uint8_t vol) {
        portENTER_CRITICAL(&mux);
        audio_volume = constrain(vol, (uint8_t)0, (uint8_t)21);
        if (audio_volume == 0) audio_muted = true;
        audio_settings_dirty = true;
        portEXIT_CRITICAL(&mux);
        registerActivity();
    }

    void setMute(bool muted) {
        portENTER_CRITICAL(&mux);
        audio_muted = muted;
        audio_settings_dirty = true;
        portEXIT_CRITICAL(&mux);
        registerActivity();
    }

    void setTouchBeep(bool enabled) {
        portENTER_CRITICAL(&mux);
        touch_beep_enabled = enabled;
        audio_settings_dirty = true;
        portEXIT_CRITICAL(&mux);
        registerActivity();
    }

    void setQuietHours(bool active) {
        if (quiet_hours_active != active) {
            quiet_hours_active = active;
            dnd_mode = active;
            ui_force_dirty = true;
        }
    }

    void setCalendarEvent(const char* summary, const char* time_val, bool is_soon) {
        portENTER_CRITICAL(&mux);
        if (summary) {
            strncpy(cal_summary, summary, sizeof(cal_summary) - 1);
            cal_summary[sizeof(cal_summary) - 1] = '\0';
        }
        if (time_val) {
            strncpy(cal_time, time_val, sizeof(cal_time) - 1);
            cal_time[sizeof(cal_time) - 1] = '\0';
        }
        cal_is_soon = is_soon;
        cal_active = (cal_summary[0] != '\0');
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
    }

    void clearCalendarEvent() {
        if (cal_active) {
            portENTER_CRITICAL(&mux);
            cal_summary[0] = '\0';
            cal_time[0] = '\0';
            cal_is_soon = false;
            cal_active = false;
            portEXIT_CRITICAL(&mux);
            ui_force_dirty = true;
        }
    }

    void pomoAdjustMinutes(int8_t delta) {
        portENTER_CRITICAL(&mux);
        if (!pomo_active) {
            int16_t new_m = (int16_t)pomo_duration_min + delta;
            pomo_duration_min = (uint8_t)constrain((int)new_m, 5, 90);
        } else {
            int32_t rem = (int32_t)(pomo_end_ms - millis()) + ((int32_t)delta * 60000L);
            if (rem < 30000) rem = 30000;
            pomo_end_ms = millis() + (uint32_t)rem;
        }
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
        registerActivity();
    }

    void pomoToggleStartPause() {
        portENTER_CRITICAL(&mux);
        uint32_t now = millis();
        if (!pomo_active) {
            pomo_active = true;
            pomo_paused = false;
            pomo_is_break = false;
            pomo_end_ms = now + ((uint32_t)pomo_duration_min * 60000L);
            light_mode = MODE_AMBER_EMBER;
        } else if (pomo_paused) {
            pomo_paused = false;
            pomo_end_ms = now + pomo_paused_rem_ms;
        } else {
            pomo_paused = true;
            pomo_paused_rem_ms = (pomo_end_ms > now) ? (pomo_end_ms - now) : 0;
        }
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
        registerActivity();
    }

    void pomoStop() {
        portENTER_CRITICAL(&mux);
        pomo_active = false;
        pomo_paused = false;
        pomo_is_break = false;
        pomo_end_ms = 0;
        pomo_paused_rem_ms = 0;
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
        registerActivity();
    }

    uint16_t getPomoRemainingSeconds() const {
        if (!pomo_active) return (uint16_t)pomo_duration_min * 60;
        if (pomo_paused) return (uint16_t)(pomo_paused_rem_ms / 1000);
        uint32_t now = millis();
        if (now >= pomo_end_ms) return 0;
        return (uint16_t)((pomo_end_ms - now) / 1000);
    }

    void updatePomodoroLifecycle(uint32_t now) {
        if (!pomo_active || pomo_paused) return;
        if (now >= pomo_end_ms) {
            portENTER_CRITICAL(&mux);
            if (!pomo_is_break) {
                pomo_is_break = true;
                pomo_end_ms = now + (5 * 60000UL);
                light_mode = MODE_OCEAN_BREEZE;
                trig_notify_bell = true;
            } else {
                pomo_active = false;
                pomo_is_break = false;
                pomo_end_ms = 0;
                light_mode = MODE_WARM_COZY;
                trig_boot_chord = true;
            }
            portEXIT_CRITICAL(&mux);
            ui_force_dirty = true;
            registerActivity();
        }
    }

    void triggerBirthdayCelebration(uint32_t duration_ms = 20000) {
        portENTER_CRITICAL(&mux);
        birthday_celebrate_active = true;
        birthday_celebrate_end_ms = millis() + duration_ms;
        sys_ui_state = SystemUiState::SYS_BIRTHDAY_CELEBRATE;
        birthday_greeted = true;
        portEXIT_CRITICAL(&mux);
        ui_force_dirty = true;
        registerActivity();
    }

    void triggerLedNotification(NotifyLedType type, bool show_screen_overlay = false) {
        led_notify_type = type;
        if (show_screen_overlay && type >= NOTIFY_FIRE && type <= NOTIFY_CROWN) {
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

    uint16_t getMediaRemainingMin() const {
        if (media_expire_ms == 0 || anim_frames_count == 0) return 0;
        int32_t rem = (int32_t)(media_expire_ms - millis());
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

    void setPendingOtaUrl(const char* url, const char* sha256 = "") {
        portENTER_CRITICAL(&mux);
        strncpy(pending_ota_url, url ? url : "", sizeof(pending_ota_url) - 1);
        pending_ota_url[sizeof(pending_ota_url) - 1] = '\0';
        strncpy(pending_ota_sha256, sha256 ? sha256 : "", sizeof(pending_ota_sha256) - 1);
        pending_ota_sha256[sizeof(pending_ota_sha256) - 1] = '\0';
        portEXIT_CRITICAL(&mux);
    }

    bool popPendingOtaUrl(char* url_out, size_t max_len, char* sha_out = nullptr, size_t sha_len = 0) {
        bool has = false;
        portENTER_CRITICAL(&mux);
        if (strlen(pending_ota_url) > 10) {
            strncpy(url_out, pending_ota_url, max_len - 1);
            url_out[max_len - 1] = '\0';
            if (sha_out && sha_len > 0) {
                strncpy(sha_out, pending_ota_sha256, sha_len - 1);
                sha_out[sha_len - 1] = '\0';
            }
            pending_ota_url[0] = '\0';
            pending_ota_sha256[0] = '\0';
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

    void setPendingMedia(const char* url, const char* name = "", const char* crc = "", bool is_saver = false) {
        portENTER_CRITICAL(&mux);
        strncpy(pending_media_url, url, sizeof(pending_media_url) - 1);
        pending_media_url[sizeof(pending_media_url) - 1] = '\0';
        strncpy(pending_media_name, name ? name : "", sizeof(pending_media_name) - 1);
        pending_media_name[sizeof(pending_media_name) - 1] = '\0';
        strncpy(pending_media_crc, crc ? crc : "", sizeof(pending_media_crc) - 1);
        pending_media_crc[sizeof(pending_media_crc) - 1] = '\0';
        pending_media_is_saver = is_saver;
        portEXIT_CRITICAL(&mux);
        registerNetActivity();
    }

    void setPendingImageUrl(const char* url) {
        setPendingMedia(url);
    }

    bool popPendingImageUrl(char* url_out, size_t max_len) {
        char n[32], c[16];
        bool is_sav = false;
        return popPendingMedia(url_out, max_len, n, sizeof(n), c, sizeof(c), is_sav);
    }

    bool popPendingMedia(char* url_out, size_t u_len, char* name_out, size_t n_len, char* crc_out, size_t c_len, bool& is_saver_out) {
        bool has = false;
        portENTER_CRITICAL(&mux);
        if (strlen(pending_media_url) > 8) {
            strncpy(url_out, pending_media_url, u_len - 1); url_out[u_len - 1] = '\0';
            strncpy(name_out, pending_media_name, n_len - 1); name_out[n_len - 1] = '\0';
            strncpy(crc_out, pending_media_crc, c_len - 1); crc_out[c_len - 1] = '\0';
            is_saver_out = pending_media_is_saver;
            pending_media_url[0] = '\0';
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

    void setOracleConfig(const char* url, const char* key) {
        portENTER_CRITICAL(&mux);
        if (url && strlen(url) > 0) {
            strncpy(oracle_url, url, sizeof(oracle_url) - 1);
            oracle_url[sizeof(oracle_url) - 1] = '\0';
        }
        if (key && strlen(key) > 0) {
            strncpy(oracle_key, key, sizeof(oracle_key) - 1);
            oracle_key[sizeof(oracle_key) - 1] = '\0';
        }
        portEXIT_CRITICAL(&mux);
    }

    void getOracleConfig(char* url_out, size_t u_len, char* key_out, size_t k_len) const {
        portENTER_CRITICAL(const_cast<portMUX_TYPE*>(&mux));
        if (url_out && u_len > 0) {
            strncpy(url_out, oracle_url, u_len - 1);
            url_out[u_len - 1] = '\0';
        }
        if (key_out && k_len > 0) {
            strncpy(key_out, oracle_key, k_len - 1);
            key_out[k_len - 1] = '\0';
        }
        portEXIT_CRITICAL(const_cast<portMUX_TYPE*>(&mux));
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

    void setDnd(bool active) {
        portENTER_CRITICAL(&mux);
        dnd_mode = active;
        audio_muted = active;
        ui_force_dirty = true;
        portEXIT_CRITICAL(&mux);
        registerActivity();
    }
};
