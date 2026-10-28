#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"
#include "core/Types.h"

namespace LedEffects {
    inline void renderNotification(
        Adafruit_NeoPixel& strip,
        uint8_t type,
        uint16_t frame,
        uint8_t ambi_r = 56,
        uint8_t ambi_g = 189,
        uint8_t ambi_b = 248
    ) {
        uint8_t wave = strip.sine8(frame * 10);
        for (int i = 0; i < Config::NUM_LEDS; i++) {
            if (type == NOTIFY_AMBILIGHT) {
                uint8_t pulse = 135 + (strip.sine8((frame * 7) + (i * 12)) >> 1);
                uint8_t r = ((uint16_t)ambi_r * pulse) >> 8;
                uint8_t g = ((uint16_t)ambi_g * pulse) >> 8;
                uint8_t b = ((uint16_t)ambi_b * pulse) >> 8;
                strip.setPixelColor(i, strip.Color(r, g, b, (pulse > 225) ? 25 : 0));
            } else if (type == NOTIFY_FIRE) {
                uint8_t spark = strip.sine8((frame * 19) + (i * 23));
                strip.setPixelColor(i, strip.Color(200 + (spark >> 3), (spark * 110) >> 8, 0, (spark > 210) ? 60 : 0));
            } else if (type == NOTIFY_LIGHTNING) {
                bool flash = ((frame % 14 == 0) || (frame % 14 == 2));
                if (flash) strip.setPixelColor(i, strip.Color(80, 160, 255, 220));
                else {
                    uint8_t glow = strip.sine8((frame * 9) + (i * 14));
                    strip.setPixelColor(i, strip.Color((glow * 40) >> 8, (glow * 90) >> 8, glow, 0));
                }
            } else if (type == NOTIFY_CAT) {
                uint8_t paw = strip.sine8((frame * 8) + (i * 18));
                if ((i / 6) % 2 == 0) strip.setPixelColor(i, strip.Color(0, paw, (paw * 170) >> 8, (paw * 40) >> 8));
                else strip.setPixelColor(i, strip.Color(paw, (paw * 140) >> 8, 0, (paw * 30) >> 8));
            } else if (type == NOTIFY_UFO) {
                uint8_t beam = strip.sine8((frame * 14) - (i * 16));
                strip.setPixelColor(i, strip.Color((255 - beam) >> 2, beam, (255 - beam) >> 1, 0));
            } else if (type == NOTIFY_CROWN) {
                uint8_t sh = strip.sine8((frame * 10) + (i * 20));
                strip.setPixelColor(i, strip.Color(220, (sh * 160) >> 8, (255 - sh) >> 1, (sh > 200) ? 80 : 10));
            } else {
                strip.setPixelColor(i, strip.Color(0, (wave * 180) >> 8, wave, (wave * 70) >> 8));
            }
        }
    }

    inline void renderDynamicMode(
        Adafruit_NeoPixel& strip,
        uint8_t mode,
        uint16_t step,
        uint8_t ambi_r = 56,
        uint8_t ambi_g = 189,
        uint8_t ambi_b = 248
    ) {
        switch (mode) {
            case MODE_CANDLE: {
                uint8_t wind = strip.sine8(step * 2);
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t flk1 = strip.sine8((step * 5) + (i * 9));
                    uint8_t flk2 = strip.sine8((step * 11) - (i * 5) + (wind >> 2));
                    uint8_t flame = ((uint16_t)flk1 + (uint16_t)flk2) >> 1;
                    strip.setPixelColor(i, strip.Color(180 + ((flame * 75) >> 8), 45 + ((flame * 65) >> 8), 0, 35 + ((flame * 95) >> 8)));
                }
                break;
            }
            case MODE_AMBER_EMBER: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t breath = strip.sine8((step * 2) + (i * 4));
                    strip.setPixelColor(i, strip.Color(150 + ((breath * 105) >> 8), 25 + ((breath * 55) >> 8), 0, 10 + ((breath * 45) >> 8)));
                }
                break;
            }
            case MODE_FIREPLACE: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t f1 = strip.sine8((step * 5) + (i * 17));
                    uint8_t f2 = strip.sine8((step * 9) - (i * 11));
                    uint8_t flk = (f1 + f2) >> 1;
                    strip.setPixelColor(i, strip.Color(170 + ((flk * 85) >> 8), 15 + ((flk * 80) >> 8), 0, (flk > 200) ? ((flk - 200) >> 1) : 0));
                }
                break;
            }
            case MODE_SUNSET_VIBE: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t wave = strip.sine8((step * 2) + (i * 6));
                    strip.setPixelColor(i, strip.Color(190 + ((wave * 65) >> 8), (wave * 70) >> 8, ((255 - wave) * 110) >> 8, (wave * 35) >> 8));
                }
                break;
            }
            case MODE_SAKURA_NEON: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t wv = strip.sine8((step * 2) + (i * 8));
                    strip.setPixelColor(i, strip.Color(130 + ((wv * 125) >> 8), ((255 - wv) * 140) >> 8, 120 + ((wv * 110) >> 8), (wv > 190) ? 25 : 5));
                }
                break;
            }
            case MODE_AURORA_WAVE: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t s1 = strip.sine8((step * 2) + (i * 7));
                    uint8_t s2 = strip.sine8(64 - (step * 2) + (i * 5));
                    strip.setPixelColor(i, strip.Color(0, 15 + ((s1 * 215) >> 8), 40 + ((s2 * 210) >> 8), (s1 > 180) ? ((s1 - 180) >> 1) : 0));
                }
                break;
            }
            case MODE_OCEAN_BREEZE: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t tide = strip.sine8((step * 3) + (i * 6));
                    uint8_t foam = (tide > 215) ? ((tide - 215) * 3) : 0;
                    strip.setPixelColor(i, strip.Color(0, (tide * 165) >> 8, 80 + ((tide * 175) >> 8), foam));
                }
                break;
            }
            case MODE_CYBER_RUNNER: {
                uint32_t period = (uint32_t)Config::NUM_LEDS * 100;
                uint32_t head_pos = ((uint32_t)step * 64) % period;
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint32_t dist = (uint32_t)abs((int32_t)(i * 100) - (int32_t)head_pos);
                    if (dist > period / 2) dist = period - dist;
                    uint8_t intensity = (dist < 800) ? (uint8_t)(255 - (dist * 255) / 800) : 0;
                    uint8_t sq = (intensity * intensity) >> 8;
                    strip.setPixelColor(i, strip.Color((sq * 190) >> 8, ((sq * 45) >> 8) + 6, ((sq * 235) >> 8) + 20, 0));
                }
                break;
            }
            case MODE_RAINBOW_FLOW: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint16_t hue = (step * 220 + (i * 65536L / Config::NUM_LEDS)) & 0xFFFF;
                    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, 255)));
                }
                break;
            }
            case MODE_SPOTIFY_AMBI: {
                uint8_t breath = strip.sine8(step * 2);
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t wave = strip.sine8((step * 3) + (i * 8));
                    uint8_t pulse = 160 + (wave >> 2);
                    uint8_t r = ((uint16_t)ambi_r * pulse) >> 8;
                    uint8_t g = ((uint16_t)ambi_g * pulse) >> 8;
                    uint8_t b = ((uint16_t)ambi_b * pulse) >> 8;
                    strip.setPixelColor(i, strip.Color(r, g, b, (breath > 235) ? 18 : 0));
                }
                break;
            }
        }
    }
}
