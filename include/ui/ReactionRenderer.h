#pragma once
#include <LovyanGFX.hpp>
#include "core/Types.h"

namespace ReactionRenderer {
    inline void draw(LGFX_Sprite& canvas, uint8_t r_type, uint32_t now) {
        int bounce = (int)(sinf(now * 0.012f) * 5.0f);

        if (r_type == NOTIFY_CAT) {
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
            uint16_t col_grn = canvas.color565(34, 197, 94);
            uint16_t col_slv = canvas.color565(148, 163, 184);
            canvas.drawArc(120, 120, 116, 110, 0, 360, col_grn);

            int uy = 85 + bounce;
            canvas.fillTriangle(120, uy + 10, 75, 165, 165, 165, canvas.color565(16, 75, 40));
            canvas.fillCircle(120, uy - 6, 22, canvas.color565(56, 189, 248));
            canvas.fillEllipse(120, uy + 4, 48, 14, col_slv);
            canvas.fillCircle(95,  uy + 4, 4, TFT_YELLOW);
            canvas.fillCircle(120, uy + 6, 4, col_grn);
            canvas.fillCircle(145, uy + 4, 4, TFT_YELLOW);

            canvas.setFont(&fonts::FreeSansBold12pt7b);
            canvas.setTextColor(col_grn);
            canvas.drawString("UFO BEAM!", 120, 190);
        }
        else if (r_type == NOTIFY_CROWN) {
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
}