#pragma once
#include <stdint.h>

/**
 * ============================================================================
 * HallmarkTheme (Dieter Rams Braun horology & Hallmark Atelier design tokens)
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
