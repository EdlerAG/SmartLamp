#pragma once
#include <LovyanGFX.hpp>
#include "core/Types.h"
#include "ui/HallmarkTheme.h"

namespace ReactionRenderer {
    inline void draw(LGFX_Sprite& canvas, uint8_t r_type, uint32_t now) {
        int bounce = (int)(sinf(now * 0.012f) * 4.0f);

        if (r_type == NOTIFY_CAT) {
            uint16_t col_acc = HallmarkTheme::ACCENT_WARM;
            canvas.drawArc(120, 120, 116, 112, 0, 360, col_acc);
            int cy = 106 + bounce;

            // Вушка котика
            canvas.fillTriangle(72, cy - 18, 62, cy - 54, 100, cy - 34, col_acc);
            canvas.fillTriangle(75, cy - 24, 69, cy - 46, 94,  cy - 32, HallmarkTheme::BG_SURFACE);
            canvas.fillTriangle(168, cy - 18, 178, cy - 54, 140, cy - 34, col_acc);
            canvas.fillTriangle(165, cy - 24, 171, cy - 46, 146, cy - 32, HallmarkTheme::BG_SURFACE);

            // Мордочка
            canvas.fillCircle(120, cy, 42, col_acc);
            canvas.fillCircle(120, cy, 38, HallmarkTheme::BG_SURFACE);

            // Оченята
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

            // Носик та ротик
            canvas.fillTriangle(116, cy + 4, 124, cy + 4, 120, cy + 9, col_acc);
            canvas.drawLine(120, cy + 9, 114, cy + 15, HallmarkTheme::TEXT_PRIMARY);
            canvas.drawLine(120, cy + 9, 126, cy + 15, HallmarkTheme::TEXT_PRIMARY);

            // Вуса
            canvas.drawLine(58, cy + 2, 86, cy + 6, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(58, cy + 12, 86, cy + 12, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(182, cy + 2, 154, cy + 6, HallmarkTheme::TEXT_SECONDARY);
            canvas.drawLine(182, cy + 12, 154, cy + 12, HallmarkTheme::TEXT_SECONDARY);

            // Підпис
            canvas.fillRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(col_acc);
            canvas.drawString("Затишний Котик ✨", 120, 188);
        }
        else if (r_type == NOTIFY_FIRE) {
            uint16_t col_org = HallmarkTheme::ACCENT_AMBER;
            uint16_t col_red = HallmarkTheme::ACCENT_DANGER;
            canvas.drawArc(120, 120, 116, 112, 0, 360, col_org);

            int fy = 114 + bounce;
            canvas.fillCircle(120, fy + 10, 38, col_red);
            canvas.fillTriangle(82, fy + 8, 158, fy + 8, 120, fy - 56, col_red);
            canvas.fillCircle(120, fy + 14, 26, col_org);
            canvas.fillTriangle(94, fy + 12, 146, fy + 12, 120, fy - 36, col_org);
            canvas.fillCircle(120, fy + 18, 14, HallmarkTheme::TEXT_PRIMARY);
            canvas.fillTriangle(106, fy + 18, 134, fy + 18, 120, fy - 14, HallmarkTheme::TEXT_PRIMARY);

            canvas.fillRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(col_org);
            canvas.drawString("Теплий Вогник 🔥", 120, 188);
        }
        else if (r_type == NOTIFY_LIGHTNING) {
            uint16_t col_el = HallmarkTheme::ACCENT_COOL;
            canvas.drawArc(120, 120, 116, 112, 0, 360, col_el);

            int ly = 108 + bounce;
            canvas.fillTriangle(132, ly - 52, 92, ly + 4, 124, ly + 4, HallmarkTheme::ACCENT_AMBER);
            canvas.fillTriangle(116, ly - 6, 148, ly - 6, 106, ly + 52, HallmarkTheme::ACCENT_AMBER);

            canvas.fillRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(col_el);
            canvas.drawString("Енергія Світла ⚡", 120, 188);
        }
        else if (r_type == NOTIFY_UFO) {
            uint16_t col_grn = HallmarkTheme::ACCENT_SUCCESS;
            canvas.drawArc(120, 120, 116, 112, 0, 360, col_grn);

            int uy = 88 + bounce;
            canvas.fillTriangle(120, uy + 10, 75, 160, 165, 160, HallmarkTheme::BG_SURFACE);
            canvas.fillCircle(120, uy - 6, 22, HallmarkTheme::ACCENT_COOL);
            canvas.fillEllipse(120, uy + 4, 48, 14, HallmarkTheme::BORDER_HAIRLINE);
            canvas.fillCircle(95,  uy + 4, 4, HallmarkTheme::ACCENT_AMBER);
            canvas.fillCircle(120, uy + 6, 4, col_grn);
            canvas.fillCircle(145, uy + 4, 4, HallmarkTheme::ACCENT_AMBER);

            canvas.fillRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(col_grn);
            canvas.drawString("Космічний Привіт 🛸", 120, 188);
        }
        else if (r_type == NOTIFY_CROWN) {
            uint16_t col_gld = HallmarkTheme::ACCENT_AMBER;
            canvas.drawArc(120, 120, 116, 112, 0, 360, col_gld);

            int ky = 118 + bounce;
            canvas.fillRect(76, ky, 88, 18, col_gld);
            canvas.fillTriangle(76, ky, 76, ky - 40, 102, ky, col_gld);
            canvas.fillTriangle(98, ky, 120, ky - 50, 142, ky, col_gld);
            canvas.fillTriangle(138, ky, 164, ky - 40, 164, ky, col_gld);
            canvas.fillCircle(76,  ky - 42, 5, HallmarkTheme::TEXT_PRIMARY);
            canvas.fillCircle(120, ky - 52, 6, HallmarkTheme::ACCENT_DANGER);
            canvas.fillCircle(164, ky - 42, 5, HallmarkTheme::TEXT_PRIMARY);

            canvas.fillRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BG_SURFACE);
            canvas.drawRoundRect(36, 176, 168, 24, 12, HallmarkTheme::BORDER_HAIRLINE);
            canvas.setFont(&fonts::efontCN_12);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(col_gld);
            canvas.drawString("Королівський Настрій 👑", 120, 188);
        }
    }
}
