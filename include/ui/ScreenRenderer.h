#pragma once
#include <LovyanGFX.hpp>
#include "core/AppState.h"
#include "core/CommandRouter.h"

/**
 * ============================================================================
 * ScreenRenderer (v4.5 Hallmark Horology, Interactive Wizard & Living Screensavers)
 * Inspired by Dieter Rams Braun horology and Hallmark Atelier design standards.
 * Pure tokenized RGB565 palette, zero acidic gradients, surgical typography.
 * ============================================================================
 */

namespace HallmarkTheme {
    constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
    }

    constexpr uint16_t BG_PRIMARY      = rgb(8, 9, 12);      // #08090C Deep Obsidian
    constexpr uint16_t BG_SURFACE      = rgb(18, 21, 30);    // #12151E Elevated Graphite
    constexpr uint16_t BORDER_HAIRLINE = rgb(34, 40, 52);    // #222834 Delicate Wire
    constexpr uint16_t TEXT_PRIMARY    = rgb(244, 243, 239); // #F4F3EF Warm Alabaster
    constexpr uint16_t TEXT_SECONDARY  = rgb(138, 148, 166); // #8A94A6 Muted Ash
    constexpr uint16_t ACCENT_WARM     = rgb(224, 122, 95);  // #E07A5F Braun Terracotta
    constexpr uint16_t ACCENT_COOL     = rgb(78, 168, 222);  // #4EA8DE Calm Indigo / Sky
    constexpr uint16_t ACCENT_SUCCESS  = rgb(82, 183, 136);  // #52B788 Sage Green / Emerald
    constexpr uint16_t ACCENT_DANGER   = rgb(230, 57, 70);   // #E63946 Matte Coral
    constexpr uint16_t ACCENT_AMBER    = rgb(245, 158, 11);  // #F59E0B Warm Amber
}

namespace ScreenRenderer {

    inline void splitUtf8Lines(const char* text, char* line1, size_t max1, char* line2, size_t max2, size_t max_chars_per_line = 19) {
        line1[0] = '\0';
        line2[0] = '\0';
        size_t byte_idx = 0;
        size_t char_cnt = 0;
        size_t last_space_byte = 0;

        while (text[byte_idx] != '\0') {
            uint8_t c = (uint8_t)text[byte_idx];
            if (c == ' ') last_space_byte = byte_idx;
            size_t step = (c < 0x80) ? 1 : ((c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4));
            if (char_cnt >= max_chars_per_line) break;
            byte_idx += step;
            char_cnt++;
        }

        if (text[byte_idx] == '\0') {
            strncpy(line1, text, max1 - 1);
            line1[max1 - 1] = '\0';
            return;
        }

        size_t split_at = (last_space_byte > 5) ? last_space_byte : byte_idx;
        if (split_at >= max1) split_at = max1 - 1;
        memcpy(line1, text, split_at);
        line1[split_at] = '\0';

        const char* rest = text + split_at;
        while (*rest == ' ') rest++;
        strncpy(line2, rest, max2 - 1);
        line2[max2 - 1] = '\0';
    }

    inline void drawPerimeterTicks(LGFX_Sprite& canvas, int highlight_sec = -1) {
        for (int i = 0; i < 60; ++i) {
            float rad = (float)i * (6.0f * 0.0174532925f);
            float cos_v = cosf(rad);
            float sin_v = sinf(rad);
            int r_in = (i % 5 == 0) ? 112 : 115;
            uint16_t col = (i == highlight_sec) ? HallmarkTheme::ACCENT_WARM :
                           ((i % 5 == 0) ? HallmarkTheme::TEXT_SECONDARY : HallmarkTheme::BORDER_HAIRLINE);
            canvas.drawLine(120 + (int)(cos_v * r_in), 120 + (int)(sin_v * r_in),
                            120 + (int)(cos_v * 118),  120 + (int)(sin_v * 118), col);
            if (i == highlight_sec) {
                canvas.fillCircle(120 + (int)(cos_v * 113), 120 + (int)(sin_v * 113), 2, HallmarkTheme::ACCENT_WARM);
            }
        }
    }

    // ========================================================================
    // РІВЕНЬ А: СИСТЕМНІ ВІКНА (ПОЗА ОСНОВНИМ ЦИКЛОМ)
    // ========================================================================

    // 1. SYS_BOOT_SPLASH (Швейцарський хронограф, тривалість 2.8 с при 60 FPS)
    inline void drawBootSplash(LGFX_Sprite& canvas, uint32_t elapsed_ms, uint32_t total_ms) {
        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);

        float progress = constrain((float)elapsed_ms / (float)total_ms, 0.0f, 1.0f);
        int active_tick = (int)(progress * 60.0f);

        for (int i = 0; i < 60; ++i) {
            float rad = (float)i * (6.0f * 0.0174532925f);
            float cos_v = cosf(rad);
            float sin_v = sinf(rad);
            int r_in = (i % 5 == 0) ? 111 : 114;
            uint16_t col = (i <= active_tick) ? HallmarkTheme::ACCENT_WARM : HallmarkTheme::BORDER_HAIRLINE;
            canvas.drawLine(120 + (int)(cos_v * r_in), 120 + (int)(sin_v * r_in),
                            120 + (int)(cos_v * 118),  120 + (int)(sin_v * 118), col);
        }

        canvas.drawArc(120, 120, 88, 86, 0, 360, HallmarkTheme::BORDER_HAIRLINE);

        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString("MAGICBOX", 120, 92);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        canvas.drawString("HALLMARK v4.5", 120, 116);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("I2C · I2S · PSRAM · OK", 120, 144);

        canvas.setTextColor(HallmarkTheme::BORDER_HAIRLINE);
        canvas.drawString("PRECISION ATELIER", 120, 172);
    }

    // 2. SYS_ONBOARDING_WIZARD (Інтерактивний 6-кроковий майстер v4.5)
    inline void drawOnboardingWizard(
        LGFX_Sprite& canvas,
        LGFX_Sprite& qr_cache,
        char* cached_qr_str,
        size_t qr_len,
        AppState& state
    ) {
        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);
        drawPerimeterTicks(canvas);

        OnboardingStep step = state.onboarding_step;

        switch (step) {
            // Крок 1: Вибір профілю звуку (Тиша vs Зі звуком)
            case OnboardingStep::WIZ_STEP_WELCOME_SOUND: {
                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
                canvas.drawString("WELCOME TO MAGICBOX", 120, 30);

                // Іконка динаміка в центрі
                canvas.drawCircle(120, 86, 32, HallmarkTheme::BORDER_HAIRLINE);
                canvas.fillCircle(120, 86, 26, HallmarkTheme::BG_SURFACE);
                canvas.drawCircle(120, 86, 26, HallmarkTheme::ACCENT_COOL);

                canvas.setFont(&fonts::FreeSansBold12pt7b);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("SOUND?", 120, 86);

                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("Увімкнути звук колонки?", 120, 130);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("I2S DAC · MAX98357A · Hi-Fi", 120, 150);

                // Дві капсули-підказки над кнопками
                canvas.fillRoundRect(22, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(22, 186, 88, 26, 13, HallmarkTheme::ACCENT_DANGER);
                canvas.setTextColor(HallmarkTheme::ACCENT_DANGER);
                canvas.drawString("◄ ТИША", 66, 199);

                canvas.fillRoundRect(130, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(130, 186, 88, 26, 13, HallmarkTheme::ACCENT_SUCCESS);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("ЗВУК ►", 174, 199);
                break;
            }

            // Крок 2: Підключення до AP лампи
            case OnboardingStep::WIZ_STEP_WIFI_AP: {
                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
                canvas.drawString("КРОК 1/3 · WI-FI ШКАТУЛКИ", 120, 24);

                const char* qr_payload = "WIFI:S:MagicBox-AP;T:nopass;;";
                if (strcmp(qr_payload, cached_qr_str) != 0) {
                    strncpy(cached_qr_str, qr_payload, qr_len - 1);
                    cached_qr_str[qr_len - 1] = '\0';
                    qr_cache.fillScreen(TFT_WHITE);
                    qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
                }

                canvas.fillRoundRect(70, 42, 100, 100, 8, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(70, 42, 100, 100, 8, HallmarkTheme::BORDER_HAIRLINE);
                qr_cache.pushSprite(&canvas, 70, 42);

                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("1. Скануй QR камерою", 120, 154);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("SSID: MagicBox-AP (Open)", 120, 172);

                // Кнопка далі
                canvas.fillRoundRect(136, 188, 80, 24, 12, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(136, 188, 80, 24, 12, HallmarkTheme::ACCENT_COOL);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("ДАЛІ ►", 176, 200);
                break;
            }

            // Крок 3: Портал вибору домашньої мережі
            case OnboardingStep::WIZ_STEP_WIFI_PORTAL: {
                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("КРОК 2/3 · ДОМАШНІЙ WI-FI", 120, 24);

                const char* qr_payload = "http://192.168.4.1/";
                if (strcmp(qr_payload, cached_qr_str) != 0) {
                    strncpy(cached_qr_str, qr_payload, qr_len - 1);
                    cached_qr_str[qr_len - 1] = '\0';
                    qr_cache.fillScreen(TFT_WHITE);
                    qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
                }

                canvas.fillRoundRect(70, 42, 100, 100, 8, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(70, 42, 100, 100, 8, HallmarkTheme::BORDER_HAIRLINE);
                qr_cache.pushSprite(&canvas, 70, 42);

                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("2. Обери мережу в браузері", 120, 154);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("http://192.168.4.1", 120, 172);

                canvas.fillRoundRect(24, 188, 80, 24, 12, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(24, 188, 80, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("◄ НАЗАД", 64, 200);

                canvas.fillRoundRect(136, 188, 80, 24, 12, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(136, 188, 80, 24, 12, HallmarkTheme::ACCENT_COOL);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("ДАЛІ ►", 176, 200);
                break;
            }

            // Крок 4: Пропозиція підключення Spotify
            case OnboardingStep::WIZ_STEP_SPOTIFY_ASK: {
                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("КРОК 3/3 · МУЗИКА SPOTIFY", 120, 28);

                // Вінілова платівка
                canvas.fillCircle(120, 86, 34, HallmarkTheme::BG_SURFACE);
                canvas.drawCircle(120, 86, 34, HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawCircle(120, 86, 26, HallmarkTheme::BORDER_HAIRLINE);
                canvas.drawCircle(120, 86, 18, HallmarkTheme::BORDER_HAIRLINE);
                canvas.fillCircle(120, 86, 6, HallmarkTheme::ACCENT_WARM);

                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("Підключити акаунт Spotify?", 120, 134);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("Для вінілового плеєра та Ambi", 120, 154);

                // Кнопки: Пізніше / Підключити
                canvas.fillRoundRect(22, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(22, 186, 88, 26, 13, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("◄ ПІЗНІШЕ", 66, 199);

                canvas.fillRoundRect(128, 186, 92, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(128, 186, 92, 26, 13, HallmarkTheme::ACCENT_SUCCESS);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("ПІДКЛЮЧИТИ ►", 174, 199);
                break;
            }

            // Крок 5: QR-код для швидкого входу у Spotify
            case OnboardingStep::WIZ_STEP_SPOTIFY_QR: {
                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("ВХІД У SPOTIFY", 120, 24);

                char qr_payload[160];
                char or_url[128];
                state.getOracleConfig(or_url, sizeof(or_url), nullptr, 0);
                if (strlen(or_url) > 8) {
                    snprintf(qr_payload, sizeof(qr_payload), "%s/api/v5/spotify/login_redirect", or_url);
                } else {
                    snprintf(qr_payload, sizeof(qr_payload), "http://192.168.4.1/api/v5/spotify/login_redirect");
                }

                if (strcmp(qr_payload, cached_qr_str) != 0) {
                    strncpy(cached_qr_str, qr_payload, qr_len - 1);
                    cached_qr_str[qr_len - 1] = '\0';
                    qr_cache.fillScreen(TFT_WHITE);
                    qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
                }

                canvas.fillRoundRect(70, 42, 100, 100, 8, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(70, 42, 100, 100, 8, HallmarkTheme::BORDER_HAIRLINE);
                qr_cache.pushSprite(&canvas, 70, 42);

                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("Скануй для входу в Spotify", 120, 154);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("Очікування авторизації...", 120, 172);

                canvas.fillRoundRect(24, 188, 92, 24, 12, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(24, 188, 92, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("◄ ПРОПУСТИТИ", 70, 200);
                break;
            }

            // Крок 6: Завершено
            case OnboardingStep::WIZ_STEP_DONE: {
                canvas.drawArc(120, 120, 116, 112, 0, 360, HallmarkTheme::ACCENT_SUCCESS);

                canvas.setFont(&fonts::FreeSansBold12pt7b);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("ВІТАЮ!", 120, 78);

                canvas.setFont(&fonts::efontCN_14);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("ВСЕ НАЛАШТОВАНО", 120, 106);

                if (state.sp_linked) {
                    canvas.setFont(&fonts::efontCN_12);
                    canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                    canvas.drawString("SPOTIFY ПІДКЛЮЧЕНО ✓", 120, 134);
                } else {
                    canvas.setFont(&fonts::efontCN_12);
                    canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                    canvas.drawString("Ready to Shine ✨", 120, 134);
                }

                canvas.setFont(&fonts::efontCN_12);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("Шкатулка готова до роботи", 120, 160);

                canvas.fillRoundRect(136, 188, 80, 24, 12, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(136, 188, 80, 24, 12, HallmarkTheme::ACCENT_SUCCESS);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("ПОЧАТИ ►", 176, 200);
                break;
            }
        }
    }

    // 3. SYS_NETWORK_DIAG (Сервісне вікно діагностики та Web Studio)
    inline void drawNetworkDiag(
        LGFX_Sprite& canvas,
        LGFX_Sprite& qr_cache,
        char* cached_qr_str,
        size_t qr_len,
        AppState& state
    ) {
        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);
        drawPerimeterTicks(canvas);

        char ssid[33], ip[20];
        state.getNetworkInfo(ssid, sizeof(ssid), ip, sizeof(ip));

        char qr_payload[80];
        snprintf(qr_payload, sizeof(qr_payload), "http://%s/", ip);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(state.oracle_online ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::ACCENT_COOL);
        canvas.drawString("SYSTEM DIAGNOSTICS", 120, 26);

        if (strcmp(qr_payload, cached_qr_str) != 0) {
            strncpy(cached_qr_str, qr_payload, qr_len - 1);
            cached_qr_str[qr_len - 1] = '\0';
            qr_cache.fillScreen(TFT_WHITE);
            qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
        }

        // QR-код у рамці
        canvas.fillRoundRect(75, 42, 90, 90, 6, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(75, 42, 90, 90, 6, HallmarkTheme::BORDER_HAIRLINE);
        qr_cache.pushSprite(&canvas, 75, 42);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(ssid, 120, 142);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        char net_line[64];
        snprintf(net_line, sizeof(net_line), "%s · %d dBm", ip, state.wifi_rssi);
        canvas.drawString(net_line, 120, 160);

        char mem_line[64];
        snprintf(mem_line, sizeof(mem_line), "Vol: %u%% %s · PSRAM: %lu MB",
                 (state.audio_volume * 100) / 21,
                 state.audio_muted ? "[MUTED]" : "",
                 (unsigned long)(ESP.getFreePsram() / (1024 * 1024)));
        canvas.drawString(mem_line, 120, 178);

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        canvas.drawString("Клікніть будь-яку кнопку для виходу", 120, 202);
    }

    // OTA екран оновлення
    inline void drawOta(LGFX_Sprite& canvas, AppState& state) {
        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);
        drawPerimeterTicks(canvas);

        int angle = 180 + (int)(state.ota_progress * 3.6f);
        canvas.drawArc(120, 120, 108, 105, 0, 360, HallmarkTheme::BORDER_HAIRLINE);
        canvas.drawArc(120, 120, 108, 105, 180, angle, HallmarkTheme::ACCENT_SUCCESS);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        canvas.drawString("UPDATING FIRMWARE", 120, 70);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        char p_txt[12];
        snprintf(p_txt, sizeof(p_txt), "%d%%", state.ota_progress);
        canvas.drawString(p_txt, 120, 118);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("НЕ ВИМИКАЙТЕ ЖИВЛЕННЯ", 120, 162);
    }

    // ========================================================================
    // РІВЕНЬ Б: ОСНОВНІ 4 ВІКНА-ЗАСТАВКИ (SCREEN_COUNT = 4)
    // ========================================================================

    // ВІКНО 1: SCREEN_CLIMATE (Hallmark Horology & Climate Desk Clock — 10 FPS)
    inline void drawClimate(LGFX_Sprite& canvas, AppState& state, float& smooth_hum_angle) {
        // Підсвічуємо поточну секунду на 60 засічках
        int sec = 0;
        if (state.time_synced) {
            time_t now_t = time(nullptr);
            struct tm* t_m = localtime(&now_t);
            if (t_m) sec = t_m->tm_sec;
        }
        drawPerimeterTicks(canvas, sec);

        // Тонка плавна дуга вологості (180..360 deg)
        float target_hum = 180.0f + constrain(state.humidity, 0.0f, 100.0f) * 1.8f;
        smooth_hum_angle += (target_hum - smooth_hum_angle) * 0.25f;
        canvas.drawArc(120, 120, 107, 105, 180, 360, HallmarkTheme::BORDER_HAIRLINE);
        canvas.drawArc(120, 120, 107, 105, 180, (int)smooth_hum_angle, HallmarkTheme::ACCENT_COOL);

        // Верхня капсула статусу
        canvas.fillRoundRect(30, 18, 180, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(30, 18, 180, 22, 11, HallmarkTheme::BORDER_HAIRLINE);

        uint16_t dot_col = (state.wifi_connected && state.oracle_online) ? HallmarkTheme::ACCENT_SUCCESS :
                          (state.wifi_connected ? HallmarkTheme::ACCENT_COOL :
                          (state.ap_mode_active ? HallmarkTheme::ACCENT_AMBER : HallmarkTheme::ACCENT_DANGER));
        canvas.fillCircle(42, 29, 3, dot_col);

        const char* m_name = CommandRouter::getModeNameShortUa(state.light_mode);
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);

        uint16_t sleep_min = state.getSleepTimerRemainingMin();
        char top_buf[64];
        if (sleep_min > 0) {
            snprintf(top_buf, sizeof(top_buf), "Сон %uхв • %s", sleep_min, m_name);
        } else if (state.time_synced) {
            char t_local[10];
            state.getTimeString(t_local, sizeof(t_local));
            snprintf(top_buf, sizeof(top_buf), "%s • %s", t_local, m_name);
        } else {
            snprintf(top_buf, sizeof(top_buf), "%s", m_name);
        }
        canvas.drawString(top_buf, 124, 29);

        // Центральна температура (Hero)
        int t_int = (int)state.temperature;
        int t_dec = abs((int)(state.temperature * 10.0f) - t_int * 10);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        char t_main[12];
        snprintf(t_main, sizeof(t_main), "%d", t_int);
        canvas.drawString(t_main, 96, 78);

        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        char t_frac[8];
        snprintf(t_frac, sizeof(t_frac), ".%d", t_dec);
        canvas.drawString(t_frac, 146, 86);
        canvas.drawCircle(164, 66, 3, HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("C", 176, 76);

        // Нижні дві колонки (Вологість та Тиск)
        canvas.fillRoundRect(24, 110, 192, 46, 8, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(24, 110, 192, 46, 8, HallmarkTheme::BORDER_HAIRLINE);
        canvas.drawFastVLine(120, 116, 34, HallmarkTheme::BORDER_HAIRLINE);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("ВОЛОГІСТЬ", 72, 122);
        canvas.drawString("ТИСК", 168, 122);

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        char h_str[16];
        snprintf(h_str, sizeof(h_str), "%.0f %%", state.humidity);
        canvas.drawString(h_str, 72, 142);

        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        char p_str[16];
        snprintf(p_str, sizeof(p_str), "%.0f mm", state.pressure);
        canvas.drawString(p_str, 168, 142);

        // Індикатор яскравості та гучності
        uint8_t brt_pct = state.getLedBrightnessPercent();
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        char brt_info[64];
        snprintf(brt_info, sizeof(brt_info), "%s • Світ %d%% • Звук %s%d%%",
                 state.auto_brt ? "АВТО" : "РУЧН",
                 brt_pct,
                 state.dnd_mode ? "🌙DND " : (state.audio_muted ? "🔇" : ""),
                 state.audio_muted ? 0 : (state.audio_volume * 100) / 21);
        canvas.drawString(brt_info, 120, 168);

        canvas.drawFastHLine(48, 180, 144, HallmarkTheme::BORDER_HAIRLINE);
        int bar_w = constrain((brt_pct * 144) / 100, 2, 144);
        canvas.drawFastHLine(48, 180, bar_w, HallmarkTheme::ACCENT_WARM);
        canvas.drawFastHLine(48, 181, bar_w, HallmarkTheme::ACCENT_WARM);
    }

    // ВІКНО 2: SCREEN_LIVING_GIF (Постійна GIF-Заставка LittleFS + Процедурний маятник Hallmark)
    inline void drawLivingGif(LGFX_Sprite& canvas, AppState& state, uint32_t now) {
        if (!state.saver_loaded || state.saver_num_frames == 0) {
            // Процедурна заставка в стилі швейцарського орбітального маятника Braun
            drawPerimeterTicks(canvas);
            canvas.drawCircle(120, 120, 72, HallmarkTheme::BORDER_HAIRLINE);
            canvas.drawCircle(120, 120, 48, HallmarkTheme::BORDER_HAIRLINE);

            float t = (float)now * 0.0018f;
            float angle_rad = sinf(t) * 0.85f - 1.57079f; // Коливання маятника
            int px = 120 + (int)(cosf(angle_rad) * 64.0f);
            int py = 120 + (int)(sinf(angle_rad) * 64.0f);

            canvas.drawLine(120, 120, px, py, HallmarkTheme::TEXT_SECONDARY);
            canvas.fillCircle(120, 120, 4, HallmarkTheme::ACCENT_WARM);
            canvas.fillCircle(px, py, 9, HallmarkTheme::ACCENT_WARM);
            canvas.drawCircle(px, py, 11, HallmarkTheme::TEXT_PRIMARY);

            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
            canvas.drawString("HALLMARK PENDULUM", 120, 164);
        }

        // Оверлей мінімалістичного годинника та температури поверх заставки (якщо увімкнено)
        if (state.saver_show_clock) {
            canvas.fillRoundRect(50, 18, 140, 22, 11, HallmarkTheme::BG_PRIMARY);
            canvas.drawRoundRect(50, 18, 140, 22, 11, HallmarkTheme::BORDER_HAIRLINE);

            char clk_buf[32];
            char t_str[10];
            state.getTimeString(t_str, sizeof(t_str));
            snprintf(clk_buf, sizeof(clk_buf), "%s · %.1f°C", t_str, state.temperature);

            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(clk_buf, 120, 29);
        }

        // Оверлей таймера медіа (якщо встановлено таймер показу)
        uint16_t rem_med = state.getMediaRemainingMin();
        if (rem_med > 0) {
            canvas.fillRoundRect(80, 192, 80, 18, 9, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(80, 192, 80, 18, 9, HallmarkTheme::ACCENT_WARM);
            char med_lbl[32];
            snprintf(med_lbl, sizeof(med_lbl), "⏱ %u хв", rem_med);
            canvas.setFont(&fonts::Font0);
            canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
            canvas.drawString(med_lbl, 120, 201);
        }
    }

    // ВІКНО 3: SCREEN_SMART_HUB (Telegram Inbox ⇄ Spotify Vinyl Player)
    inline void drawSmartHub(
        LGFX_Sprite& canvas,
        LGFX_Sprite& vinylSprite,
        AppState& state,
        float& vinyl_angle,
        uint32_t now
    ) {
        // --- ПРІОРИТЕТ 1: Непрочитане повідомлення Telegram ---
        if (state.tg_unread) {
            drawPerimeterTicks(canvas);

            char sender[36], msg[128], msg_time[8];
            state.getTelegramData(sender, sizeof(sender), msg, sizeof(msg), msg_time, sizeof(msg_time));

            // Пульсуючий обідок повідомлення
            uint8_t pulse = 160 + (int)(sinf((float)now * 0.006f) * 80.0f);
            uint16_t border_col = canvas.color565(pulse, pulse / 2, pulse / 3);

            canvas.fillRoundRect(22, 28, 196, 148, 10, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(22, 28, 196, 148, 10, border_col);

            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
            char s_line[64];
            snprintf(s_line, sizeof(s_line), "@%s", sender);
            canvas.drawString(s_line, 80, 46);

            canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
            canvas.drawString(msg_time[0] != '\0' ? msg_time : "--:--", 182, 46);
            canvas.drawFastHLine(30, 60, 180, HallmarkTheme::BORDER_HAIRLINE);

            char line1[64], line2[64];
            splitUtf8Lines(msg, line1, sizeof(line1), line2, sizeof(line2), 19);

            canvas.setFont(&fonts::efontCN_14);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            if (line2[0] == '\0') {
                canvas.drawString(line1, 120, 96);
            } else {
                canvas.drawString(line1, 120, 84);
                canvas.drawString(line2, 120, 108);
            }

            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
            canvas.drawString("Нове • Торкнись кришки", 120, 148);
            return;
        }

        // --- ПРІОРИТЕТ 2: Грає Spotify (Hallmark Vinyl Deck — 30 FPS) ---
        if (state.spotify.playing) {
            drawPerimeterTicks(canvas);

            // 1. Обертання вінілової платівки 136x136 px у центрі (120, 94)
            vinylSprite.pushRotateZoom(&canvas, 120, 94, vinyl_angle, 1.0f, 1.0f);
            vinyl_angle += 1.8f;
            if (vinyl_angle >= 360.0f) vinyl_angle -= 360.0f;

            // 2. Векторний тонарм (голка звукознімача) зверху справа
            canvas.drawLine(186, 26, 154, 76, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(187, 26, 155, 76, HallmarkTheme::TEXT_SECONDARY);
            canvas.fillCircle(186, 26, 5, HallmarkTheme::ACCENT_WARM); // База тонарма
            canvas.fillCircle(154, 76, 3, HallmarkTheme::ACCENT_COOL); // Голка на платівці

            // 3. Інтерпольована дуга прогресу треку по периметру (R = 116)
            uint32_t cur_prog = state.spotify.progress_ms;
            if (state.spotify.last_sync_ms > 0) {
                cur_prog += (now - state.spotify.last_sync_ms);
            }
            if (state.spotify.duration_ms > 0 && cur_prog > state.spotify.duration_ms) {
                cur_prog = state.spotify.duration_ms;
            }

            float prog_frac = (state.spotify.duration_ms > 0) ? ((float)cur_prog / (float)state.spotify.duration_ms) : 0.0f;
            int arc_deg = 180 + (int)(prog_frac * 360.0f);
            canvas.drawArc(120, 120, 108, 106, 0, 360, HallmarkTheme::BORDER_HAIRLINE);
            canvas.drawArc(120, 120, 108, 106, 180, arc_deg, HallmarkTheme::ACCENT_WARM);

            // 4. Назва треку (з горизонтальним скролінгом якщо довжина > 18 символів)
            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);

            size_t title_len = strlen(state.spotify.title);
            if (title_len > 18) {
                int scroll_offset = ((now / 45) % (int)(title_len * 9 + 40)) - 20;
                int base_x = 120 - scroll_offset;
                canvas.drawString(state.spotify.title, base_x, 172);
            } else {
                canvas.drawString(state.spotify.title[0] != '\0' ? state.spotify.title : "Spotify Track", 120, 172);
            }

            // 5. Виконавець та час треку
            canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
            canvas.drawString(state.spotify.artist[0] != '\0' ? state.spotify.artist : "Unknown Artist", 120, 188);

            char time_prog[32];
            uint32_t c_sec = cur_prog / 1000;
            uint32_t d_sec = state.spotify.duration_ms / 1000;
            snprintf(time_prog, sizeof(time_prog), "%02u:%02u / %02u:%02u",
                     c_sec / 60, c_sec % 60, d_sec / 60, d_sec % 60);
            canvas.setFont(&fonts::Font0);
            canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
            canvas.drawString(time_prog, 120, 204);
            return;
        }

        // --- ПРІОРИТЕТ 2.5: Найближча подія Google Календаря Насті (<= 15 хв до початку) ---
        if (state.cal_active && state.cal_is_soon && !state.tg_unread && !state.spotify.playing) {
            drawPerimeterTicks(canvas);

            canvas.fillRoundRect(22, 28, 196, 148, 10, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(22, 28, 196, 148, 10, HallmarkTheme::ACCENT_AMBER);

            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
            canvas.drawString("📅 КАЛЕНДАР НАСТІ", 80, 46);

            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(state.cal_time[0] != '\0' ? state.cal_time : "--:--", 182, 46);
            canvas.drawFastHLine(30, 60, 180, HallmarkTheme::BORDER_HAIRLINE);

            canvas.setFont(&fonts::efontCN_14);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(state.cal_summary, 120, 96);

            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
            canvas.drawString("Скоро початок (<= 15 хв)", 120, 148);
            return;
        }

        // --- ПРІОРИТЕТ 3: Історія повідомлень (Inbox Hub — 4 FPS) ---
        drawPerimeterTicks(canvas);

        char sender[36], msg[128], msg_time[8];
        state.getTelegramData(sender, sizeof(sender), msg, sizeof(msg), msg_time, sizeof(msg_time));

        canvas.fillRoundRect(22, 28, 196, 148, 10, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(22, 28, 196, 148, 10, HallmarkTheme::BORDER_HAIRLINE);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        char s_line[64];
        snprintf(s_line, sizeof(s_line), "@%s", sender);
        canvas.drawString(s_line, 80, 46);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString(msg_time[0] != '\0' ? msg_time : "--:--", 182, 46);
        canvas.drawFastHLine(30, 60, 180, HallmarkTheme::BORDER_HAIRLINE);

        char line1[64], line2[64];
        splitUtf8Lines(msg, line1, sizeof(line1), line2, sizeof(line2), 19);

        canvas.setFont(&fonts::efontCN_14);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        if (line2[0] == '\0') {
            canvas.drawString(line1, 120, 96);
        } else {
            canvas.drawString(line1, 120, 84);
            canvas.drawString(line2, 120, 108);
        }

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        char pg_buf[32];
        snprintf(pg_buf, sizeof(pg_buf), "%u / %u • R: Історія", state.inbox_view_idx + 1, state.inbox_count);
        canvas.drawString(pg_buf, 120, 148);
    }

    // ВІКНО 4: SCREEN_BAROMETER (Dieter Rams Braun Barometer — 4 FPS)
    inline void drawBarometer(LGFX_Sprite& canvas, AppState& state, float& smooth_pres_angle) {
        drawPerimeterTicks(canvas);

        float hist[24];
        uint8_t h_cnt = 0;
        float p_diff = 0.0f;
        state.getPressureHistory(hist, h_cnt, p_diff);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("BAROMETRIC TREND", 120, 32);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        char p_str[16];
        snprintf(p_str, sizeof(p_str), "%.0f", state.pressure);
        canvas.drawString(p_str, 120, 68);

        const char* arrow_sym = "→";
        uint16_t trend_col = HallmarkTheme::TEXT_SECONDARY;
        const char* trend_word = "СТАБІЛЬНИЙ";

        if (p_diff >= 0.7f) {
            arrow_sym = "↑";
            trend_col = HallmarkTheme::ACCENT_SUCCESS;
            trend_word = "ЗРОСТАЄ";
        } else if (p_diff <= -0.7f) {
            arrow_sym = "↓";
            trend_col = HallmarkTheme::ACCENT_DANGER;
            trend_word = "ПАДАЄ";
        }

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(trend_col);
        char trend_str[64];
        snprintf(trend_str, sizeof(trend_str), "%s %s (%+.1f mmHg)", arrow_sym, trend_word, p_diff);
        canvas.drawString(trend_str, 120, 96);

        const char* wf = "Стабільна погода";
        if (state.pressure < 742.0f || p_diff <= -1.5f) wf = "Можливі опади / Циклон";
        else if (state.pressure > 760.0f || p_diff >= 1.5f) wf = "Ясно / Антициклон";

        canvas.fillRoundRect(36, 110, 168, 20, 10, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(36, 110, 168, 20, 10, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(wf, 120, 120);

        // 24-точковий Sparkline з базовою лінією 750 mmHg
        canvas.fillRoundRect(36, 138, 168, 44, 6, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(36, 138, 168, 44, 6, HallmarkTheme::BORDER_HAIRLINE);

        int baseline_y = 160;
        canvas.drawFastHLine(42, baseline_y, 156, HallmarkTheme::BORDER_HAIRLINE);

        for (int i = 0; i < 24; i++) {
            float v = constrain(hist[i], 735.0f, 765.0f);
            int bar_h = 3 + (int)((v - 735.0f) / 30.0f * 26.0f);
            int bx = 45 + i * 6;
            int by = 176 - bar_h;
            uint16_t b_col = (i == 23) ? HallmarkTheme::ACCENT_WARM :
                            ((i >= 18) ? HallmarkTheme::ACCENT_COOL : HallmarkTheme::BORDER_HAIRLINE);
            canvas.fillRect(bx, by, 4, bar_h, b_col);
        }

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("750", 192, baseline_y - 4);
    }

    // ВІКНО 5: SCREEN_POMODORO (Hallmark Horology Focus Dial & Timer — 10 FPS)
    inline void drawPomodoro(LGFX_Sprite& canvas, AppState& state, uint32_t now) {
        drawPerimeterTicks(canvas);

        uint16_t rem_sec = state.getPomoRemainingSeconds();
        uint16_t tot_sec = (uint16_t)state.pomo_duration_min * 60;
        float prog = (tot_sec > 0) ? ((float)rem_sec / (float)tot_sec) : 0.0f;
        prog = constrain(prog, 0.0f, 1.0f);

        // Хорологічна дуга зворотного відліку
        uint16_t arc_col = state.pomo_is_break ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::ACCENT_WARM;
        int deg_end = (int)(prog * 360.0f);
        canvas.drawArc(120, 120, 108, 104, 0, 360, HallmarkTheme::BORDER_HAIRLINE);
        if (deg_end > 0) {
            canvas.drawArc(120, 120, 108, 104, 270, (270 + deg_end) % 360, arc_col);
        }

        // Заголовок
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("HALLMARK FOCUS DIAL", 120, 32);

        // Великі цифри таймера
        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        char t_buf[16];
        uint16_t m = rem_sec / 60;
        uint16_t s = rem_sec % 60;
        snprintf(t_buf, sizeof(t_buf), "%02u:%02u", m, s);
        canvas.drawString(t_buf, 120, 84);

        // Статус-бейдж
        const char* st_txt = "НАЛАШТУВАННЯ (◄/►)";
        uint16_t badge_col = HallmarkTheme::BORDER_HAIRLINE;
        uint16_t text_col = HallmarkTheme::TEXT_SECONDARY;
        if (state.pomo_active) {
            if (state.pomo_paused) {
                st_txt = "ПАУЗА • ТОРКНИСЬ КРИШКИ";
                badge_col = HallmarkTheme::ACCENT_COOL;
                text_col = HallmarkTheme::ACCENT_COOL;
            } else if (state.pomo_is_break) {
                st_txt = "☕ ПЕРЕРВА • СМАРАГД";
                badge_col = HallmarkTheme::ACCENT_SUCCESS;
                text_col = HallmarkTheme::ACCENT_SUCCESS;
            } else {
                st_txt = "📚 ФОКУС • БУРШТИН";
                badge_col = HallmarkTheme::ACCENT_WARM;
                text_col = HallmarkTheme::ACCENT_WARM;
            }
        }

        canvas.fillRoundRect(32, 114, 176, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(32, 114, 176, 22, 11, badge_col);
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(text_col);
        canvas.drawString(st_txt, 120, 125);

        // Капсули керування над кнопками
        const char* l_lbl = state.pomo_active ? "◄ -1м" : "◄ -5м";
        canvas.fillRoundRect(22, 148, 58, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(22, 148, 58, 22, 11, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(l_lbl, 51, 159);

        const char* t_lbl = !state.pomo_active ? "СТАРТ" : (state.pomo_paused ? "ДАЛІ" : "ПАУЗА");
        canvas.fillRoundRect(88, 148, 64, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(88, 148, 64, 22, 11, arc_col);
        canvas.setTextColor(arc_col);
        canvas.drawString(t_lbl, 120, 159);

        const char* r_lbl = state.pomo_active ? "+1м ►" : "+5м ►";
        canvas.fillRoundRect(160, 148, 58, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(160, 148, 58, 22, 11, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(r_lbl, 189, 159);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("Утримуй ◄ для виходу", 120, 184);
    }

    // СИСТЕМНИЙ СТАН: SYS_BIRTHDAY_CELEBRATE (Святкова картка привітання Насті)
    inline void drawBirthdayCelebrate(LGFX_Sprite& canvas, AppState& state, uint32_t now) {
        drawPerimeterTicks(canvas);

        uint8_t pulse = 180 + (int)(sinf((float)now * 0.005f) * 70.0f);
        uint16_t gold_col = canvas.color565(pulse, (pulse * 3) / 4, pulse / 3);

        canvas.drawArc(120, 120, 114, 111, 0, 360, gold_col);
        canvas.drawArc(120, 120, 107, 106, 0, 360, HallmarkTheme::ACCENT_WARM);

        canvas.fillRoundRect(22, 28, 196, 174, 14, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(22, 28, 196, 174, 14, gold_col);

        canvas.fillCircle(120, 52, 16, HallmarkTheme::BG_PRIMARY);
        canvas.drawCircle(120, 52, 16, gold_col);
        canvas.setFont(&fonts::efontCN_14);
        canvas.setTextColor(gold_col);
        canvas.drawString("🎂", 120, 52);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("З ДНЕМ НАРОДЖЕННЯ,", 120, 78);

        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString("НАСТЮ! ✨", 120, 104);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(gold_col);
        canvas.drawString("05.11 · Твій особливий день", 120, 130);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("Тепла, затишку та посмішок", 120, 150);

        canvas.fillRoundRect(70, 168, 100, 24, 12, HallmarkTheme::BG_PRIMARY);
        canvas.drawRoundRect(70, 168, 100, 24, 12, gold_col);
        canvas.setTextColor(gold_col);
        canvas.drawString("🪄 ДЯКУЮ", 120, 180);
    }

    // НАВІГАЦІЙНИЙ ПІДВАЛ (FOOTER NAV ДЛЯ 5 ВІКОН)
    inline void drawFooterNav(LGFX_Sprite& canvas, AppState& state) {
        for (int i = 0; i < SCREEN_COUNT; i++) {
            int x = 96 + i * 12;
            uint16_t c = (state.screen_mode == i) ? HallmarkTheme::ACCENT_WARM : HallmarkTheme::BORDER_HAIRLINE;
            canvas.fillCircle(x, 202, (state.screen_mode == i) ? 3 : 2, c);
        }

        canvas.fillRoundRect(68,  212, 28, 14, 7, state.btn_l_pressed ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(68,  212, 28, 14, 7, HallmarkTheme::BORDER_HAIRLINE);

        canvas.fillRoundRect(106, 212, 28, 14, 7, state.btn_t_pressed ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(106, 212, 28, 14, 7, HallmarkTheme::BORDER_HAIRLINE);

        canvas.fillRoundRect(144, 212, 28, 14, 7, state.btn_r_pressed ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(144, 212, 28, 14, 7, HallmarkTheme::BORDER_HAIRLINE);

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        const char* l_btn = (state.screen_mode == SCREEN_POMODORO) ? (state.pomo_active ? "-1М" : "-5М") : "ЕКР";
        canvas.drawString(l_btn, 82, 219);

        const char* m_btn = (state.screen_mode == SCREEN_POMODORO) ? (!state.pomo_active ? "ПУСК" : "ПАУЗ") :
                            (state.dnd_mode ? "DND" : (state.audio_muted ? "ТИША" : "СВІТ"));
        canvas.drawString(m_btn, 120, 219);

        const char* r_lbl = "ЯСК";
        if (state.screen_mode == SCREEN_LIVING_GIF) r_lbl = "ГОД";
        else if (state.screen_mode == SCREEN_SMART_HUB) r_lbl = state.spotify.playing ? "ТРЕК" : "ДАЛІ";
        else if (state.screen_mode == SCREEN_POMODORO) r_lbl = state.pomo_active ? "+1М" : "+5М";
        else if (state.auto_brt) r_lbl = "АВТ";

        canvas.drawString(r_lbl, 158, 219);
    }
}
