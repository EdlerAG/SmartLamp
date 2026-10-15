#pragma once
#include <LovyanGFX.hpp>
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "ui/TextUtils.h"

namespace ScreenRenderer {
    inline void drawOta(LGFX_Sprite& canvas, AppState& state) {
        canvas.drawCircle(120, 120, 118, canvas.color565(30, 45, 65));
        int angle = 180 + (int)(state.ota_progress * 3.6f);
        canvas.drawArc(120, 120, 112, 103, 0, 360, canvas.color565(18, 26, 38));
        canvas.drawArc(120, 120, 112, 103, 180, angle, canvas.color565(16, 185, 129));

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(canvas.color565(56, 189, 248));
        canvas.drawString("OTA FIRMWARE", 120, 78);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(TFT_WHITE);
        char p_txt[12];
        snprintf(p_txt, sizeof(p_txt), "%d%%", state.ota_progress);
        canvas.drawString(p_txt, 120, 125);

        canvas.setFont(&fonts::FreeSans9pt7b);
        canvas.setTextColor(canvas.color565(148, 163, 184));
        canvas.drawString("Do not power off", 120, 168);
    }

    inline void drawClimate(LGFX_Sprite& canvas, AppState& state, float& smooth_hum_angle) {
        float target_hum = 180.0f + constrain(state.humidity, 0.0f, 100.0f) * 1.8f;
        smooth_hum_angle += (target_hum - smooth_hum_angle) * 0.25f;

        canvas.drawCircle(120, 120, 118, canvas.color565(28, 38, 54));
        canvas.drawArc(120, 120, 114, 108, 180, 360, canvas.color565(18, 26, 38));
        canvas.drawArc(120, 120, 114, 108, 180, (int)smooth_hum_angle, canvas.color565(56, 189, 248));

        uint16_t pill_col = canvas.color565(18, 38, 62);
        if (state.light_mode == MODE_LIGHT_OFF) pill_col = canvas.color565(40, 18, 24);
        else if (state.light_mode >= MODE_CANDLE && state.light_mode <= MODE_FIREPLACE) pill_col = canvas.color565(65, 32, 10);

        canvas.fillRoundRect(24, 20, 192, 24, 12, pill_col);
        canvas.drawRoundRect(24, 20, 192, 24, 12, canvas.color565(60, 90, 130));

        uint16_t wifi_dot = state.wifi_connected ? canvas.color565(16, 185, 129) :
                           (state.ap_mode_active ? TFT_ORANGE : canvas.color565(90, 100, 115));
        canvas.fillCircle(36, 32, 4, wifi_dot);

        const char* m_name = CommandRouter::getModeNameAscii(state.light_mode);
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(TFT_WHITE);

        uint16_t sleep_min = state.getSleepTimerRemainingMin();
        if (sleep_min > 0) {
            char sl_bar[38];
            snprintf(sl_bar, sizeof(sl_bar), "SLEEP %um | %s", sleep_min, m_name);
            canvas.drawString(sl_bar, 124, 32);
        } else if (state.time_synced) {
            char t_local[10], top_bar[38];
            state.getTimeString(t_local, sizeof(t_local));
            snprintf(top_bar, sizeof(top_bar), "%s %s", t_local, m_name);
            canvas.drawString(top_bar, 124, 32);
        } else {
            canvas.drawString(m_name, 124, 32);
        }

        int t_int = (int)state.temperature;
        int t_dec = abs((int)(state.temperature * 10.0f) - t_int * 10);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(TFT_WHITE);
        char t_main[12];
        snprintf(t_main, sizeof(t_main), "%d", t_int);
        canvas.drawString(t_main, 96, 84);

        const uint16_t col_accent = canvas.color565(56, 189, 248);
        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextColor(col_accent);
        char t_frac[8];
        snprintf(t_frac, sizeof(t_frac), ".%d", t_dec);
        canvas.drawString(t_frac, 148, 92);
        canvas.drawCircle(168, 70, 3, col_accent);
        canvas.drawString("C", 182, 82);

        canvas.fillRoundRect(22, 114, 94, 42, 10, canvas.color565(14, 22, 36));
        canvas.drawRoundRect(22, 114, 94, 42, 10, canvas.color565(32, 48, 72));
        canvas.fillRoundRect(124, 114, 94, 42, 10, canvas.color565(14, 22, 36));
        canvas.drawRoundRect(124, 114, 94, 42, 10, canvas.color565(32, 48, 72));

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(canvas.color565(140, 155, 175));
        canvas.drawString("HUMIDITY", 69, 123);
        canvas.drawString("PRESSURE", 171, 123);

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(canvas.color565(52, 211, 153));
        char h_str[12];
        snprintf(h_str, sizeof(h_str), "%.0f %%", state.humidity);
        canvas.drawString(h_str, 69, 142);

        canvas.setTextColor(canvas.color565(251, 146, 60));
        char p_str[12];
        snprintf(p_str, sizeof(p_str), "%.0f mm", state.pressure);
        canvas.drawString(p_str, 171, 142);

        uint8_t brt_pct = state.getLedBrightnessPercent();
        canvas.fillRoundRect(28, 162, 184, 26, 8, canvas.color565(12, 18, 30));
        int bar_w = constrain((brt_pct * 176) / 100, 4, 176);
        uint16_t bar_col = (state.light_mode == MODE_LIGHT_OFF) ? canvas.color565(70, 80, 95) : canvas.color565(250, 204, 21);
        canvas.fillRoundRect(32, 181, bar_w, 4, 2, bar_col);

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(canvas.color565(226, 232, 240));
        char brt_info[40];
        snprintf(brt_info, sizeof(brt_info), "%s | LAMP: %d%% | ROOM: %d%%",
                 state.auto_brt ? "AUTO" : "MAN", brt_pct, state.light_percent);
        canvas.drawString(brt_info, 120, 171);
    }

    inline void drawBarometer(LGFX_Sprite& canvas, AppState& state, float& smooth_pres_angle) {
        for (int a = 135; a <= 405; a += 15) {
            float rad = a * 0.0174532925f;
            int r_in  = (a % 45 == 0) ? 98 : 103;
            canvas.drawLine(120 + (int)(cosf(rad) * r_in), 120 + (int)(sinf(rad) * r_in),
                            120 + (int)(cosf(rad) * 108),  120 + (int)(sinf(rad) * 108),
                            (a == 270) ? TFT_GREEN : canvas.color565(60, 75, 95));
        }

        float hist[24];
        uint8_t h_cnt = 0;
        float p_diff  = 0.0f;
        state.getPressureHistory(hist, h_cnt, p_diff);

        float clamped_p = constrain(state.pressure, 730.0f, 770.0f);
        float target_p_angle = 135.0f + ((clamped_p - 730.0f) / 40.0f * 270.0f);
        smooth_pres_angle += (target_p_angle - smooth_pres_angle) * 0.25f;

        uint16_t arc_col = (state.pressure < 744.0f) ? canvas.color565(250, 204, 21) :
                          ((state.pressure > 758.0f) ? canvas.color565(52, 211, 153) : canvas.color565(56, 189, 248));

        canvas.drawArc(120, 120, 115, 110, 135, 405, canvas.color565(20, 28, 42));
        canvas.drawArc(120, 120, 115, 110, 135, (int)smooth_pres_angle, arc_col);

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(canvas.color565(148, 163, 184));
        canvas.drawString("BAROMETER", 120, 42);

        canvas.setFont(&fonts::FreeSansBold24pt7b);
        canvas.setTextColor(TFT_WHITE);
        char p_str[16];
        snprintf(p_str, sizeof(p_str), "%.0f", state.pressure);
        canvas.drawString(p_str, 120, 82);

        const char* trend_str = "mm Hg | STEADY";
        if (p_diff >= 0.8f) trend_str = "mm Hg | RISING ^";
        else if (p_diff <= -0.8f) trend_str = "mm Hg | FALLING v";

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(arc_col);
        canvas.drawString(trend_str, 120, 110);

        canvas.fillRoundRect(44, 120, 152, 30, 6, canvas.color565(12, 18, 30));
        for (int i = 0; i < 24; i++) {
            float v = constrain(hist[i], 735.0f, 765.0f);
            int bh = 4 + (int)((v - 735.0f) / 30.0f * 20.0f);
            canvas.fillRect(49 + i * 6, 147 - bh, 4, bh, (i == 23) ? TFT_WHITE : arc_col);
        }

        const char* wf = "OPTIMAL / STABLE";
        if (state.pressure < 742.0f || p_diff <= -1.5f) wf = "RAIN / STORM FRONT";
        else if (state.pressure > 760.0f || p_diff >= 1.5f) wf = "CLEAR / SUNNY SKY";

        canvas.fillRoundRect(32, 156, 176, 24, 12, canvas.color565(16, 26, 42));
        canvas.drawRoundRect(32, 156, 176, 24, 12, arc_col);
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(wf, 120, 168);
    }

    inline void drawTelegram(LGFX_Sprite& canvas, AppState& state) {
        char raw_sender[28], raw_msg[64], msg_time[8];
        state.getTelegramData(raw_sender, sizeof(raw_sender), raw_msg, sizeof(raw_msg), msg_time, sizeof(msg_time));

        char sender[42], msg[96];
        TextUtils::utf8ToCleanAscii(raw_sender, sender, sizeof(sender));
        TextUtils::utf8ToCleanAscii(raw_msg, msg, sizeof(msg));

        if (state.tg_unread) {
            uint8_t pulse = 100 + (abs((int)((millis() / 8) % 300) - 150));
            canvas.drawCircle(120, 120, 116, canvas.color565(0, pulse / 2, pulse));
            canvas.drawCircle(120, 120, 115, canvas.color565(0, pulse / 2, pulse));
        } else {
            canvas.drawCircle(120, 120, 116, canvas.color565(30, 42, 58));
        }

        canvas.fillRoundRect(38, 20, 164, 24, 12, canvas.color565(2, 132, 199));
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(TFT_WHITE);
        char hdr_buf[24];
        snprintf(hdr_buf, sizeof(hdr_buf), "INBOX %u/%u", state.inbox_view_idx + 1, state.inbox_count);
        canvas.drawString(hdr_buf, 120, 32);

        canvas.fillCircle(120, 68, 16, canvas.color565(14, 165, 233));
        canvas.drawCircle(120, 68, 16, TFT_WHITE);
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.drawString("TG", 120, 68);

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(canvas.color565(56, 189, 248));
        char s_line[54];
        snprintf(s_line, sizeof(s_line), "%s (%s)", sender, msg_time);
        canvas.drawString(s_line, 120, 100);

        canvas.fillRoundRect(16, 116, 208, 52, 12, canvas.color565(18, 27, 42));
        canvas.drawRoundRect(16, 116, 208, 52, 12, canvas.color565(45, 65, 95));

        canvas.setFont(&fonts::FreeSans9pt7b);
        canvas.setTextColor(TFT_WHITE);

        size_t len = strlen(msg);
        if (len <= 20) {
            canvas.drawString(msg, 120, 142);
        } else {
            int split_pos = 20;
            for (int i = 20; i >= 10; i--) {
                if (msg[i] == ' ') { split_pos = i; break; }
            }
            char line1[26], line2[32];
            strncpy(line1, msg, split_pos);
            line1[split_pos] = '\0';
            const char* rest = msg + split_pos + (msg[split_pos] == ' ' ? 1 : 0);
            strncpy(line2, rest, 24);
            line2[24] = '\0';

            canvas.drawString(line1, 120, 132);
            canvas.drawString(line2, 120, 152);
        }

        canvas.setFont(&fonts::Font0);
        if (state.tg_unread) {
            canvas.setTextColor(canvas.color565(250, 204, 21));
            canvas.drawString("● TAP LID TO READ | R: NEXT MSG", 120, 180);
        } else {
            canvas.setTextColor(canvas.color565(100, 116, 139));
            canvas.drawString("✓ SEEN | TAP R FOR HISTORY", 120, 180);
        }
    }

    inline void drawNetworkQr(LGFX_Sprite& canvas, LGFX_Sprite& qr_cache, char* cached_qr_str, size_t qr_len, AppState& state) {
        char ssid[33], ip[20];
        state.getNetworkInfo(ssid, sizeof(ssid), ip, sizeof(ip));

        char qr_payload[80];
        const char* header_txt = "WEB PANEL QR";
        const char* sub_txt    = ip;
        uint16_t hdr_col       = canvas.color565(5, 150, 105);

        if (state.wifi_sleeping) {
            header_txt = "ECO WIFI OFF";
            hdr_col    = canvas.color565(75, 85, 99);
            snprintf(qr_payload, sizeof(qr_payload), "WIFI:S:%s;T:nopass;;", Config::AP_SSID);
        } else if (state.ap_mode_active) {
            if (state.ap_clients == 0) {
                header_txt = "1. JOIN WIFI QR";
                sub_txt    = "Scan to connect AP";
                hdr_col    = canvas.color565(217, 119, 6);
                snprintf(qr_payload, sizeof(qr_payload), "WIFI:S:%s;T:nopass;;", Config::AP_SSID);
            } else {
                header_txt = "2. OPEN SITE QR";
                sub_txt    = "Scan -> Enter Password";
                hdr_col    = canvas.color565(16, 185, 129);
                snprintf(qr_payload, sizeof(qr_payload), "http://%s/", ip);
            }
        } else {
            header_txt = "SMART HUB QR";
            hdr_col    = canvas.color565(2, 132, 199);
            snprintf(qr_payload, sizeof(qr_payload), "http://%s/", ip);
        }

        canvas.fillRoundRect(34, 18, 172, 22, 11, hdr_col);
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(header_txt, 120, 29);

        if (strcmp(qr_payload, cached_qr_str) != 0) {
            strncpy(cached_qr_str, qr_payload, qr_len - 1);
            cached_qr_str[qr_len - 1] = '\0';
            qr_cache.fillScreen(TFT_WHITE);
            qr_cache.qrcode(qr_payload, 5, 5, 100, 3);
        }

        canvas.fillRoundRect(59, 42, 122, 122, 12, hdr_col);
        canvas.fillRoundRect(61, 44, 118, 118, 10, TFT_WHITE);
        qr_cache.pushSprite(&canvas, 65, 49);

        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(canvas.color565(56, 189, 248));
        canvas.drawString(ssid, 120, 174);

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(canvas.color565(250, 204, 21));
        canvas.drawString(sub_txt, 120, 190);
    }

    inline void drawFooterNav(LGFX_Sprite& canvas, AppState& state) {
        for (int i = 0; i < SCREEN_COUNT; i++) {
            int x = 102 + i * 12;
            uint16_t c = (state.screen_mode == i) ? canvas.color565(56, 189, 248) : canvas.color565(40, 50, 65);
            canvas.fillCircle(x, 201, (state.screen_mode == i) ? 3 : 2, c);
        }

        canvas.fillRoundRect(68,  210, 28, 16, 8, state.btn_l_pressed ? TFT_GREEN : canvas.color565(22, 32, 48));
        canvas.fillRoundRect(106, 210, 28, 16, 8, state.btn_t_pressed ? TFT_GREEN : canvas.color565(22, 32, 48));
        canvas.fillRoundRect(144, 210, 28, 16, 8, state.btn_r_pressed ? TFT_GREEN : canvas.color565(22, 32, 48));

        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(canvas.color565(200, 210, 225));
        canvas.drawString("SCR", 82, 218);
        canvas.drawString("LED", 120, 218);
        canvas.drawString((state.screen_mode == SCREEN_TELEGRAM) ? "NXT" : (state.auto_brt ? "AUT" : "MAN"), 158, 218);
    }
}