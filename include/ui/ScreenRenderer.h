#pragma once
#include <LovyanGFX.hpp>
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "ui/HallmarkTheme.h"
#include "ui/UkrainianFont12.h"
#include "ui/UkrainianFont12Bold.h"
#include "ui/UkrainianFont14.h"
#include "ui/UkrainianFont16Bold.h"
#include "utils/TextSanitizer.h"

/**
 * ============================================================================
 * ScreenRenderer (v4.5 Hallmark Horology, Interactive Wizard & Living Screensavers)
 * Inspired by Dieter Rams Braun horology and Hallmark Atelier design standards.
 * Pure tokenized RGB565 palette, zero acidic gradients, surgical typography.
 * ============================================================================
 */

namespace ScreenRenderer {

    inline void splitUtf8Lines(const char* text, char* line1, size_t max1, char* line2, size_t max2, size_t max_chars_per_line = 23) {
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
            float rad = (float)i * (6.0f * 0.0174532925f) - 1.5707963f; // 0 index at 12 o'clock!
            float cos_v = cosf(rad);
            float sin_v = sinf(rad);
            bool is_major = (i % 5 == 0);
            int r_in = is_major ? 112 : 115;
            uint16_t col = (i == highlight_sec) ? HallmarkTheme::ACCENT_AMBER :
                           (is_major ? HallmarkTheme::TICK_MAJOR : HallmarkTheme::TICK_NORMAL);
            canvas.drawLine(120 + (int)(cos_v * r_in), 120 + (int)(sin_v * r_in),
                            120 + (int)(cos_v * 118),  120 + (int)(sin_v * 118), col);
            if (i == highlight_sec) {
                canvas.fillSmoothCircle(120 + (int)(cos_v * 114), 120 + (int)(sin_v * 114), 3, HallmarkTheme::ACCENT_AMBER);
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

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        canvas.drawString("HALLMARK v4.5", 120, 116);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("I2C | I2S | PSRAM | OK", 120, 144);

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
                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
                canvas.drawString("WELCOME TO MAGICBOX", 120, 30);

                // Іконка динаміка в центрі
                canvas.drawCircle(120, 86, 32, HallmarkTheme::BORDER_HAIRLINE);
                canvas.fillCircle(120, 86, 26, HallmarkTheme::BG_SURFACE);
                canvas.drawCircle(120, 86, 26, HallmarkTheme::ACCENT_COOL);

                canvas.setFont(&fonts::FreeSansBold12pt7b);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("SOUND?", 120, 86);

                canvas.setFont(&UkrainianFont14);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("Увімкнути звук колонки?", 120, 130);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("I2S DAC | MAX98357A | Hi-Fi", 120, 150);

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

            // Крок 1: Hellcat Telegram Bot QR з deep-link claim (@SmartLampEdlerAGBot?start=c_<claim_secret>)
            case OnboardingStep::WIZ_STEP_TG_BOT: {
                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
                canvas.drawString("КРОК 1/2 | TELEGRAM-БОТ", 120, 22);

                char qr_payload[160];
                if (strlen(state.claim_secret) >= 4) {
                    snprintf(qr_payload, sizeof(qr_payload), "https://t.me/SmartLampEdlerAGBot?start=c_%s", state.claim_secret);
                } else {
                    snprintf(qr_payload, sizeof(qr_payload), "https://t.me/SmartLampEdlerAGBot");
                }

                if (strcmp(qr_payload, cached_qr_str) != 0) {
                    strncpy(cached_qr_str, qr_payload, qr_len - 1);
                    cached_qr_str[qr_len - 1] = '\0';
                    qr_cache.fillScreen(TFT_WHITE);
                    qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
                }

                canvas.fillRoundRect(70, 38, 100, 100, 8, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(70, 38, 100, 100, 8, HallmarkTheme::BORDER_HAIRLINE);
                qr_cache.pushSprite(&canvas, 70, 38);

                canvas.setFont(&UkrainianFont14);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("1. Скануй QR камерою", 120, 148);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("Бот допоможе з налаштуванням", 120, 166);

                canvas.fillRoundRect(24, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(24, 186, 88, 26, 13, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("◄ НАЗАД", 68, 199);

                canvas.fillRoundRect(128, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(128, 186, 88, 26, 13, HallmarkTheme::ACCENT_AMBER);
                canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
                canvas.drawString("ДАЛІ ►", 172, 199);
                break;
            }

            // Крок 2: Точка доступу лампи для підключення до домашнього Wi-Fi
            case OnboardingStep::WIZ_STEP_WIFI_AP: {
                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("КРОК 2/2 | ДОМАШНІЙ WI-FI", 120, 22);

                const char* qr_payload = "WIFI:S:MagicBox-AP;T:nopass;;";
                if (strcmp(qr_payload, cached_qr_str) != 0) {
                    strncpy(cached_qr_str, qr_payload, qr_len - 1);
                    cached_qr_str[qr_len - 1] = '\0';
                    qr_cache.fillScreen(TFT_WHITE);
                    qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
                }

                canvas.fillRoundRect(70, 38, 100, 100, 8, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(70, 38, 100, 100, 8, HallmarkTheme::BORDER_HAIRLINE);
                qr_cache.pushSprite(&canvas, 70, 38);

                canvas.setFont(&UkrainianFont14);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("2. Обери домашній Wi-Fi", 120, 148);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("SSID: MagicBox-AP (192.168.4.1)", 120, 166);

                canvas.fillRoundRect(24, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(24, 186, 88, 26, 13, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("◄ НАЗАД", 68, 199);

                canvas.fillRoundRect(128, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(128, 186, 88, 26, 13, HallmarkTheme::ACCENT_COOL);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("ДАЛІ ►", 172, 199);
                break;
            }

            // Крок 3: Анімація з'єднання з домашнім Wi-Fi та хмарою
            case OnboardingStep::WIZ_STEP_CONNECTING: {
                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("ПІДКЛЮЧЕННЯ ДО ХМАРИ", 120, 24);

                uint32_t phase = (millis() / 25) % 360;
                canvas.drawCircle(120, 90, 36, HallmarkTheme::BORDER_HAIRLINE);
                canvas.drawArc(120, 90, 36, 33, phase, (phase + 120) % 360, HallmarkTheme::ACCENT_COOL);
                canvas.fillCircle(120, 90, 24, HallmarkTheme::BG_SURFACE);

                canvas.setFont(&fonts::FreeSansBold12pt7b);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("...", 120, 88);

                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString(state.wifi_connected ? "Wi-Fi підключено ✓" : "З'єднання з Wi-Fi...", 120, 142);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("З'єднуюсь із сервером...", 120, 162);

                canvas.fillRoundRect(24, 186, 88, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(24, 186, 88, 26, 13, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("◄ WI-FI", 68, 199);
                break;
            }

            // Крок 4: Екран очікування налаштування в боті + живий чекліст su
            case OnboardingStep::WIZ_STEP_WAITING_CHECKLIST: {
                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
                canvas.drawString("ШКАТУЛКА ОНЛАЙН ✨", 120, 20);

                // Картка чекліста
                canvas.fillRoundRect(20, 34, 200, 142, 10, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(20, 34, 200, 142, 10, HallmarkTheme::BORDER_HAIRLINE);

                uint8_t su = state.setup_bitmask;
                bool s_wifi = (su & 0x01) || state.wifi_connected;
                bool s_pair = (su & 0x02);
                bool s_mood = (su & 0x04);
                bool s_sp   = (su & 0x08) || state.sp_linked;
                bool s_cal  = (su & 0x10) || state.cal_active;

                auto drawItem = [&](int y, const char* name, bool done) {
                    canvas.setFont(&UkrainianFont12Bold);
                    canvas.setTextColor(done ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::TEXT_SECONDARY);
                    canvas.drawString(done ? "✓" : "○", 36, y);
                    canvas.setTextColor(done ? HallmarkTheme::TEXT_PRIMARY : HallmarkTheme::TEXT_SECONDARY);
                    canvas.setTextDatum(textdatum_t::middle_left);
                    canvas.drawString(name, 50, y);
                    canvas.setTextDatum(textdatum_t::middle_center);
                };

                drawItem(50,  "Wi-Fi зв'язок", s_wifi);
                drawItem(74,  "Прив'язка бота", s_pair);
                drawItem(98,  "Атмосфера лампи", s_mood);
                drawItem(122, "Музика Spotify", s_sp);
                drawItem(146, "Розклад & Календар", s_cal);

                canvas.fillRoundRect(128, 186, 92, 26, 13, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(128, 186, 92, 26, 13, HallmarkTheme::BORDER_HAIRLINE);
                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("ПІЗНІШЕ ►", 174, 199);
                break;
            }

            // Крок 5: Завершено! Перехід на робочий стіл
            case OnboardingStep::WIZ_STEP_DONE: {
                canvas.drawArc(120, 120, 116, 112, 0, 360, HallmarkTheme::ACCENT_SUCCESS);

                canvas.setFont(&UkrainianFont16Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_SUCCESS);
                canvas.drawString("ВІТАЮ! ✨", 120, 78);

                canvas.setFont(&UkrainianFont14);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString("ШКАТУЛКА ГОТОВА", 120, 106);

                canvas.setFont(&UkrainianFont12Bold);
                canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
                canvas.drawString("Приємного користування", 120, 134);

                canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
                canvas.drawString("Запуск годинника...", 120, 160);

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

        canvas.setFont(&UkrainianFont12Bold);
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

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(ssid, 120, 142);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        char net_line[64];
        snprintf(net_line, sizeof(net_line), "%s | %d dBm", ip, state.wifi_rssi);
        canvas.drawString(net_line, 120, 160);

        char mem_line[64];
        snprintf(mem_line, sizeof(mem_line), "WS:%s (rec:%u) | PSRAM: %lu MB",
                 state.oracle_online ? "ON" : "OFF",
                 (unsigned int)state.ws_reconnect_cnt,
                 (unsigned long)(ESP.getFreePsram() / (1024 * 1024)));
        canvas.drawString(mem_line, 120, 178);

        canvas.setFont(&UkrainianFont12Bold);
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

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        canvas.drawString("UPDATING FIRMWARE", 120, 70);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        char p_txt[12];
        snprintf(p_txt, sizeof(p_txt), "%d%%", state.ota_progress);
        canvas.drawString(p_txt, 120, 118);

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("НЕ ВИМИКАЙТЕ ЖИВЛЕННЯ", 120, 162);
    }

    // ========================================================================
    // РІВЕНЬ Б: ОСНОВНІ 4 ВІКНА-ЗАСТАВКИ (SCREEN_COUNT = 4)
    // ========================================================================

    // ВІКНО 1: SCREEN_CLIMATE (Luxury Swiss Horology & Climate Desk Clock — 10 FPS)
    inline void drawClimate(LGFX_Sprite& canvas, AppState& state) {
        // 1. Делікатне радіальне світіння у колір поточної теми
        uint8_t aur_r = state.ambi_r / 20;
        uint8_t aur_g = state.ambi_g / 20;
        uint8_t aur_b = state.ambi_b / 20;
        if (aur_r == 0 && aur_g == 0 && aur_b == 0) {
            aur_r = 14; aur_g = 10; aur_b = 6; // Теплий бурштиновий відблиск
        }
        canvas.fillSmoothCircle(120, 112, 64, canvas.color565(aur_r, aur_g, aur_b));
        canvas.fillSmoothCircle(120, 112, 44, canvas.color565(aur_r * 2, aur_g * 2, aur_b * 2));

        // 2. Зовнішнє кільце (Радіус 112..118 px): 60 засічок з бурштиновою секундною точкою
        int sec = 0;
        if (state.time_synced) {
            time_t now_t = time(nullptr);
            struct tm* t_m = localtime(&now_t);
            if (t_m) sec = t_m->tm_sec;
        }
        drawPerimeterTicks(canvas, sec);

        // 3. Верхній блок (Y = 38..46): Крупний ідеально центрований час, нуль слів поруч
        char t_buf[16];
        if (state.time_synced) {
            state.getTimeString(t_buf, sizeof(t_buf));
        } else {
            strncpy(t_buf, "--:--", sizeof(t_buf));
        }
        canvas.setFont(&fonts::FreeSansBold18pt7b);
        canvas.setTextSize(1.0f);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(t_buf, 120, 42);

        // 4. Центральний блок (Y = 105..115): Монолітна температура з хірургічним кернінгом
        float temp_val = state.temperature;
        int t_int = (int)temp_val;
        int t_dec = abs((int)(temp_val * 10.0f) - t_int * 10);

        char int_buf[12];
        snprintf(int_buf, sizeof(int_buf), "%d", t_int);
        char dec_buf[8];
        snprintf(dec_buf, sizeof(dec_buf), ".%d", t_dec);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextSize(1.35f);
        int w_int = canvas.textWidth(int_buf);

        canvas.setFont(&fonts::FreeSansBold18pt7b);
        canvas.setTextSize(1.0f);
        int w_dec = canvas.textWidth(dec_buf);

        canvas.setFont(&fonts::FreeSansBold12pt7b);
        int w_c = canvas.textWidth("C");
        int w_deg = 6;
        int w_unit = w_deg + 3 + w_c;

        int gap_dec = 1;  // Впритул між цілою і десятковою частиною (нуль дірок!)
        int gap_unit = 5; // Відступ до одиниці виміру
        int total_w = w_int + gap_dec + w_dec + gap_unit + w_unit;
        int start_x = (240 - total_w) / 2;

        int base_y = 112;

        // 4.1. Ціла частина (Hero, монолітна)
        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextSize(1.35f);
        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(int_buf, start_x, base_y);

        // 4.2. Десяткова частина (.6)
        int cur_x = start_x + w_int + gap_dec;
        canvas.setFont(&fonts::FreeSansBold18pt7b);
        canvas.setTextSize(1.0f);
        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        canvas.drawString(dec_buf, cur_x, base_y + 4);

        // 4.3. Символ градуса та 'C'
        cur_x += w_dec + gap_unit;
        canvas.drawCircle(cur_x + 3, base_y - 12, 3, HallmarkTheme::TEXT_SECONDARY);
        canvas.drawCircle(cur_x + 3, base_y - 12, 2, HallmarkTheme::TEXT_SECONDARY);
        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextSize(1.0f);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("C", cur_x + w_deg + 3, base_y);

        // 5. Нижній блок (Y = 162..196): Два заокруглених бейджі-капсули (#12141A, r=12)
        // Ліва капсула: Вологість
        canvas.fillSmoothRoundRect(28, 162, 88, 34, 12, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(28, 162, 88, 34, 12, HallmarkTheme::BORDER_HAIRLINE);

        int cx = 46, cy = 179;
        canvas.fillCircle(cx, cy + 2, 3, HallmarkTheme::ACCENT_COOL);
        canvas.fillTriangle(cx - 3, cy + 2, cx + 3, cy + 2, cx, cy - 4, HallmarkTheme::ACCENT_COOL);

        char h_buf[12];
        snprintf(h_buf, sizeof(h_buf), "%.0f%%", state.humidity);
        canvas.setFont(&UkrainianFont16Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        canvas.drawString(h_buf, 79, 179);

        // Права капсула: Барометр + тренд
        canvas.fillSmoothRoundRect(124, 162, 88, 34, 12, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(124, 162, 88, 34, 12, HallmarkTheme::BORDER_HAIRLINE);

        int ax = 138, ay = 179;
        if (state.pressure_diff > 0.35f) {
            canvas.drawLine(ax, ay + 4, ax, ay - 4, HallmarkTheme::ACCENT_AMBER);
            canvas.drawLine(ax - 2, ay - 2, ax, ay - 4, HallmarkTheme::ACCENT_AMBER);
            canvas.drawLine(ax + 2, ay - 2, ax, ay - 4, HallmarkTheme::ACCENT_AMBER);
        } else if (state.pressure_diff < -0.35f) {
            canvas.drawLine(ax, ay - 4, ax, ay + 4, HallmarkTheme::ACCENT_AMBER);
            canvas.drawLine(ax - 2, ay + 2, ax, ay + 4, HallmarkTheme::ACCENT_AMBER);
            canvas.drawLine(ax + 2, ay + 2, ax, ay + 4, HallmarkTheme::ACCENT_AMBER);
        } else {
            canvas.drawLine(ax - 3, ay, ax + 3, ay, HallmarkTheme::ACCENT_AMBER);
            canvas.drawLine(ax + 1, ay - 2, ax + 3, ay, HallmarkTheme::ACCENT_AMBER);
            canvas.drawLine(ax + 1, ay + 2, ax + 3, ay, HallmarkTheme::ACCENT_AMBER);
        }

        char p_buf[10];
        snprintf(p_buf, sizeof(p_buf), "%.0f", state.pressure);
        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.drawString(p_buf, 148, 179);
        int w_p = canvas.textWidth(p_buf);
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("мм", 148 + w_p + 2, 180);
    }

    // ВІКНО 2: SCREEN_LIVING_GIF    // ВІКНО 2: SCREEN_LIVING_GIF (Постійна GIF-Заставка LittleFS + Процедурний маятник Hallmark)
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

            canvas.setFont(&UkrainianFont12Bold);
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
            snprintf(clk_buf, sizeof(clk_buf), "%s | %.1f°C", t_str, state.temperature);

            canvas.setFont(&UkrainianFont14);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(clk_buf, 120, 29);
        }

        // Оверлей таймера медіа (якщо встановлено таймер показу)
        uint16_t rem_med = state.getMediaRemainingMin();
        if (rem_med > 0) {
            canvas.fillRoundRect(80, 180, 80, 18, 9, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(80, 180, 80, 18, 9, HallmarkTheme::ACCENT_WARM);
            char med_lbl[32];
            snprintf(med_lbl, sizeof(med_lbl), "⏱ %u хв", rem_med);
            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
            canvas.drawString(med_lbl, 120, 189);
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

            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
            char s_line[64];
            snprintf(s_line, sizeof(s_line), "@%s", sender);
            canvas.drawString(s_line, 80, 46);

            canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
            canvas.drawString(msg_time[0] != '\0' ? msg_time : "--:--", 182, 46);
            canvas.drawFastHLine(30, 60, 180, HallmarkTheme::BORDER_HAIRLINE);

            char line1[64], line2[64];
            splitUtf8Lines(msg, line1, sizeof(line1), line2, sizeof(line2), 19);

            canvas.setFont(&UkrainianFont14);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            if (line2[0] == '\0') {
                canvas.drawString(line1, 120, 96);
            } else {
                canvas.drawString(line1, 120, 84);
                canvas.drawString(line2, 120, 108);
            }

            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
            canvas.drawString("Нове | Торкнись кришки", 120, 148);
            return;
        }

        // --- ПРІОРИТЕТ 2: Spotify (Hallmark Vinyl Deck — 30 FPS) ---
        bool has_spotify_deck = (state.spotify.playing || (state.spotify.track_id[0] != '\0' && (now - state.spotify.last_sync_ms < 60000UL)));
        if (has_spotify_deck) {
            drawPerimeterTicks(canvas);

            // 1. Обертання вінілової платівки 136x136 px у центрі (120, 94) з фізичною інерцією
            static float s_vinyl_speed = 0.0f;
            float target_speed = state.spotify.playing ? 1.8f : 0.0f;
            // 33.3 RPM: розгін від 0 за ~650 мс, плавний вибіг/гальмування за ~850 мс
            float inertia_factor = state.spotify.playing ? 0.082f : 0.045f;
            s_vinyl_speed += (target_speed - s_vinyl_speed) * inertia_factor;
            if (s_vinyl_speed > 0.005f) {
                vinyl_angle += s_vinyl_speed;
                if (vinyl_angle >= 360.0f) vinyl_angle -= 360.0f;
            }
            vinylSprite.pushRotateZoom(&canvas, 120, 94, vinyl_angle, 1.0f, 1.0f);

            // Оптичний міст (Optical Bridge): м'яке 4px градієнтне свічення вздовж нижньої кромки дисплея (дуга 45°..135° знизу)
            // Колір синхронізовано з передніми діодами Дна для візуального «перетікання» з екрана на стіл
            uint8_t br_r = state.ambi_r;
            uint8_t br_g = state.ambi_g;
            uint8_t br_b = state.ambi_b;
            uint16_t bridge_col = canvas.color565(br_r, br_g, br_b);
            uint16_t bridge_dim = canvas.color565(br_r / 3, br_g / 3, br_b / 3);
            canvas.drawArc(120, 120, 119, 117, 45, 135, bridge_col);
            canvas.drawArc(120, 120, 117, 115, 45, 135, bridge_dim);

            // 2. Векторний тонарм (голка звукознімача) зверху справа
            if (state.spotify.playing) {
                canvas.drawLine(186, 26, 154, 76, HallmarkTheme::TEXT_SECONDARY);
                canvas.drawLine(187, 26, 155, 76, HallmarkTheme::TEXT_SECONDARY);
                canvas.fillCircle(186, 26, 5, HallmarkTheme::ACCENT_WARM);
                canvas.fillCircle(154, 76, 3, HallmarkTheme::ACCENT_COOL);
            } else {
                // Паркова позиція тонарма коли платівка зупинена на паузі
                canvas.drawLine(186, 26, 192, 72, HallmarkTheme::BORDER_HAIRLINE);
                canvas.drawLine(187, 26, 193, 72, HallmarkTheme::BORDER_HAIRLINE);
                canvas.fillCircle(186, 26, 5, HallmarkTheme::TEXT_SECONDARY);
                canvas.fillCircle(192, 72, 3, HallmarkTheme::TEXT_SECONDARY);
            }

            // 2. Плавна кругова дуга прогресу треку навколо вінілової платівки (R = 73..71)
            // Синхронізована за кольором з передньою зоною нижнього Ambilight
            uint32_t cur_prog = state.getLocalSpotifyProgressMs();
            float prog_frac = (state.spotify.duration_ms > 0) ? ((float)cur_prog / (float)state.spotify.duration_ms) : 0.0f;
            prog_frac = constrain(prog_frac, 0.0f, 1.0f);

            uint16_t prog_col = canvas.color565(state.ambi_r, state.ambi_g, state.ambi_b);
            if (state.ambi_r == 0 && state.ambi_g == 0 && state.ambi_b == 0) {
                prog_col = HallmarkTheme::ACCENT_WARM;
            }
            canvas.drawArc(120, 94, 73, 71, 0, 360, HallmarkTheme::BORDER_HAIRLINE);
            int prog_deg = (int)(prog_frac * 360.0f);
            if (prog_deg > 0) {
                canvas.drawArc(120, 94, 73, 71, 270, 270 + prog_deg, prog_col);
            }

            // 3. Назва треку (шрифт UkrainianFont16Bold з високим контрастом і плавним скролінгом)
            canvas.setFont(&UkrainianFont16Bold);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);

            int title_w = canvas.textWidth(state.spotify.title[0] != '\0' ? state.spotify.title : "Spotify");
            if (title_w > 192) {
                int scroll_range = title_w + 50;
                int scroll_offset = (now / 35) % scroll_range;
                int base_x = 120 + 96 - scroll_offset;
                canvas.setClipRect(20, 158, 200, 22);
                canvas.setTextDatum(textdatum_t::middle_left);
                canvas.drawString(state.spotify.title, base_x, 169);
                canvas.clearClipRect();
            } else {
                canvas.setTextDatum(textdatum_t::middle_center);
                canvas.drawString(state.spotify.title[0] != '\0' ? state.spotify.title : "Spotify", 120, 169);
            }

            // 4. Виконавець та час треку (великий згладжений шрифт, нуль дрібного тексту)
            canvas.setFont(&UkrainianFont14);
            canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.drawString(state.spotify.artist[0] != '\0' ? state.spotify.artist : "Unknown Artist", 120, 189);

            char time_prog[32];
            uint32_t c_sec = cur_prog / 1000;
            uint32_t d_sec = state.spotify.duration_ms / 1000;
            snprintf(time_prog, sizeof(time_prog), "%02u:%02u / %02u:%02u",
                     c_sec / 60, c_sec % 60, d_sec / 60, d_sec % 60);
            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
            canvas.drawString(time_prog, 120, 204);
            return;
        }

        // --- ПРІОРИТЕТ 2.5: Найближча подія Google Календаря Насті (<= 15 хв до початку) ---
        if (state.cal_active && state.cal_is_soon && !state.tg_unread && !has_spotify_deck) {
            drawPerimeterTicks(canvas);

            canvas.fillRoundRect(22, 28, 196, 148, 10, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(22, 28, 196, 148, 10, HallmarkTheme::ACCENT_AMBER);

            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextColor(HallmarkTheme::ACCENT_AMBER);
            canvas.drawString("КАЛЕНДАР НАСТІ", 80, 46);

            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(state.cal_time[0] != '\0' ? state.cal_time : "--:--", 182, 46);
            canvas.drawFastHLine(30, 60, 180, HallmarkTheme::BORDER_HAIRLINE);

            canvas.setFont(&UkrainianFont14);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(state.cal_summary, 120, 96);

            canvas.setFont(&UkrainianFont12Bold);
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

        // Індикатор актуальності хмарної синхронізації
        uint32_t sync_age = (state.last_cloud_sync_ms > 0) ? (now - state.last_cloud_sync_ms) : 999999;
        uint16_t sync_col = (sync_age < 15000) ? HallmarkTheme::ACCENT_SUCCESS : ((sync_age < 45000) ? HallmarkTheme::ACCENT_WARM : HallmarkTheme::ACCENT_DANGER);
        canvas.fillCircle(34, 46, 3, sync_col);

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        char s_line[64];
        snprintf(s_line, sizeof(s_line), "@%s", sender);
        canvas.drawString(s_line, 80, 46);

        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString(msg_time[0] != '\0' ? msg_time : "--:--", 182, 46);
        canvas.drawFastHLine(30, 60, 180, HallmarkTheme::BORDER_HAIRLINE);

        char line1[64], line2[64];
        splitUtf8Lines(msg, line1, sizeof(line1), line2, sizeof(line2), 19);

        canvas.setFont(&UkrainianFont14);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        if (line2[0] == '\0') {
            canvas.drawString(line1, 120, 96);
        } else {
            canvas.drawString(line1, 120, 84);
            canvas.drawString(line2, 120, 108);
        }

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        char pg_buf[32];
        snprintf(pg_buf, sizeof(pg_buf), "%u / %u | R: Історія", state.inbox_view_idx + 1, state.inbox_count);
        canvas.drawString(pg_buf, 120, 148);
    }

    // ВІКНО 4: SCREEN_BAROMETER (Dieter Rams Braun Barometer — 4 FPS)
    inline void drawBarometer(LGFX_Sprite& canvas, AppState& state, float& smooth_pres_angle) {
        drawPerimeterTicks(canvas);

        float hist[24];
        uint8_t h_cnt = 0;
        float p_diff = 0.0f;
        state.getPressureHistory(hist, h_cnt, p_diff);

        canvas.setFont(&UkrainianFont12Bold);
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

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(trend_col);
        char trend_str[64];
        snprintf(trend_str, sizeof(trend_str), "%s %s (%+.1f mmHg)", arrow_sym, trend_word, p_diff);
        canvas.drawString(trend_str, 120, 96);

        const char* wf = "Стабільна погода";
        if (state.pressure < 742.0f || p_diff <= -1.5f) wf = "Можливі опади / Циклон";
        else if (state.pressure > 760.0f || p_diff >= 1.5f) wf = "Ясно / Антициклон";

        canvas.fillRoundRect(36, 110, 168, 20, 10, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(36, 110, 168, 20, 10, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setFont(&UkrainianFont14);
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

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("750", 192, baseline_y - 4);
    }

    // ВІКНО 5: SCREEN_POMODORO (Hallmark Horology Focus Dial & Timer — 10 FPS)
    inline void drawPomodoro(LGFX_Sprite& canvas, AppState& state, uint32_t now) {
        drawPerimeterTicks(canvas);

        uint16_t rem_sec = state.getPomoRemainingSeconds();
        uint16_t tot_sec = state.pomo_is_break ? ((uint16_t)state.pomo_break_duration_min * 60) : ((uint16_t)state.pomo_duration_min * 60);
        float prog = (tot_sec > 0) ? ((float)rem_sec / (float)tot_sec) : 0.0f;
        prog = constrain(prog, 0.0f, 1.0f);

        // Хорологічна дуга зворотного відліку
        uint16_t arc_col = state.pomo_is_break ? HallmarkTheme::ACCENT_SUCCESS : HallmarkTheme::ACCENT_WARM;
        int deg_end = (int)(prog * 360.0f);
        canvas.drawArc(120, 120, 108, 104, 0, 360, HallmarkTheme::BORDER_HAIRLINE);
        if (deg_end > 0) {
            canvas.drawArc(120, 120, 108, 104, 270, 270 + deg_end, arc_col);
        }

        // Заголовок
        canvas.setFont(&UkrainianFont12Bold);
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
                st_txt = "ПАУЗА | ТОРКНИСЬ КРИШКИ";
                badge_col = HallmarkTheme::ACCENT_COOL;
                text_col = HallmarkTheme::ACCENT_COOL;
            } else if (state.pomo_is_break) {
                st_txt = "ПЕРЕРВА | СМАРАГД";
                badge_col = HallmarkTheme::ACCENT_SUCCESS;
                text_col = HallmarkTheme::ACCENT_SUCCESS;
            } else {
                st_txt = "ФОКУС | БУРШТИН";
                badge_col = HallmarkTheme::ACCENT_WARM;
                text_col = HallmarkTheme::ACCENT_WARM;
            }
        }

        canvas.fillRoundRect(32, 114, 176, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(32, 114, 176, 22, 11, badge_col);
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(text_col);
        canvas.drawString(st_txt, 120, 125);

        // Капсули керування над кнопками
        const char* l_lbl = state.pomo_active ? "◄ -1м" : "◄ -5м";
        canvas.fillRoundRect(22, 148, 58, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(22, 148, 58, 22, 11, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setFont(&UkrainianFont12Bold);
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

        canvas.setFont(&UkrainianFont12Bold);
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
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(gold_col);
        canvas.drawString("B-DAY", 120, 52);

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("З ДНЕМ НАРОДЖЕННЯ,", 120, 78);

        canvas.setFont(&UkrainianFont16Bold);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString("НАСТЮ!", 120, 104);

        canvas.setFont(&UkrainianFont14);
        canvas.setTextColor(gold_col);
        canvas.drawString("05.11 | Твій особливий день", 120, 130);

        canvas.setFont(&UkrainianFont14);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("Тепла, затишку та посмішок", 120, 150);

        canvas.fillRoundRect(70, 168, 100, 24, 12, HallmarkTheme::BG_PRIMARY);
        canvas.drawRoundRect(70, 168, 100, 24, 12, gold_col);
        canvas.setTextColor(gold_col);
        canvas.drawString("ДЯКУЮ", 120, 180);
    }

    // ІНДИКАТОР СТОРІНОК (Y = 222): 3 акуратні крапки (активна: pill 12x4 px бурштин)
    inline void drawPageIndicator(LGFX_Sprite& canvas, AppState& state) {
        if (state.screen_mode >= MAIN_SCREEN_COUNT) return;
        const int dot_xs[3] = {100, 120, 140};
        for (int i = 0; i < 3; ++i) {
            if (state.screen_mode == i) {
                canvas.fillSmoothRoundRect(dot_xs[i] - 6, 220, 12, 4, 2, HallmarkTheme::ACCENT_AMBER);
            } else {
                canvas.fillSmoothCircle(dot_xs[i], 222, 2, HallmarkTheme::TICK_NORMAL);
            }
        }
    }

    // HUD TOAST (Luxury OSD Overlay - 1.5s non-blocking Swiss capsule)
    inline void drawHudToast(LGFX_Sprite& canvas, AppState& state, uint32_t now) {
        if (!state.isHudActive(now)) return;

        int32_t rem = (int32_t)(state.hud_until_ms - now);
        int offset_y = 0;
        if (rem < 200) {
            offset_y = (200 - rem) / 6; // Плавне опускання вниз при зникненні
        }

        int x = 22;
        int y = 146 + offset_y;
        int w = 196;
        int h = 46;
        int r = 16;

        canvas.fillSmoothRoundRect(x, y, w, h, r, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(x, y, w, h, r, HallmarkTheme::BORDER_ACTIVE);

        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setFont(&UkrainianFont14);
        canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
        canvas.drawString(state.hud_title, 120, y + 15);

        if (state.hud_subtitle[0] != '\0') {
            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
            canvas.drawString(state.hud_subtitle, 120, y + 32);
        } else {
            int bar_w = 120;
            int bar_h = 4;
            int bar_x = 120 - bar_w / 2;
            int bar_y = y + 31;
            canvas.fillRoundRect(bar_x, bar_y, bar_w, bar_h, 2, HallmarkTheme::BORDER_HAIRLINE);
            int fill_w = (bar_w * state.manual_brt_pct) / 100;
            if (fill_w > bar_w) fill_w = bar_w;
            if (fill_w > 0) {
                canvas.fillRoundRect(bar_x, bar_y, fill_w, bar_h, 2, HallmarkTheme::ACCENT_WARM);
            }
        }
    }
}
