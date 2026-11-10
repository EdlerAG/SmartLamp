#pragma once
#include <LovyanGFX.hpp>
#include "core/Types.h"
#include "ui/HallmarkTheme.h"
#include "ui/UkrainianFont12Bold.h"

namespace ReactionRenderer {
    inline void drawReactionBadge(LGFX_Sprite& canvas, uint16_t accent_col, const char* label) {
        canvas.drawArc(120, 120, 116, 112, 0, 360, accent_col);
        canvas.fillRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BG_SURFACE);
        canvas.drawRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
        canvas.setFont(&UkrainianFont12Bold);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(accent_col);
        canvas.drawString(label, 120, 188);
    }

    inline void draw(LGFX_Sprite& canvas, uint8_t r_type, uint32_t now) {
        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);
        int bounce = (int)(sinf(now * 0.012f) * 4.0f);

        if (r_type == NOTIFY_CAT) {
            uint16_t col_acc = HallmarkTheme::ACCENT_WARM;
            int cy = 106 + bounce;

            canvas.fillTriangle(72, cy - 18, 62, cy - 54, 100, cy - 34, col_acc);
            canvas.fillTriangle(75, cy - 24, 69, cy - 46, 94,  cy - 32, HallmarkTheme::BG_SURFACE);
            canvas.fillTriangle(168, cy - 18, 178, cy - 54, 140, cy - 34, col_acc);
            canvas.fillTriangle(165, cy - 24, 171, cy - 46, 146, cy - 32, HallmarkTheme::BG_SURFACE);

            canvas.fillCircle(120, cy, 42, col_acc);
            canvas.fillCircle(120, cy, 38, HallmarkTheme::BG_SURFACE);

            bool wink = ((now / 500) % 2 == 0);
            canvas.fillCircle(102, cy - 6, 6, HallmarkTheme::ACCENT_AMBER);
            canvas.fillCircle(102, cy - 6, 2, HallmarkTheme::BG_PRIMARY);
            if (wink) {
                canvas.drawFastHLine(130, cy - 6, 16, HallmarkTheme::ACCENT_AMBER);
                canvas.drawFastHLine(130, cy - 5, 16, HallmarkTheme::ACCENT_AMBER);
            } else {
                canvas.fillCircle(138, cy - 6, 6, HallmarkTheme::ACCENT_AMBER);
                canvas.fillCircle(138, cy - 6, 2, HallmarkTheme::BG_PRIMARY);
            }

            canvas.fillTriangle(116, cy + 4, 124, cy + 4, 120, cy + 9, col_acc);
            canvas.drawLine(120, cy + 9, 114, cy + 15, HallmarkTheme::TEXT_PRIMARY);
            canvas.drawLine(120, cy + 9, 126, cy + 15, HallmarkTheme::TEXT_PRIMARY);

            canvas.drawLine(58, cy + 2, 86, cy + 6, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(58, cy + 12, 86, cy + 12, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(182, cy + 2, 154, cy + 6, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(182, cy + 12, 154, cy + 12, HallmarkTheme::TEXT_SECONDARY);

            drawReactionBadge(canvas, col_acc, "Затишний Котик");
        }
        else if (r_type == NOTIFY_FIRE) {
            uint16_t col_org = HallmarkTheme::ACCENT_AMBER;
            uint16_t col_red = HallmarkTheme::ACCENT_DANGER;
            int fy = 114 + bounce;
            canvas.fillCircle(120, fy + 10, 38, col_red);
            canvas.fillTriangle(82, fy + 8, 158, fy + 8, 120, fy - 56, col_red);
            canvas.fillCircle(120, fy + 14, 26, col_org);
            canvas.fillTriangle(94, fy + 12, 146, fy + 12, 120, fy - 36, col_org);
            canvas.fillCircle(120, fy + 18, 14, HallmarkTheme::TEXT_PRIMARY);
            canvas.fillTriangle(106, fy + 18, 134, fy + 18, 120, fy - 14, HallmarkTheme::TEXT_PRIMARY);
            drawReactionBadge(canvas, col_org, "Теплий Вогник");
        }
        else if (r_type == NOTIFY_LIGHTNING) {
            int ly = 108 + bounce;
            canvas.fillTriangle(132, ly - 52, 92, ly + 4, 124, ly + 4, HallmarkTheme::ACCENT_AMBER);
            canvas.fillTriangle(116, ly - 6, 148, ly - 6, 106, ly + 52, HallmarkTheme::ACCENT_AMBER);
            drawReactionBadge(canvas, HallmarkTheme::ACCENT_COOL, "Енергія Світла");
        }
        else if (r_type == NOTIFY_UFO) {
            uint16_t col_grn = HallmarkTheme::ACCENT_SUCCESS;
            int uy = 88 + bounce;
            canvas.fillTriangle(120, uy + 10, 75, 160, 165, 160, HallmarkTheme::BG_SURFACE);
            canvas.fillCircle(120, uy - 6, 22, HallmarkTheme::ACCENT_COOL);
            canvas.fillEllipse(120, uy + 4, 48, 14, HallmarkTheme::BORDER_HAIRLINE);
            canvas.fillCircle(95,  uy + 4, 4, HallmarkTheme::ACCENT_AMBER);
            canvas.fillCircle(120, uy + 6, 4, col_grn);
            canvas.fillCircle(145, uy + 4, 4, HallmarkTheme::ACCENT_AMBER);
            drawReactionBadge(canvas, col_grn, "Космічний Привіт");
        }
        else if (r_type == NOTIFY_CROWN) {
            uint16_t col_gld = HallmarkTheme::ACCENT_AMBER;
            int ky = 118 + bounce;
            canvas.fillRect(76, ky, 88, 18, col_gld);
            canvas.fillTriangle(76, ky, 76, ky - 40, 102, ky, col_gld);
            canvas.fillTriangle(98, ky, 120, ky - 50, 142, ky, col_gld);
            canvas.fillTriangle(138, ky, 164, ky - 40, 164, ky, col_gld);
            canvas.fillCircle(76,  ky - 42, 5, HallmarkTheme::TEXT_PRIMARY);
            canvas.fillCircle(120, ky - 52, 6, HallmarkTheme::ACCENT_DANGER);
            canvas.fillCircle(164, ky - 42, 5, HallmarkTheme::TEXT_PRIMARY);
            drawReactionBadge(canvas, col_gld, "Королівський Настрій");
        }
    }
}
