#pragma once
#include "DisplayDriver.h"
#include "IService.h"
#include "AppState.h"
#include "CommandRouter.h"

class DisplayService : public IService {
private:
    AppState& state;
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas{&lcd};
    LGFX_Sprite qr_cache{&canvas};

    uint32_t last_frame_ms     = 0;
    uint8_t  applied_lcd_brt   = 200;
    char     cached_qr_str[80] = "";

    float smooth_hum_angle  = 180.0f;
    float smooth_pres_angle = 270.0f;

    static void utf8ToCleanAscii(const char* in, char* out, size_t max_out) {
        size_t i = 0, j = 0;
        while (in[i] != '\0' && j < max_out - 3) {
            uint8_t c = (uint8_t)in[i];
            if (c < 0x80) {
                out[j++] = (char)c;
                i++;
            } else if ((c == 0xD0 || c == 0xD1) && in[i + 1] != '\0') {
                uint8_t c2 = (uint8_t)in[i + 1];
                uint16_t code = (c << 8) | c2;
                i += 2;
                const char* rep = "";
                switch (code) {
                    case 0xD090: rep="A"; break;  case 0xD0B0: rep="a"; break;
                    case 0xD091: rep="B"; break;  case 0xD0B1: rep="b"; break;
                    case 0xD092: rep="V"; break;  case 0xD0B2: rep="v"; break;
                    case 0xD093: rep="H"; break;  case 0xD0B3: rep="h"; break;
                    case 0xD290: rep="G"; break;  case 0xD291: rep="g"; break;
                    case 0xD094: rep="D"; break;  case 0xD0B4: rep="d"; break;
                    case 0xD095: rep="E"; break;  case 0xD0B5: rep="e"; break;
                    case 0xD084: rep="Ye"; break; case 0xD194: rep="ie"; break;
                    case 0xD096: rep="Zh"; break; case 0xD0B6: rep="zh"; break;
                    case 0xD097: rep="Z"; break;  case 0xD0B7: rep="z"; break;
                    case 0xD098: rep="Y"; break;  case 0xD0B8: rep="y"; break;
                    case 0xD086: rep="I"; break;  case 0xD196: rep="i"; break;
                    case 0xD087: rep="Yi"; break; case 0xD197: rep="i"; break;
                    case 0xD099: rep="Y"; break;  case 0xD0B9: rep="i"; break;
                    case 0xD09A: rep="K"; break;  case 0xD0BA: rep="k"; break;
                    case 0xD09B: rep="L"; break;  case 0xD0BB: rep="l"; break;
                    case 0xD09C: rep="M"; break;  case 0xD0BC: rep="m"; break;
                    case 0xD09D: rep="N"; break;  case 0xD0BD: rep="n"; break;
                    case 0xD09E: rep="O"; break;  case 0xD0BE: rep="o"; break;
                    case 0xD09F: rep="P"; break;  case 0xD0BF: rep="p"; break;
                    case 0xD0A0: rep="R"; break;  case 0xD180: rep="r"; break;
                    case 0xD0A1: rep="S"; break;  case 0xD181: rep="s"; break;
                    case 0xD0A2: rep="T"; break;  case 0xD182: rep="t"; break;
                    case 0xD0A3: rep="U"; break;  case 0xD183: rep="u"; break;
                    case 0xD0A4: rep="F"; break;  case 0xD184: rep="f"; break;
                    case 0xD0A5: rep="Kh"; break; case 0xD185: rep="kh"; break;
                    case 0xD0A6: rep="Ts"; break; case 0xD186: rep="ts"; break;
                    case 0xD0A7: rep="Ch"; break; case 0xD187: rep="ch"; break;
                    case 0xD0A8: rep="Sh"; break; case 0xD188: rep="sh"; break;
                    case 0xD0A9: rep="Shch";break;case 0xD189: rep="shch";break;
                    case 0xD0AE: rep="Yu"; break; case 0xD18E: rep="iu"; break;
                    case 0xD0AF: rep="Ya"; break; case 0xD18F: rep="ia"; break;
                    default: rep=""; break;
                }
                while (*rep && j < max_out - 1) out[j++] = *rep++;
            } else {
                i++;
            }
        }
        out[j] = '\0';
    }

public:
    DisplayService(AppState& st) : state(st) {}

    void init() override {
        lcd.init();
        lcd.setRotation(0);
        lcd.setBrightness(state.current_lcd_brt);

        canvas.setPsram(false);
        canvas.setColorDepth(16);
        canvas.createSprite(240, 240);
        canvas.setTextDatum(textdatum_t::middle_center);

        qr_cache.setPsram(false);
        qr_cache.setColorDepth(8);
        qr_cache.createSprite(110, 110);
    }

    bool isHighFpsNeeded() const {
        uint32_t now = millis();
        return state.ota_in_progress ||
               (now < state.screen_reaction_until) ||
               (now - state.last_user_activity_ms < 4000) ||
               state.btn_l_pressed || state.btn_t_pressed || state.btn_r_pressed;
    }

    void update() override {
        uint32_t now = millis();

        uint32_t frame_interval = isHighFpsNeeded() ? 16 : 250;
        if (now - last_frame_ms < frame_interval && !state.ui_force_dirty) return;
        last_frame_ms = now;
        state.ui_force_dirty = false;

        if (applied_lcd_brt != state.current_lcd_brt) {
            applied_lcd_brt = state.current_lcd_brt;
            lcd.setBrightness(applied_lcd_brt);
        }

        canvas.fillScreen(canvas.color565(6, 9, 16));

        if (state.ota_in_progress) {
            drawOtaScreen();
            canvas.pushSprite(0, 0);
            return;
        }

        if (now < state.screen_reaction_until && state.screen_reaction_type >= NOTIFY_FIRE) {
            drawReactionOverlay(state.screen_reaction_type, now);
            canvas.pushSprite(0, 0);
            return;
        }

        switch (state.screen_mode) {
            case SCREEN_CLIMATE:   drawClimateScreen();   break;
            case SCREEN_BAROMETER: drawBarometerScreen(); break;
            case SCREEN_TELEGRAM:  drawTelegramScreen();  break;
            case SCREEN_NETWORK:   drawNetworkQrScreen(); break;
        }

        drawFooterNav();
        canvas.pushSprite(0, 0);
    }

private:
    // =================================================================
    // 5 АНІМОВАНИХ ВЕКТОРНИХ МЕМ-РЕАКЦІЙ НА ПОВНИЙ ЕКРАН
    // =================================================================
    void drawReactionOverlay(uint8_t r_type, uint32_t now) {
        int bounce = (int)(sinf(now * 0.012f) * 5.0f);

        if (r_type == NOTIFY_CAT) {
            // 🐱 МЕМНИЙ КІТ (підморгує кожну секунду!)
            uint16_t col_mint = canvas.color565(52, 211, 153);
            uint16_t col_pink = canvas.color565(244, 114, 182);
            canvas.drawArc(120, 120, 116, 110, 0, 360, col_mint);
            int cy = 110 + bounce;

            canvas.fillTriangle(72, cy - 18, 62, cy - 58, 100, cy - 36, col_mint);
            canvas.fillTriangle(75, cy - 24, 69, cy - 48, 94,  cy - 34, col_pink);
            canvas.fillTriangle(168, cy - 18, 178, cy - 58, 140, cy - 36, col_mint);
            canvas.fillTriangle(165, cy - 24, 171, cy - 48, 146, cy - 34, col_pink);

            canvas.fillCircle(120, cy, 44, col_mint);
            canvas.fillCircle(120, cy, 40, canvas.color565(15, 23, 38));

            bool wink = ((now / 450) % 2 == 0);
            canvas.fillCircle(102, cy - 8, 7, TFT_YELLOW);
            canvas.fillCircle(102, cy - 8, 3, TFT_BLACK);
            if (wink) {
                canvas.drawFastHLine(130, cy - 8, 16, TFT_YELLOW);
                canvas.drawFastHLine(130, cy - 7, 16, TFT_YELLOW);
            } else {
                canvas.fillCircle(138, cy - 8, 7, TFT_YELLOW);
                canvas.fillCircle(138, cy - 8, 3, TFT_BLACK);
            }

            canvas.fillTriangle(115, cy + 4, 125, cy + 4, 120, cy + 10, col_pink);
            canvas.drawLine(120, cy + 10, 112, cy + 16, TFT_WHITE);
            canvas.drawLine(120, cy + 10, 128, cy + 16, TFT_WHITE);

            canvas.drawLine(58, cy + 2, 86, cy + 6, TFT_WHITE);
            canvas.drawLine(58, cy + 12, 86, cy + 12, TFT_WHITE);
            canvas.drawLine(182, cy + 2, 154, cy + 6, TFT_WHITE);
            canvas.drawLine(182, cy + 12, 154, cy + 12, TFT_WHITE);

            canvas.setFont(&fonts::FreeSansBold12pt7b);
            canvas.setTextColor(col_mint);
            canvas.drawString("TYTS-KUS!", 120, 186);
        }
        else if (r_type == NOTIFY_FIRE) {
            // 🔥 ВОГНИК
            uint16_t col_org = canvas.color565(251, 146, 60);
            uint16_t col_red = canvas.color565(239, 68, 68);
            canvas.drawArc(120, 120, 116, 110, 0, 360, col_org);

            int fy = 118 + bounce;
            canvas.fillCircle(120, fy + 10, 38, col_red);
            canvas.fillTriangle(82, fy + 8, 158, fy + 8, 120, fy - 56, col_red);
            canvas.fillCircle(120, fy + 14, 26, col_org);
            canvas.fillTriangle(94, fy + 12, 146, fy + 12, 120, fy - 36, col_org);
            canvas.fillCircle(120, fy + 18, 14, TFT_YELLOW);
            canvas.fillTriangle(106, fy + 18, 134, fy + 18, 120, fy - 14, TFT_YELLOW);

            canvas.setFont(&fonts::FreeSansBold12pt7b);
            canvas.setTextColor(TFT_YELLOW);
            canvas.drawString("FIRE VIBE!", 120, 190);
        }
        else if (r_type == NOTIFY_LIGHTNING) {
            // ⚡ БЛИСКАВКА
            uint16_t col_el = canvas.color565(56, 189, 248);
            canvas.drawArc(120, 120, 116, 110, 0, 360, col_el);

            int ly = 110 + bounce;
            canvas.fillTriangle(132, ly - 56, 92, ly + 4, 124, ly + 4, TFT_YELLOW);
            canvas.fillTriangle(116, ly - 6, 148, ly - 6, 106, ly + 56, TFT_YELLOW);

            canvas.setFont(&fonts::FreeSansBold12pt7b);
            canvas.setTextColor(col_el);
            canvas.drawString("KUS-ZAP!", 120, 190);
        }
        else if (r_type == NOTIFY_UFO) {
            // 👽 НЛО ("ПОРОБЛЕНО!")
            uint16_t col_grn = canvas.color565(34, 197, 94);
            uint16_t col_Slv = canvas.color565(148, 163, 184);
            canvas.drawArc(120, 120, 116, 110, 0, 360, col_grn);

            int uy = 85 + bounce;
            // Промінь викрадення
            canvas.fillTriangle(120, uy + 10, 75, 165, 165, 165, canvas.color565(16, 75, 40));
            // Купол та корпус тарілки
            canvas.fillCircle(120, uy - 6, 22, canvas.color565(56, 189, 248));
            canvas.fillEllipse(120, uy + 4, 48, 14, col_Slv);
            canvas.fillCircle(95,  uy + 4, 4, TFT_YELLOW);
            canvas.fillCircle(120, uy + 6, 4, col_grn);
            canvas.fillCircle(145, uy + 4, 4, TFT_YELLOW);

            canvas.setFont(&fonts::FreeSansBold12pt7b);
            canvas.setTextColor(col_grn);
            canvas.drawString("UFO BEAM!", 120, 190);
        }
        else if (r_type == NOTIFY_CROWN) {
            // 👑 КОРОЛІВСЬКА КОРОНА
            uint16_t col_gld = canvas.color565(250, 204, 21);
            canvas.drawArc(120, 120, 116, 110, 0, 360, col_gld);

            int ky = 122 + bounce;
            canvas.fillRect(76, ky, 88, 18, col_gld);
            canvas.fillTriangle(76, ky, 76, ky - 42, 102, ky, col_gld);
            canvas.fillTriangle(98, ky, 120, ky - 52, 142, ky, col_gld);
            canvas.fillTriangle(138, ky, 164, ky - 42, 164, ky, col_gld);
            canvas.fillCircle(76,  ky - 44, 5, TFT_WHITE);
            canvas.fillCircle(120, ky - 54, 6, canvas.color565(244, 63, 94));
            canvas.fillCircle(164, ky - 44, 5, TFT_WHITE);

            canvas.setFont(&fonts::FreeSansBold12pt7b);
            canvas.setTextColor(col_gld);
            canvas.drawString("QUEEN VIBE!", 120, 190);
        }
    }

    void drawOtaScreen() {
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

    void drawClimateScreen() {
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

    // ЕКРАН 1: Барометр зі стовпчиковим графіком історії та трендом
    void drawBarometerScreen() {
        for (int a = 135; a <= 405; a += 15) {
            float rad = a * 0.0174532925f;
            int r_in  = (a % 45 == 0) ? 98 : 103;
            int x0 = 120 + (int)(cosf(rad) * r_in);
            int y0 = 120 + (int)(sinf(rad) * r_in);
            int x1 = 120 + (int)(cosf(rad) * 108);
            int y1 = 120 + (int)(sinf(rad) * 108);
            uint16_t tick_col = (a == 270) ? TFT_GREEN : canvas.color565(60, 75, 95);
            canvas.drawLine(x0, y0, x1, y1, tick_col);
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

        // Живий стовпчиковий міні-графік тиску (24 точки)
        canvas.fillRoundRect(44, 120, 152, 30, 6, canvas.color565(12, 18, 30));
        for (int i = 0; i < 24; i++) {
            float v = constrain(hist[i], 735.0f, 765.0f);
            int bh = 4 + (int)((v - 735.0f) / 30.0f * 20.0f);
            int bx = 49 + i * 6;
            uint16_t bcol = (i == 23) ? TFT_WHITE : arc_col;
            canvas.fillRect(bx, 147 - bh, 4, bh, bcol);
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

    // ЕКРАН 2: Історія повідомлень (Inbox 1/5)
    void drawTelegramScreen() {
        char raw_sender[28], raw_msg[64], msg_time[8];
        state.getTelegramData(raw_sender, sizeof(raw_sender), raw_msg, sizeof(raw_msg), msg_time, sizeof(msg_time));

        char sender[42], msg[96];
        utf8ToCleanAscii(raw_sender, sender, sizeof(sender));
        utf8ToCleanAscii(raw_msg, msg, sizeof(msg));

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

    void drawNetworkQrScreen() {
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
            strncpy(cached_qr_str, qr_payload, sizeof(cached_qr_str) - 1);
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

    void drawFooterNav() {
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
};