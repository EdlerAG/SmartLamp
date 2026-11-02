#pragma once
#include <stdint.h>

/**
 * ============================================================================
 * HallmarkTheme (Dieter Rams Braun horology & Hallmark Atelier design tokens)
 * Pure tokenized RGB565 palette, zero acidic gradients, surgical typography.
 * Luxury Swiss Horology & Apple Watch Ultra design aesthetic.
 * ============================================================================
 */
namespace HallmarkTheme {
    constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
    }

    constexpr uint16_t BG_PRIMARY      = rgb(0, 0, 0);       // #000000 Pure Luxury OLED Black
    constexpr uint16_t BG_SURFACE      = rgb(18, 20, 26);    // #12141A Elevated Swiss Graphite Pill
    constexpr uint16_t BORDER_HAIRLINE = rgb(38, 42, 54);    // #262A36 Subtle 1px Border
    constexpr uint16_t BORDER_ACTIVE   = rgb(65, 75, 95);    // #414B5F Active / Focus border
    constexpr uint16_t TICK_NORMAL     = rgb(58, 53, 47);    // #3A352F Warm Horology Tick
    constexpr uint16_t TICK_MAJOR      = rgb(92, 84, 75);    // #5C544B 5-minute Major Tick
    constexpr uint16_t TEXT_PRIMARY    = rgb(250, 246, 240); // #FAF6F0 Warm Cream
    constexpr uint16_t TEXT_SECONDARY  = rgb(138, 148, 166); // #8A94A6 Muted Ash
    constexpr uint16_t ACCENT_WARM     = rgb(224, 122, 95);  // #E07A5F Braun Terracotta
    constexpr uint16_t ACCENT_COOL     = rgb(56, 189, 248);  // #38BDF8 Soft Sky / Cyan
    constexpr uint16_t ACCENT_SUCCESS  = rgb(82, 183, 136);  // #52B788 Sage Green / Emerald
    constexpr uint16_t ACCENT_DANGER   = rgb(230, 57, 70);   // #E63946 Matte Coral
    constexpr uint16_t ACCENT_AMBER    = rgb(245, 158, 11);  // #F59E0B Warm Amber
}
