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

    struct PerimeterTick {
        uint8_t x_in, y_in;
        uint8_t x_out, y_out;
        uint8_t x_dot, y_dot;
        bool is_major;
    };

    static constexpr PerimeterTick TICK_LUT[60] = {
        { 120, 8, 120, 2, 120, 6, true }, // 0
        { 132, 6, 132, 3, 132, 7, false }, // 1
        { 144, 8, 145, 5, 144, 8, false }, // 2
        { 156, 11, 156, 8, 155, 12, false }, // 3
        { 167, 15, 168, 12, 166, 16, false }, // 4
        { 176, 23, 179, 18, 177, 21, true }, // 5
        { 188, 27, 189, 25, 187, 28, false }, // 6
        { 197, 35, 199, 32, 196, 35, false }, // 7
        { 205, 43, 208, 41, 205, 44, false }, // 8
        { 213, 52, 215, 51, 212, 53, false }, // 9
        { 217, 64, 222, 61, 219, 63, true }, // 10
        { 225, 73, 228, 72, 224, 74, false }, // 11
        { 229, 84, 232, 84, 228, 85, false }, // 12
        { 232, 96, 235, 95, 232, 96, false }, // 13
        { 234, 108, 237, 108, 233, 108, false }, // 14
        { 232, 120, 238, 120, 234, 120, true }, // 15
        { 234, 132, 237, 132, 233, 132, false }, // 16
        { 232, 144, 235, 145, 232, 144, false }, // 17
        { 229, 156, 232, 156, 228, 155, false }, // 18
        { 225, 167, 228, 168, 224, 166, false }, // 19
        { 217, 176, 222, 179, 219, 177, true }, // 20
        { 213, 188, 215, 189, 212, 187, false }, // 21
        { 205, 197, 208, 199, 205, 196, false }, // 22
        { 197, 205, 199, 208, 196, 205, false }, // 23
        { 188, 213, 189, 215, 187, 212, false }, // 24
        { 176, 217, 179, 222, 177, 219, true }, // 25
        { 167, 225, 168, 228, 166, 224, false }, // 26
        { 156, 229, 156, 232, 155, 228, false }, // 27
        { 144, 232, 145, 235, 144, 232, false }, // 28
        { 132, 234, 132, 237, 132, 233, false }, // 29
        { 120, 232, 120, 238, 120, 234, true }, // 30
        { 108, 234, 108, 237, 108, 233, false }, // 31
        { 96, 232, 95, 235, 96, 232, false }, // 32
        { 84, 229, 84, 232, 85, 228, false }, // 33
        { 73, 225, 72, 228, 74, 224, false }, // 34
        { 64, 217, 61, 222, 63, 219, true }, // 35
        { 52, 213, 51, 215, 53, 212, false }, // 36
        { 43, 205, 41, 208, 44, 205, false }, // 37
        { 35, 197, 32, 199, 35, 196, false }, // 38
        { 27, 188, 25, 189, 28, 187, false }, // 39
        { 23, 176, 18, 179, 21, 177, true }, // 40
        { 15, 167, 12, 168, 16, 166, false }, // 41
        { 11, 156, 8, 156, 12, 155, false }, // 42
        { 8, 144, 5, 145, 8, 144, false }, // 43
        { 6, 132, 3, 132, 7, 132, false }, // 44
        { 8, 120, 2, 120, 6, 120, true }, // 45
        { 6, 108, 3, 108, 7, 108, false }, // 46
        { 8, 96, 5, 95, 8, 96, false }, // 47
        { 11, 84, 8, 84, 12, 85, false }, // 48
        { 15, 73, 12, 72, 16, 74, false }, // 49
        { 23, 64, 18, 61, 21, 63, true }, // 50
        { 27, 52, 25, 51, 28, 53, false }, // 51
        { 35, 43, 32, 41, 35, 44, false }, // 52
        { 43, 35, 41, 32, 44, 35, false }, // 53
        { 52, 27, 51, 25, 53, 28, false }, // 54
        { 64, 23, 61, 18, 63, 21, true }, // 55
        { 73, 15, 72, 12, 74, 16, false }, // 56
        { 84, 11, 84, 8, 85, 12, false }, // 57
        { 96, 8, 95, 5, 96, 8, false }, // 58
        { 108, 6, 108, 3, 108, 7, false }  // 59
    };

    inline void drawPerimeterTicks(LGFX_Sprite& canvas, int highlight_sec = -1, uint16_t highlight_col = HallmarkTheme::ACCENT_AMBER) {
        for (int i = 0; i < 60; ++i) {
            const auto& pt = TICK_LUT[i];
            uint16_t col = (i == highlight_sec) ? highlight_col :
                           (pt.is_major ? HallmarkTheme::TICK_MAJOR : HallmarkTheme::TICK_NORMAL);
            canvas.drawLine(pt.x_in, pt.y_in, pt.x_out, pt.y_out, col);
            if (i == highlight_sec) {
                canvas.fillSmoothCircle(pt.x_dot, pt.y_dot, 3, highlight_col);
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
        canvas.drawString(Config::FW_VERSION, 120, 116);

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
        char diag_hdr[44];
        snprintf(diag_hdr, sizeof(diag_hdr), "DIAG | %s", Config::FW_VERSION);
        canvas.drawString(diag_hdr, 120, 26);

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

    // ВІКНО 1: SCREEN_CLIMATE (Prestige Horology & Microclimate Dial — 10 FPS)
    inline void drawClimate(LGFX_Sprite& canvas, AppState& state) {
        // 1. Фон циферблата (Два стилі: 0 — Глибокий графіт, 1 — Жива аура)
        uint32_t c_now = millis();
        uint16_t cap_bg = HallmarkTheme::BG_SURFACE;
        uint16_t cap_border = HallmarkTheme::BORDER_HAIRLINE;
        uint16_t aura_accent = HallmarkTheme::ACCENT_AMBER;
        uint16_t date_col = HallmarkTheme::ACCENT_AMBER;
        uint16_t sec_col = HallmarkTheme::ACCENT_AMBER;
        uint16_t room_tag_col = HallmarkTheme::TEXT_SECONDARY;

        if (state.climate_bg_style == 1) {
            // Стиль 1: Жива аура (м'яке дихання в тон Spotify або поточної атмосфери світла)
            float breathe = (sinf((float)c_now * 0.002f) + 1.0f) * 0.5f;
            uint8_t base_r = state.ambi_r, base_g = state.ambi_g, base_b = state.ambi_b;
            if (base_r == 0 && base_g == 0 && base_b == 0) {
                base_r = 54; base_g = 48; base_b = 44;
            }
            uint8_t bg_r = (uint8_t)(base_r * 0.12f);
            uint8_t bg_g = (uint8_t)(base_g * 0.12f);
            uint8_t bg_b = (uint8_t)(base_b * 0.12f);
            canvas.fillScreen(canvas.color565(bg_r, bg_g, bg_b));

            // Концентричний оксамитовий ореол від центру (120, 120)
            for (int ring = 4; ring >= 1; ring--) {
                int r_px = 30 + ring * 22 + (int)(breathe * 6.0f);
                float f = (0.16f + (float)(5 - ring) * 0.05f) * (0.85f + breathe * 0.3f);
                uint8_t rr = (uint8_t)constrain(base_r * f, 0.0f, 255.0f);
                uint8_t gg = (uint8_t)constrain(base_g * f, 0.0f, 255.0f);
                uint8_t bb = (uint8_t)constrain(base_b * f, 0.0f, 255.0f);
                canvas.fillCircle(120, 120, r_px, canvas.color565(rr, gg, bb));
            }

            // Матові напівпрозорі капсули в тон живої аури
            cap_bg = canvas.color565((uint8_t)(base_r * 0.22f), (uint8_t)(base_g * 0.22f), (uint8_t)(base_b * 0.22f));
            cap_border = canvas.color565((uint8_t)constrain(base_r * 0.55f, 0.0f, 255.0f), 
                                        (uint8_t)constrain(base_g * 0.55f, 0.0f, 255.0f), 
                                        (uint8_t)constrain(base_b * 0.55f, 0.0f, 255.0f));

            // Світловий гармонійний акцент аури для дати, секундної мітки та бейджів
            uint8_t ar = (uint8_t)constrain(base_r * 0.85f + 70.0f, 0.0f, 255.0f);
            uint8_t ag = (uint8_t)constrain(base_g * 0.85f + 70.0f, 0.0f, 255.0f);
            uint8_t ab = (uint8_t)constrain(base_b * 0.85f + 70.0f, 0.0f, 255.0f);
            aura_accent = canvas.color565(ar, ag, ab);
            date_col = aura_accent;
            sec_col = aura_accent;
            room_tag_col = aura_accent;
        } else {
            // Стиль 0: Глибокий швейцарський графіт (максимальний контраст і читабельність)
            canvas.fillScreen(canvas.color565(12, 14, 18));
        }

        // 2. Годинникові засічки по периметру (Радіус 112..118 px) + адаптивна секундна мітка
        int sec = 0;
        if (state.time_synced) {
            time_t now_t = time(nullptr);
            struct tm* t_m = localtime(&now_t);
            if (t_m) sec = t_m->tm_sec;
        }
        drawPerimeterTicks(canvas, sec, sec_col);

        // ====================================================================
        // ЯРУС 1: ВЕРХНІЙ КУПОЛ (Y = 36..56) — ЧАС ТА ДАТА
        // ====================================================================
        char t_buf[16];
        if (state.time_synced) {
            state.getTimeString(t_buf, sizeof(t_buf));
        } else {
            strncpy(t_buf, "--:--", sizeof(t_buf));
        }
        canvas.setFont(&fonts::FreeSansBold18pt7b);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(t_buf, 120, 38);

        // Дата під часом (Українська локалізація: "СБ, 7 ЛИС")
        char date_buf[28] = "";
        if (state.time_synced) {
            time_t now_t = time(nullptr);
            struct tm* tm_info = localtime(&now_t);
            if (tm_info) {
                static const char* const W_DAYS[] = {"НД", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"};
                static const char* const MONTHS[] = {"СІЧ", "ЛЮТ", "БЕР", "КВІ", "ТРА", "ЧЕР", "ЛИП", "СЕР", "ВЕР", "ЖОВ", "ЛИС", "ГРУ"};
                int wd = constrain(tm_info->tm_wday, 0, 6);
                int mo = constrain(tm_info->tm_mon, 0, 11);
                snprintf(date_buf, sizeof(date_buf), "%s, %d %s", W_DAYS[wd], tm_info->tm_mday, MONTHS[mo]);
            }
        }
        if (date_buf[0] == '\0') {
            strncpy(date_buf, "SMARTLAMP", sizeof(date_buf));
        }
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(date_col);
        canvas.drawString(date_buf, 120, 58);

        // ====================================================================
        // ЯРУС 2: ЦЕНТРАЛЬНИЙ HERO-БЛОК — ТЕМПЕРАТУРА В КІМНАТІ (Y = 96..128)
        // Строго по центру циферблата (X = 120), без відсутніх гліфів!
        // ====================================================================
        float temp_val = state.temperature;
        int t_int = (int)temp_val;
        int t_dec = abs((int)(temp_val * 10.0f) - t_int * 10);
        char num_buf[12];
        snprintf(num_buf, sizeof(num_buf), "%d.%d", t_int, t_dec);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        int w_num = canvas.textWidth(num_buf);
        int deg_r = 3;
        int deg_gap = 4;
        canvas.setFont(&fonts::FreeSansBold18pt7b);
        int w_c = canvas.textWidth("C");

        int total_w = w_num + deg_gap + (deg_r * 2) + deg_gap + w_c;
        int start_x = 120 - total_w / 2;
        int base_y = 104;

        // Число температури
        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(num_buf, start_x, base_y);

        // Коло градуса (акуратне векторне коло замість відсутнього шрифтового квадрата [])
        int deg_cx = start_x + w_num + deg_gap + deg_r;
        int deg_cy = base_y - 12;
        canvas.drawCircle(deg_cx, deg_cy, deg_r, HallmarkTheme::TEXT_PRIMARY);
        canvas.drawCircle(deg_cx, deg_cy, deg_r - 1, HallmarkTheme::TEXT_PRIMARY);

        // Літера C
        canvas.setFont(&fonts::FreeSansBold18pt7b);
        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString("C", deg_cx + deg_r + deg_gap, base_y);

        // Мікро-бейдж "КІМНАТА" строго по центру під температурою
        canvas.fillRoundRect(92, 122, 56, 16, 8, cap_bg);
        canvas.drawRoundRect(92, 122, 56, 16, 8, cap_border);
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(room_tag_col);
        canvas.drawString("КІМНАТА", 120, 130);

        // ====================================================================
        // ЯРУС 3: ДВІ СИМЕТРИЧНІ ПРЕСТИЖ-КАПСУЛИ (Y = 148..178, Висота 30 px)
        // Вологість та тиск крупні, солідні, чітко читаються здалеку!
        // ====================================================================

        // Ліва капсула: Вологість (X = 22..116, ширина 94, висота 30)
        canvas.fillSmoothRoundRect(22, 148, 94, 30, 10, cap_bg);
        canvas.drawRoundRect(22, 148, 94, 30, 10, cap_border);

        // Іконка-крапелька
        int cx = 38, cy = 163;
        canvas.fillCircle(cx, cy + 2, 3, HallmarkTheme::ACCENT_COOL);
        canvas.fillTriangle(cx - 3, cy + 2, cx + 3, cy + 2, cx, cy - 4, HallmarkTheme::ACCENT_COOL);

        char h_str[12];
        snprintf(h_str, sizeof(h_str), "%.0f%%", state.humidity);
        canvas.setFont(&UkrainianFont16Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::ACCENT_COOL);
        canvas.drawString(h_str, 77, 163);

        // Права капсула: Атмосферний тиск (X = 124..218, ширина 94, висота 30)
        canvas.fillSmoothRoundRect(124, 148, 94, 30, 10, cap_bg);
        canvas.drawRoundRect(124, 148, 94, 30, 10, cap_border);

        uint16_t p_col = (state.pressure_diff >= 0.5f) ? HallmarkTheme::ACCENT_SUCCESS :
                         ((state.pressure_diff <= -0.5f) ? HallmarkTheme::ACCENT_DANGER : 
                          ((state.climate_bg_style == 1) ? aura_accent : HallmarkTheme::ACCENT_AMBER));

        // Стрілка тренду
        int ax = 138, ay = 163;
        if (state.pressure_diff > 0.35f) {
            canvas.drawLine(ax, ay + 4, ax, ay - 4, p_col);
            canvas.drawLine(ax - 2, ay - 1, ax, ay - 4, p_col);
            canvas.drawLine(ax + 2, ay - 1, ax, ay - 4, p_col);
        } else if (state.pressure_diff < -0.35f) {
            canvas.drawLine(ax, ay - 4, ax, ay + 4, p_col);
            canvas.drawLine(ax - 2, ay + 1, ax, ay + 4, p_col);
            canvas.drawLine(ax + 2, ay + 1, ax, ay + 4, p_col);
        } else {
            canvas.drawLine(ax - 3, ay, ax + 3, ay, p_col);
            canvas.drawLine(ax + 1, ay - 2, ax + 3, ay, p_col);
            canvas.drawLine(ax + 1, ay + 2, ax + 3, ay, p_col);
        }

        char p_str[16];
        snprintf(p_str, sizeof(p_str), "%.0f мм", state.pressure);
        canvas.setFont(&UkrainianFont16Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(p_col);
        canvas.drawString(p_str, 177, 163);

        // ====================================================================
        // ЯРУС 4: ПОГОДА НА ВУЛИЦІ (Y = 184..208, Престиж-капсула погоди)
        // Елегантна напівпрозора капсула з адаптивним термо-кольором температури!
        // ====================================================================
        String w_desc = state.outdoor_desc;
        w_desc.trim();
        w_desc.replace("☀️", ""); w_desc.replace("🌤", ""); w_desc.replace("⛅", "");
        w_desc.replace("☁️", ""); w_desc.replace("🌧", ""); w_desc.replace("🌨", "");
        w_desc.replace("⛈", ""); w_desc.replace("🌫", "");
        w_desc.trim();

        String short_desc = "Хмарно";
        if (w_desc.length() > 0) {
            String low = w_desc;
            low.toLowerCase();
            if (low.indexOf("хмарн") >= 0 || low.indexOf("похмур") >= 0) short_desc = "Хмарно";
            else if (low.indexOf("ясн") >= 0) short_desc = "Ясно";
            else if (low.indexOf("злив") >= 0) short_desc = "Злива";
            else if (low.indexOf("дощ") >= 0 || low.indexOf("мряк") >= 0) short_desc = "Дощ";
            else if (low.indexOf("сніг") >= 0) short_desc = "Сніг";
            else if (low.indexOf("туман") >= 0 || low.indexOf("памороз") >= 0) short_desc = "Туман";
            else if (low.indexOf("гроз") >= 0) short_desc = "Гроза";
            else short_desc = w_desc;
        }

        // Адаптивний колір для вуличної температури за рівнем тепла
        uint16_t out_t_col = HallmarkTheme::TEXT_PRIMARY;
        if (state.outdoor_temp < 900.0f) {
            if (state.outdoor_temp <= 0.0f) {
                out_t_col = canvas.color565(130, 205, 255); // Мороз / крижаний блакитний
            } else if (state.outdoor_temp < 15.0f) {
                out_t_col = canvas.color565(195, 225, 255); // Прохолодно / чисте сріблясте небо
            } else if (state.outdoor_temp < 25.0f) {
                out_t_col = HallmarkTheme::ACCENT_SUCCESS;  // Комфорт / весняна зелень
            } else {
                out_t_col = HallmarkTheme::ACCENT_AMBER;    // Тепло / бурштиновий
            }
        }

        // Нижня погодна капсула (X = 45..195, ширина 150, висота 24, радіус 12)
        canvas.fillSmoothRoundRect(45, 184, 150, 24, 12, cap_bg);
        canvas.drawRoundRect(45, 184, 150, 24, 12, cap_border);

        char out_cap_txt[48];
        if (state.outdoor_temp < 900.0f) {
            snprintf(out_cap_txt, sizeof(out_cap_txt), "Вулиця: %+.0f°C · %s", state.outdoor_temp, short_desc.c_str());
        } else {
            snprintf(out_cap_txt, sizeof(out_cap_txt), "Вулиця: +2°C · %s", short_desc.c_str());
        }

        canvas.setFont(&UkrainianFont12Bold);
        if (canvas.textWidth(out_cap_txt) > 138) {
            snprintf(out_cap_txt, sizeof(out_cap_txt), "%+.0f°C · %s", state.outdoor_temp, short_desc.c_str());
        }

        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(out_t_col);
        canvas.drawString(out_cap_txt, 120, 196);
    }

    // ВІКНО 2: SCREEN_LIVING_GIF    // ВІКНО 2: SCREEN_LIVING_GIF (Постійна GIF-Заставка LittleFS + Процедурний маятник Hallmark)
    inline void drawLivingGif(LGFX_Sprite& canvas, AppState& state, uint32_t now) {
        bool has_frames = (state.saver_loaded && state.saver_num_frames > 0) || (state.anim_frames_count > 0);
        if (!has_frames) {
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

        // Оверлей годинника поверх заставки з підтримкою 3 режимів (saver_clock_mode)
        if (state.saver_show_clock) {
            char clk_buf[32];
            char t_str[10];
            state.getTimeString(t_str, sizeof(t_str));

            if (state.saver_clock_mode == 0) {
                // Стиль 0: Капсула вгорі
                canvas.fillRoundRect(50, 18, 140, 22, 11, HallmarkTheme::BG_PRIMARY);
                canvas.drawRoundRect(50, 18, 140, 22, 11, HallmarkTheme::BORDER_HAIRLINE);
                snprintf(clk_buf, sizeof(clk_buf), "%s | %.1f°C", t_str, state.temperature);
                canvas.setFont(&UkrainianFont14);
                canvas.setTextDatum(textdatum_t::middle_center);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString(clk_buf, 120, 29);
            } else if (state.saver_clock_mode == 1) {
                // Стиль 1: Напівпрозора капсула внизу (не закриває обличчя та арт персонажів)
                canvas.fillRoundRect(80, 188, 80, 20, 10, HallmarkTheme::BG_SURFACE);
                canvas.drawRoundRect(80, 188, 80, 20, 10, HallmarkTheme::BORDER_HAIRLINE);
                snprintf(clk_buf, sizeof(clk_buf), "%s", t_str);
                canvas.setFont(&UkrainianFont14);
                canvas.setTextDatum(textdatum_t::middle_center);
                canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
                canvas.drawString(clk_buf, 120, 198);
            }
            // Стиль 2: Full Art (лише чиста анімація без тексту)
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
    inline void drawSmoothMarquee(
        LGFX_Sprite& canvas,
        const char* text,
        int center_x,
        int y,
        int max_w,
        uint16_t color,
        const GFXfont* font,
        uint32_t now,
        uint32_t pause_start_ms = 2200,
        uint32_t pause_end_ms = 1500,
        float px_per_sec = 28.0f
    ) {
        if (!text || text[0] == '\0') return;
        canvas.setFont(font);
        canvas.setTextColor(color);

        int text_w = canvas.textWidth(text);
        if (text_w <= max_w) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.drawString(text, center_x, y);
            return;
        }

        int overflow = text_w - max_w;
        uint32_t scroll_duration_ms = (uint32_t)((float)overflow / px_per_sec * 1000.0f);
        if (scroll_duration_ms < 600) scroll_duration_ms = 600;
        uint32_t return_duration_ms = (uint32_t)((float)overflow / 45.0f * 1000.0f);
        if (return_duration_ms < 400) return_duration_ms = 400;

        uint32_t total_cycle_ms = pause_start_ms + scroll_duration_ms + pause_end_ms + return_duration_ms;

        static uint32_t s_last_hash[2] = {0, 0};
        static uint32_t s_start_time[2] = {0, 0};
        int slot = (y >= 185) ? 1 : 0;
        uint32_t h = 5381;
        for (const char* p = text; *p; ++p) {
            h = ((h << 5) + h) + (uint8_t)(*p);
        }
        if (s_last_hash[slot] != h) {
            s_last_hash[slot] = h;
            s_start_time[slot] = now;
        }

        uint32_t elapsed = now - s_start_time[slot];
        uint32_t cycle_t = elapsed % total_cycle_ms;
        float offset = 0.0f;

        if (cycle_t < pause_start_ms) {
            offset = 0.0f;
        } else if (cycle_t < (pause_start_ms + scroll_duration_ms)) {
            // Суворо лінійна, рівномірна швидкість (пікселі за секунду) без ривків!
            float progress = (float)(cycle_t - pause_start_ms) / (float)scroll_duration_ms;
            offset = progress * (float)overflow;
        } else if (cycle_t < (pause_start_ms + scroll_duration_ms + pause_end_ms)) {
            offset = (float)overflow;
        } else {
            uint32_t return_t = cycle_t - (pause_start_ms + scroll_duration_ms + pause_end_ms);
            float progress = (float)return_t / (float)return_duration_ms;
            offset = (1.0f - progress) * (float)overflow;
        }

        int clip_x = center_x - (max_w / 2);
        int base_x = clip_x - (int)offset;

        canvas.setClipRect(clip_x, y - 12, max_w, 24);
        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.drawString(text, base_x, y);
        canvas.clearClipRect();
    }

    inline void drawSmartHub(
        LGFX_Sprite& canvas,
        LGFX_Sprite& vinylSprite,
        LGFX_Sprite& bgSprite,
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

        // --- ПРІОРИТЕТ 2: Spotify (Hallmark Luxury Vinyl Deck — 30 FPS) ---
        // Екран 3 завжди залишається Spotify Deck (навіть при паузі чи без пісні)
        bool has_spotify_deck = true;
        if (has_spotify_deck) {
            // Преміальний «Теплий графіт» за замовчуванням (Warm Graphite)
            static constexpr float GRAPHITE_R = 54.0f;
            static constexpr float GRAPHITE_G = 48.0f;
            static constexpr float GRAPHITE_B = 44.0f;
            static constexpr float GRAPHITE_R2 = 38.0f;
            static constexpr float GRAPHITE_G2 = 34.0f;
            static constexpr float GRAPHITE_B2 = 32.0f;
            static constexpr float GRAPHITE_R3 = 78.0f;
            static constexpr float GRAPHITE_G3 = 66.0f;
            static constexpr float GRAPHITE_B3 = 54.0f;

            static float s_lerp_r = GRAPHITE_R, s_lerp_g = GRAPHITE_G, s_lerp_b = GRAPHITE_B;
            static float s_lerp_r2 = GRAPHITE_R2, s_lerp_g2 = GRAPHITE_G2, s_lerp_b2 = GRAPHITE_B2;
            static float s_lerp_r3 = GRAPHITE_R3, s_lerp_g3 = GRAPHITE_G3, s_lerp_b3 = GRAPHITE_B3;

            // Під час перемикання треків (isSwitchingOptimistic) утримуємо попередній колір,
            // не скидаючи в дефолт, доки не надійдуть нові дані обкладинки
            static float s_hold_target_r = GRAPHITE_R;
            static float s_hold_target_g = GRAPHITE_G;
            static float s_hold_target_b = GRAPHITE_B;
            static float s_hold_target_r2 = GRAPHITE_R2;
            static float s_hold_target_g2 = GRAPHITE_G2;
            static float s_hold_target_b2 = GRAPHITE_B2;
            static float s_hold_target_r3 = GRAPHITE_R3;
            static float s_hold_target_g3 = GRAPHITE_G3;
            static float s_hold_target_b3 = GRAPHITE_B3;

            bool has_cover_colors = (state.spotify.ambi_r > 0 || state.spotify.ambi_g > 0 || state.spotify.ambi_b > 0);
            if (has_cover_colors) {
                s_hold_target_r = (float)state.spotify.ambi_r;
                s_hold_target_g = (float)state.spotify.ambi_g;
                s_hold_target_b = (float)state.spotify.ambi_b;

                s_hold_target_r2 = (state.spotify.ambi2_r > 0 || state.spotify.ambi2_g > 0 || state.spotify.ambi2_b > 0) ? (float)state.spotify.ambi2_r : (s_hold_target_r * 0.7f);
                s_hold_target_g2 = (state.spotify.ambi2_r > 0 || state.spotify.ambi2_g > 0 || state.spotify.ambi2_g > 0) ? (float)state.spotify.ambi2_g : (s_hold_target_g * 0.7f);
                s_hold_target_b2 = (state.spotify.ambi2_r > 0 || state.spotify.ambi2_g > 0 || state.spotify.ambi2_b > 0) ? (float)state.spotify.ambi2_b : (s_hold_target_b * 0.7f);

                s_hold_target_r3 = (state.spotify.ambi3_r > 0 || state.spotify.ambi3_g > 0 || state.spotify.ambi3_b > 0) ? (float)state.spotify.ambi3_r : (s_hold_target_r * 0.85f);
                s_hold_target_g3 = (state.spotify.ambi3_r > 0 || state.spotify.ambi3_g > 0 || state.spotify.ambi3_b > 0) ? (float)state.spotify.ambi3_g : (s_hold_target_g * 0.85f);
                s_hold_target_b3 = (state.spotify.ambi3_r > 0 || state.spotify.ambi3_g > 0 || state.spotify.ambi3_b > 0) ? (float)state.spotify.ambi3_b : (s_hold_target_b * 0.85f);
            } else if (!state.spotify.isSwitchingOptimistic(now) && state.spotify.track_id[0] == '\0') {
                s_hold_target_r = GRAPHITE_R; s_hold_target_g = GRAPHITE_G; s_hold_target_b = GRAPHITE_B;
                s_hold_target_r2 = GRAPHITE_R2; s_hold_target_g2 = GRAPHITE_G2; s_hold_target_b2 = GRAPHITE_B2;
                s_hold_target_r3 = GRAPHITE_R3; s_hold_target_g3 = GRAPHITE_G3; s_hold_target_b3 = GRAPHITE_B3;
            }

            float target_r = s_hold_target_r;
            float target_g = s_hold_target_g;
            float target_b = s_hold_target_b;
            float target_r2 = s_hold_target_r2;
            float target_g2 = s_hold_target_g2;
            float target_b2 = s_hold_target_b2;
            float target_r3 = s_hold_target_r3;
            float target_g3 = s_hold_target_g3;
            float target_b3 = s_hold_target_b3;

            // Спокійна аналогова інтерполяція кольору (0.10) без різких стрибків
            s_lerp_r += (target_r - s_lerp_r) * 0.10f;
            s_lerp_g += (target_g - s_lerp_g) * 0.10f;
            s_lerp_b += (target_b - s_lerp_b) * 0.10f;

            s_lerp_r2 += (target_r2 - s_lerp_r2) * 0.10f;
            s_lerp_g2 += (target_g2 - s_lerp_g2) * 0.10f;
            s_lerp_b2 += (target_b2 - s_lerp_b2) * 0.10f;

            s_lerp_r3 += (target_r3 - s_lerp_r3) * 0.10f;
            s_lerp_g3 += (target_g3 - s_lerp_g3) * 0.10f;
            s_lerp_b3 += (target_b3 - s_lerp_b3) * 0.10f;

            // Точне осідання на цільових кольорах (snap < 0.8) без «застрягання»
            if (fabsf(target_r - s_lerp_r) < 0.8f) s_lerp_r = target_r;
            if (fabsf(target_g - s_lerp_g) < 0.8f) s_lerp_g = target_g;
            if (fabsf(target_b - s_lerp_b) < 0.8f) s_lerp_b = target_b;
            if (fabsf(target_r2 - s_lerp_r2) < 0.8f) s_lerp_r2 = target_r2;
            if (fabsf(target_g2 - s_lerp_g2) < 0.8f) s_lerp_g2 = target_g2;
            if (fabsf(target_b2 - s_lerp_b2) < 0.8f) s_lerp_b2 = target_b2;
            if (fabsf(target_r3 - s_lerp_r3) < 0.8f) s_lerp_r3 = target_r3;
            if (fabsf(target_g3 - s_lerp_g3) < 0.8f) s_lerp_g3 = target_g3;
            if (fabsf(target_b3 - s_lerp_b3) < 0.8f) s_lerp_b3 = target_b3;

            uint8_t cur_r = (uint8_t)constrain(s_lerp_r, 0.0f, 255.0f);
            uint8_t cur_g = (uint8_t)constrain(s_lerp_g, 0.0f, 255.0f);
            uint8_t cur_b = (uint8_t)constrain(s_lerp_b, 0.0f, 255.0f);

            uint8_t cur_r2 = (uint8_t)constrain(s_lerp_r2, 0.0f, 255.0f);
            uint8_t cur_g2 = (uint8_t)constrain(s_lerp_g2, 0.0f, 255.0f);
            uint8_t cur_b2 = (uint8_t)constrain(s_lerp_b2, 0.0f, 255.0f);

            uint8_t cur_r3 = (uint8_t)constrain(s_lerp_r3, 0.0f, 255.0f);
            uint8_t cur_g3 = (uint8_t)constrain(s_lerp_g3, 0.0f, 255.0f);
            uint8_t cur_b3 = (uint8_t)constrain(s_lerp_b3, 0.0f, 255.0f);

            uint16_t primary_col = canvas.color565(cur_r, cur_g, cur_b);
            uint16_t secondary_col = (cur_r2 > 25 || cur_g2 > 25 || cur_b2 > 25) ? canvas.color565(cur_r2, cur_g2, cur_b2) : primary_col;

            if (state.spotify_bg_style == 1) {
                // Стиль 1: «Глибокий чорний» (Obsidian Dark v6.8)
                uint16_t bg_base_col = canvas.color565((uint8_t)(cur_r * 0.12f), (uint8_t)(cur_g * 0.12f), (uint8_t)(cur_b * 0.12f));
                canvas.fillScreen(bg_base_col);
                for (int ring = 8; ring >= 1; ring--) {
                    int r_px = 68 + ring * 7;
                    float frac = 0.12f + (float)(9 - ring) * 0.045f;
                    uint8_t rr = (uint8_t)constrain((cur_r * 0.72f + cur_r2 * 0.28f) * frac, 0.0f, 255.0f);
                    uint8_t gg = (uint8_t)constrain((cur_g * 0.72f + cur_g2 * 0.28f) * frac, 0.0f, 255.0f);
                    uint8_t bb = (uint8_t)constrain((cur_b * 0.72f + cur_b2 * 0.28f) * frac, 0.0f, 255.0f);
                    canvas.fillCircle(120, 84, r_px, canvas.color565(rr, gg, bb));
                }
            } else {
                // Стиль 0: «Кольорова аура» (Full Cover Aura v6.9 — Precomputed PSRAM LUT)
                // Таблиця ореолу (sqrtf + 3-кольорові ваги + м'яке віньєтування під текстом + Bayer 4x4)
                // обчислюється ОДИН РАЗ у PSRAM при старті (0 sqrtf у рендер-циклі, < 1 мс на оновлення, 0 ризику WDT!).
                // Запис у bgSprite виконується із явною перестановкою байтів __builtin_bswap16 (big-endian swap565 LovyanGFX),
                // що назавжди виключає кольоровий шум та спотворення каналів.
                struct HaloLutCell {
                    uint8_t w1;
                    uint8_t w2;
                    uint8_t w3;
                    int8_t  dither;
                };
                static HaloLutCell* s_halo_lut = nullptr;
                if (!s_halo_lut) {
                    s_halo_lut = (HaloLutCell*)ps_malloc(240 * 240 * sizeof(HaloLutCell));
                    if (s_halo_lut) {
                        static const uint8_t BAYER_4X4[4][4] = {
                            {  0,  8,  2, 10 },
                            { 12,  4, 14,  6 },
                            {  3, 11,  1,  9 },
                            { 15,  7, 13,  5 }
                        };
                        for (int y = 0; y < 240; y++) {
                            int dy_scr = y - 120;
                            int dy2_scr = dy_scr * dy_scr;
                            int v_dy = y - 84;
                            int v_dy2 = v_dy * v_dy;
                            float v_ratio = (float)y * (1.0f / 239.0f);

                            // М'яка напівпрозора тіньова підкладка під текстом (Y = 152..198) без різких чорних рамок
                            float text_scrim_y = 0.0f;
                            if (y >= 152 && y <= 198) {
                                float ty = (float)(y - 152) / 46.0f;
                                text_scrim_y = sinf(ty * 3.14159265f);
                            }

                            for (int x = 0; x < 240; x++) {
                                int idx = y * 240 + x;
                                int dx = x - 120;
                                int r2_scr = dx * dx + dy2_scr;
                                int v_r2 = dx * dx + v_dy2;

                                if (r2_scr >= 14400 || v_r2 <= 4225) {
                                    s_halo_lut[idx] = { 0, 0, 0, 0 };
                                    continue;
                                }

                                float v_dist = sqrtf((float)v_r2);
                                float fade = constrain((v_dist - 66.0f) * (1.0f / 88.0f), 0.0f, 1.0f);
                                // Насичений оксамитовий ореол: від 82% яскравості біля платівки до 42% на краях екрана
                                float halo = 0.82f - fade * 0.40f;

                                if (text_scrim_y > 0.0f && x >= 16 && x <= 224) {
                                    float tx = (float)(x - 16) / 208.0f;
                                    float text_scrim_x = sinf(tx * 3.14159265f);
                                    halo *= (1.0f - 0.46f * text_scrim_y * text_scrim_x);
                                }

                                float u = (float)x * (1.0f / 239.0f);
                                float f1 = (1.0f - u) * (1.0f - v_ratio * 0.45f);
                                float f2 = u * (1.0f - v_ratio * 0.45f);
                                float f3 = v_ratio * 0.45f;

                                s_halo_lut[idx].w1 = (uint8_t)constrain(f1 * halo * 255.0f, 0.0f, 255.0f);
                                s_halo_lut[idx].w2 = (uint8_t)constrain(f2 * halo * 255.0f, 0.0f, 255.0f);
                                s_halo_lut[idx].w3 = (uint8_t)constrain(f3 * halo * 255.0f, 0.0f, 255.0f);
                                s_halo_lut[idx].dither = (int8_t)(((int)BAYER_4X4[y & 3][x & 3] - 8) >> 1);
                            }
                        }
                    }
                }

                static uint8_t s_last_r = 0, s_last_g = 0, s_last_b = 0;
                static uint8_t s_last_r2 = 0, s_last_g2 = 0, s_last_b2 = 0;
                static uint8_t s_last_r3 = 0, s_last_g3 = 0, s_last_b3 = 0;
                static bool s_bg_inited = false;

                int color_delta = abs((int)cur_r - (int)s_last_r) + abs((int)cur_g - (int)s_last_g) + abs((int)cur_b - (int)s_last_b) +
                                  abs((int)cur_r2 - (int)s_last_r2) + abs((int)cur_g2 - (int)s_last_g2) + abs((int)cur_b2 - (int)s_last_b2) +
                                  abs((int)cur_r3 - (int)s_last_r3) + abs((int)cur_g3 - (int)s_last_g3) + abs((int)cur_b3 - (int)s_last_b3);

                if (!s_bg_inited || color_delta >= 1 || state.spotify_bg_dirty) {
                    s_bg_inited = true;
                    state.spotify_bg_dirty = false;
                    s_last_r = cur_r; s_last_g = cur_g; s_last_b = cur_b;
                    s_last_r2 = cur_r2; s_last_g2 = cur_g2; s_last_b2 = cur_b2;
                    s_last_r3 = cur_r3; s_last_g3 = cur_g3; s_last_b3 = cur_b3;

                    uint16_t* p_buf = (uint16_t*)bgSprite.getBuffer();
                    if (p_buf && s_halo_lut) {
                        for (int i = 0; i < 57600; i++) {
                            const HaloLutCell& c = s_halo_lut[i];
                            if ((c.w1 | c.w2 | c.w3) == 0) {
                                p_buf[i] = 0x0000;
                                continue;
                            }
                            int d = c.dither;
                            int rb = (((int)cur_r * c.w1 + (int)cur_r2 * c.w2 + (int)cur_r3 * c.w3) >> 8) + d;
                            int gb = (((int)cur_g * c.w1 + (int)cur_g2 * c.w2 + (int)cur_g3 * c.w3) >> 8) + (d >> 1);
                            int bb = (((int)cur_b * c.w1 + (int)cur_b2 * c.w2 + (int)cur_b3 * c.w3) >> 8) + d;

                            uint8_t r8 = (rb < 0) ? 0 : ((rb > 255) ? 255 : (uint8_t)rb);
                            uint8_t g8 = (gb < 0) ? 0 : ((gb > 255) ? 255 : (uint8_t)gb);
                            uint8_t b8 = (bb < 0) ? 0 : ((bb > 255) ? 255 : (uint8_t)bb);

                            uint16_t rgb565 = ((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3);
                            p_buf[i] = __builtin_bswap16(rgb565);
                        }
                    }
                }
                bgSprite.pushSprite(&canvas, 0, 0);
            }

            // 1. Динамічний орбітальний спектро-еквалайзер (40 радіальних барів навколо платівки)
            // Попередньо обчислена тригонометрична таблиця (LUT) для нульового навантаження на CPU
            struct EqTrigPoint { float cos_a; float sin_a; float rad; };
            static constexpr EqTrigPoint EQ_LUT[40] = {
                { 1.000000f,  0.000000f, 0.000000f }, { 0.987688f,  0.156434f, 0.157080f },
                { 0.951057f,  0.309017f, 0.314159f }, { 0.891007f,  0.453990f, 0.471239f },
                { 0.809017f,  0.587785f, 0.628319f }, { 0.707107f,  0.707107f, 0.785398f },
                { 0.587785f,  0.809017f, 0.942478f }, { 0.453990f,  0.891007f, 1.099557f },
                { 0.309017f,  0.951057f, 1.256637f }, { 0.156434f,  0.987688f, 1.413717f },
                { 0.000000f,  1.000000f, 1.570796f }, {-0.156434f,  0.987688f, 1.727876f },
                {-0.309017f,  0.951057f, 1.884956f }, {-0.453990f,  0.891007f, 2.042035f },
                {-0.587785f,  0.809017f, 2.199115f }, {-0.707107f,  0.707107f, 2.356194f },
                {-0.809017f,  0.587785f, 2.513274f }, {-0.891007f,  0.453990f, 2.670354f },
                {-0.951057f,  0.309017f, 2.827433f }, {-0.987688f,  0.156434f, 2.984513f },
                {-1.000000f,  0.000000f, 3.141593f }, {-0.987688f, -0.156434f, 3.298672f },
                {-0.951057f, -0.309017f, 3.455752f }, {-0.891007f, -0.453990f, 3.612832f },
                {-0.809017f, -0.587785f, 3.769911f }, {-0.707107f, -0.707107f, 3.926991f },
                {-0.587785f, -0.809017f, 4.084070f }, {-0.453990f, -0.891007f, 4.241150f },
                {-0.309017f, -0.951057f, 4.398230f }, {-0.156434f, -0.987688f, 4.555309f },
                { 0.000000f, -1.000000f, 4.712389f }, { 0.156434f, -0.987688f, 4.869469f },
                { 0.309017f, -0.951057f, 5.026548f }, { 0.453990f, -0.891007f, 5.183628f },
                { 0.587785f, -0.809017f, 5.340708f }, { 0.707107f, -0.707107f, 5.497787f },
                { 0.809017f, -0.587785f, 5.654867f }, { 0.891007f, -0.453990f, 5.811946f },
                { 0.951057f, -0.309017f, 5.969026f }, { 0.987688f, -0.156434f, 6.126106f }
            };

            float bpm_speed = (state.spotify.bpm >= 60 && state.spotify.bpm <= 200) ? (state.spotify.bpm / 120.0f) : 1.0f;
            float energy_boost = (state.spotify.energy > 0) ? (0.6f + (float)state.spotify.energy * 0.008f) : 1.0f;

            uint16_t dim_primary = canvas.color565((uint8_t)(cur_r * 0.22f), (uint8_t)(cur_g * 0.22f), (uint8_t)(cur_b * 0.22f));

            for (int i = 0; i < 40; i++) {
                float cos_a = EQ_LUT[i].cos_a;
                float sin_a = EQ_LUT[i].sin_a;
                float rad = EQ_LUT[i].rad;

                float wave1 = sinf(rad * 3.0f + (float)now * 0.007f * bpm_speed);
                float wave2 = cosf(rad * 5.0f - (float)now * 0.005f);
                float intensity = constrain((wave1 * 0.55f + wave2 * 0.45f + 1.0f) * 0.5f, 0.0f, 1.0f);

                int bar_len = state.spotify.playing ? (int)(intensity * 7.5f * energy_boost) + 2 : 1;
                bar_len = constrain(bar_len, 1, 10);

                int r_in = 74;
                int r_out = r_in + bar_len;

                int x1 = 120 + (int)(cos_a * (float)r_in);
                int y1 = 84  + (int)(sin_a * (float)r_in);
                int x2 = 120 + (int)(cos_a * (float)r_out);
                int y2 = 84  + (int)(sin_a * (float)r_out);

                uint16_t b_col;
                if (!state.spotify.playing) {
                    b_col = dim_primary;
                } else if (intensity > 0.62f) {
                    b_col = secondary_col;
                } else {
                    b_col = primary_col;
                }

                canvas.drawLine(x1, y1, x2, y2, b_col);
            }

            // 2. Обертання вінілової платівки 136x136 px у центрі (X = 120, Y = 84)
            // Плавне обертання за фізичним часом (dt): ~108 град/с (~18 RPM feel, без стробування та без ривків)
            static uint32_t s_last_rot_ms = 0;
            uint32_t dt_ms = (s_last_rot_ms == 0) ? 33 : (now - s_last_rot_ms);
            s_last_rot_ms = now;
            if (dt_ms > 100) dt_ms = 33;

            if (state.spotify.playing) {
                float rot_speed = state.spotify.isSwitchingOptimistic(now) ? 320.0f : 108.0f;
                vinyl_angle += rot_speed * ((float)dt_ms / 1000.0f);
                if (vinyl_angle >= 360.0f) vinyl_angle -= 360.0f;
            }

            // Швидкий pushRotateZoom (цілочисельний раструвальник) замість важкого pushRotateZoomWithAA
            // Непрозора підкладка вінілу: 100% захист від просвічування або накривання фоном!
            canvas.fillCircle(120, 84, 67, canvas.color565(14, 16, 22));
            canvas.drawCircle(120, 84, 68, canvas.color565((uint8_t)(cur_r * 0.45f), (uint8_t)(cur_g * 0.45f), (uint8_t)(cur_b * 0.45f)));
            vinylSprite.pushRotateZoom(&canvas, 120, 84, vinyl_angle, 1.0f, 1.0f, (uint16_t)0x0000);

            // 3. Кругова дуга прогресу треку навколо вінілу (R = 72..70 на X = 120, Y = 84)
            uint32_t cur_prog = state.getLocalSpotifyProgressMs();
            float prog_frac = (state.spotify.duration_ms > 0) ? ((float)cur_prog / (float)state.spotify.duration_ms) : 0.0f;
            prog_frac = constrain(prog_frac, 0.0f, 1.0f);

            canvas.drawArc(120, 84, 72, 70, 0, 360, dim_primary);
            int prog_deg = (int)(prog_frac * 360.0f);
            if (prog_deg > 0) {
                canvas.drawArc(120, 84, 72, 70, 270, 270 + prog_deg, primary_col);
                float rad = (270.0f + (float)prog_deg) * 0.0174532925f;
                int dot_x = 120 + (int)(cosf(rad) * 71.0f);
                int dot_y = 84  + (int)(sinf(rad) * 71.0f);
                canvas.fillCircle(dot_x, dot_y, 3, primary_col);
                canvas.fillCircle(dot_x, dot_y, 1, TFT_WHITE);
            }

            // 4. Механічний тонарм програвача (Hi-Fi Precision Tonearm)
            // Точна часова анімація з Quad Ease-Out:
            // - При зупинці: мікро-пауза 100 мс (захист від тремтіння), потім граціозне паркування рівно за 260 мс (без нескінченного «хвоста»!)
            // - При старті: делікатне опускання на платівку рівно за 320 мс
            static float s_arm_pos = 0.0f;
            static float s_arm_start = 0.0f;
            static uint32_t s_anim_start_ms = 0;
            static uint32_t s_anim_dur_ms = 260;
            static bool s_last_playing_target = false;
            static uint32_t s_pause_event_ms = 0;

            if (s_last_playing_target != state.spotify.playing) {
                if (!state.spotify.playing) {
                    s_pause_event_ms = now;
                }
                s_last_playing_target = state.spotify.playing;
                s_arm_start = s_arm_pos;
                s_anim_start_ms = now;
                s_anim_dur_ms = state.spotify.playing ? 320 : 260;
            }

            float target_val = state.spotify.playing ? 1.0f : 0.0f;
            if (!state.spotify.playing && (now - s_pause_event_ms < 100)) {
                s_anim_start_ms = now;
            } else {
                uint32_t elapsed = (now >= s_anim_start_ms) ? (now - s_anim_start_ms) : 0;
                if (elapsed >= s_anim_dur_ms) {
                    s_arm_pos = target_val;
                } else {
                    float t = (float)elapsed / (float)s_anim_dur_ms;
                    float ease = t * (2.0f - t); // Quad Ease-Out
                    s_arm_pos = s_arm_start + (target_val - s_arm_start) * ease;
                }
            }
            float s_arm_lerp = s_arm_pos;
            state.tonearm_moving = (s_arm_pos > 0.005f && s_arm_pos < 0.995f) || (now - s_pause_event_ms < 380);

            if (s_arm_lerp > 0.02f) {
                int pivot_x = 208, pivot_y = 30;
                float groove_r = 64.0f - prog_frac * 28.0f;
                float needle_angle_rad = 0.58f; // ~33 градуси вниз-вправо від центру вінілу
                float on_x = 120.0f + cosf(needle_angle_rad) * groove_r;
                float on_y = 84.0f  + sinf(needle_angle_rad) * groove_r;

                float rest_x = 204.0f, rest_y = 68.0f;
                float rest_elbow_x = 212.0f, rest_elbow_y = 48.0f;
                float on_elbow_x = 188.0f, on_elbow_y = 54.0f;

                int cur_arm_x = (int)(rest_x + (on_x - rest_x) * s_arm_lerp);
                int cur_arm_y = (int)(rest_y + (on_y - rest_y) * s_arm_lerp);
                int cur_elb_x = (int)(rest_elbow_x + (on_elbow_x - rest_elbow_x) * s_arm_lerp);
                int cur_elb_y = (int)(rest_elbow_y + (on_elbow_y - rest_elbow_y) * s_arm_lerp);

                canvas.fillCircle(pivot_x, pivot_y, 4, canvas.color565(75, 85, 99));
                canvas.drawCircle(pivot_x, pivot_y, 4, canvas.color565(156, 163, 175));
                canvas.drawLine(pivot_x, pivot_y, cur_elb_x, cur_elb_y, canvas.color565(156, 163, 175));
                canvas.drawLine(cur_elb_x, cur_elb_y, cur_arm_x, cur_arm_y, canvas.color565(209, 213, 219));
                canvas.fillRect(cur_arm_x - 2, cur_arm_y - 2, 5, 5, primary_col);
                canvas.drawPixel(cur_arm_x, cur_arm_y, TFT_WHITE);
            }

            // 5. Елегантний індикатор паузи ⏸ на вінілі при зупинці (миттєво при 0 ms)
            if (!state.spotify.playing) {
                canvas.fillRoundRect(102, 72, 36, 24, 8, canvas.color565(15, 18, 26));
                canvas.drawRoundRect(102, 72, 36, 24, 8, primary_col);
                canvas.fillRect(112, 78, 4, 12, TFT_WHITE);
                canvas.fillRect(124, 78, 4, 12, TFT_WHITE);
            }

            if (state.spotify_bg_style == 1) {
                // Scrim Card для стилю «Глибокий чорний» (Obsidian Dark v6.8)
                uint16_t scrim_bg = canvas.color565(
                    (uint8_t)constrain(12 + cur_r * 0.08f, 0.0f, 255.0f),
                    (uint8_t)constrain(14 + cur_g * 0.08f, 0.0f, 255.0f),
                    (uint8_t)constrain(20 + cur_b * 0.08f, 0.0f, 255.0f)
                );
                uint16_t scrim_border = canvas.color565(
                    (uint8_t)constrain(28 + cur_r * 0.22f, 0.0f, 255.0f),
                    (uint8_t)constrain(32 + cur_g * 0.22f, 0.0f, 255.0f),
                    (uint8_t)constrain(42 + cur_b * 0.22f, 0.0f, 255.0f)
                );
                canvas.fillRoundRect(16, 154, 208, 44, 10, scrim_bg);
                canvas.drawRoundRect(16, 154, 208, 44, 10, scrim_border);
            }

            // 3. Рядок 1 (Y = 168): Назва пісні (UkrainianFont16Bold, чистий білий #FFFFFF, плавний лінійний marquee)
            const char* title_str = state.spotify.isSwitchingOptimistic(now) ? ">> Перемикання..." : (state.spotify.title[0] != '\0' ? state.spotify.title : "Spotify Ready");
            drawSmoothMarquee(canvas, title_str, 120, 168, 196, TFT_WHITE, &UkrainianFont16Bold, now, 2200, 1500, 28.0f);

            // 4. Рядок 2 (Y = 188): Виключно виконавець
            uint8_t pastel_r = (uint8_t)((cur_r * 55 + 255 * 45) / 100);
            uint8_t pastel_g = (uint8_t)((cur_g * 55 + 255 * 45) / 100);
            uint8_t pastel_b = (uint8_t)((cur_b * 55 + 255 * 45) / 100);
            uint16_t pastel_col = canvas.color565(pastel_r, pastel_g, pastel_b);

            const char* artist_str = (state.spotify.artist[0] != '\0') ? state.spotify.artist : "Торкнись кришки — Play";
            drawSmoothMarquee(canvas, artist_str, 120, 188, 194, pastel_col, &UkrainianFont12Bold, now, 2200, 1500, 26.0f);

            // 5. Рядок 3 (Y = 208): Кристально чистий таймер MM:SS / MM:SS на стильному бейджі
            uint32_t prog_sec = cur_prog / 1000UL;
            uint32_t dur_sec = state.spotify.duration_ms / 1000UL;
            char timer_buf[24];
            snprintf(timer_buf, sizeof(timer_buf), "%02u:%02u  /  %02u:%02u",
                     (unsigned)(prog_sec / 60), (unsigned)(prog_sec % 60),
                     (unsigned)(dur_sec / 60), (unsigned)(dur_sec % 60));

            uint16_t pill_bg = canvas.color565((uint8_t)(cur_r * 0.16f), (uint8_t)(cur_g * 0.16f), (uint8_t)(cur_b * 0.16f));
            uint16_t pill_border = canvas.color565((uint8_t)(cur_r * 0.35f), (uint8_t)(cur_g * 0.35f), (uint8_t)(cur_b * 0.35f));
            canvas.fillRoundRect(56, 200, 128, 16, 8, pill_bg);
            canvas.drawRoundRect(56, 200, 128, 16, 8, pill_border);

            canvas.setFont(&UkrainianFont12Bold);
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString(timer_buf, 120, 208);

            // 6. Нижня кромка (Y = 230..239): Оптичний міст (Optical Bridge)
            float bass_pulse = 1.0f + 0.15f * sinf((float)now * 0.008f * bpm_speed);
            uint8_t ob_r = (uint8_t)constrain(cur_r * bass_pulse, 0.0f, 255.0f);
            uint8_t ob_g = (uint8_t)constrain(cur_g * bass_pulse, 0.0f, 255.0f);
            uint8_t ob_b = (uint8_t)constrain(cur_b * bass_pulse, 0.0f, 255.0f);
            uint16_t ob_col = canvas.color565(ob_r, ob_g, ob_b);
            uint16_t ob_dim = canvas.color565((uint8_t)(ob_r * 0.35f), (uint8_t)(ob_g * 0.35f), (uint8_t)(ob_b * 0.35f));
            canvas.drawArc(120, 120, 119, 117, 50, 130, ob_col);
            canvas.drawArc(120, 120, 117, 115, 50, 130, ob_dim);
            return;
        }

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

    // ВІКНО: SCREEN_POMODORO (10 FPS)
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
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("POMODORO FOCUS", 120, 36);

        // Статус-бейдж (компактна капсула зверху таймера)
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

        canvas.fillRoundRect(36, 52, 168, 20, 10, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(36, 52, 168, 20, 10, badge_col);
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(text_col);
        canvas.drawString(st_txt, 120, 62);

        // Великі цифри таймера — строго в геометричному центрі циферблата (Y = 108)
        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        char t_buf[16];
        uint16_t m = rem_sec / 60;
        uint16_t s = rem_sec % 60;
        snprintf(t_buf, sizeof(t_buf), "%02u:%02u", m, s);
        canvas.drawString(t_buf, 120, 108);

        // Капсули керування над кнопками (гармонійно врівноважені знизу)
        const char* l_lbl = state.pomo_active ? "◄ -1м" : "◄ -5м";
        canvas.fillRoundRect(22, 143, 58, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(22, 143, 58, 22, 11, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(l_lbl, 51, 154);

        const char* t_lbl = !state.pomo_active ? "СТАРТ" : (state.pomo_paused ? "ДАЛІ" : "ПАУЗА");
        canvas.fillRoundRect(88, 143, 64, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(88, 143, 64, 22, 11, arc_col);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(arc_col);
        canvas.drawString(t_lbl, 120, 154);

        const char* r_lbl = state.pomo_active ? "+1м ►" : "+5м ►";
        canvas.fillRoundRect(160, 143, 58, 22, 11, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(160, 143, 58, 22, 11, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::TEXT_PRIMARY);
        canvas.drawString(r_lbl, 189, 154);

        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(HallmarkTheme::TEXT_SECONDARY);
        canvas.drawString("Утримуй ◄ для виходу", 120, 188);
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
        // На екрані Spotify ховаємо крапки пагінації через 2 с, щоб не захаращувати світловий міст дна
        if (state.screen_mode == SCREEN_SMART_HUB && state.last_screen_switch_ms > 0 && (millis() - state.last_screen_switch_ms > 2000UL)) {
            return;
        }
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
