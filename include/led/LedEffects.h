#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"
#include "core/Types.h"

namespace LedEffects {
    static inline uint8_t ambiGamma8(float v) {
        if (v <= 0.0f) return 0;
        if (v >= 255.0f) return 255;
        float norm = v * (1.0f / 255.0f);
        float g = norm * norm * sqrtf(norm); // gamma ~2.5
        return (uint8_t)(g * 255.0f + 0.5f);
    }

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
        uint8_t ambi_b = 248,
        uint8_t ambi2_r = 248,
        uint8_t ambi2_g = 113,
        uint8_t ambi2_b = 113,
        const SpotifyTrackInfo* sp = nullptr
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
                uint32_t now = millis();
                bool is_playing = (sp && sp->playing);
                uint16_t raw_bpm = (sp && sp->bpm >= 40 && sp->bpm <= 240) ? sp->bpm : 120;
                float track_energy = (sp && sp->energy > 0) ? ((float)sp->energy / 100.0f) : 0.55f;
                track_energy = constrain(track_energy, 0.20f, 1.00f);
                bool is_mono = sp ? sp->is_mono : false;

                // 1. Час пісні з урахуванням безперервного локального прогресу без мережевого джиттера
                uint32_t song_ms = now;
                if (sp && is_playing) {
                    if (sp->last_sync_millis > 0) {
                        song_ms = sp->last_server_progress_ms + (now - sp->last_sync_millis);
                    } else {
                        song_ms = sp->progress_ms;
                    }
                } else if (sp) {
                    song_ms = sp->progress_ms;
                }

                // 2. М\'який басовий грув (Half-Time Sinusoidal Swell)
                // Якщо bpm > 122, ділимо темп навпіл (effective_bpm = bpm * 0.5f), переходячи в Half-Time Groove
                float eff_bpm = (raw_bpm > 122) ? ((float)raw_bpm * 0.5f) : (float)raw_bpm;
                float beat_period_ms = 60000.0f / eff_bpm;
                float swell = 0.0f;

                if (is_playing) {
                    float beat_phase = fmodf((float)song_ms, beat_period_ms) / beat_period_ms; // 0.0 .. 1.0
                    float s = sinf(beat_phase * 3.14159265f);
                    swell = s * s * s * s; // sin^4 еластичний кач
                } else {
                    // Спокійне медитативне дихання в паузі (~3.8 с цикл)
                    float breath_phase = fmodf((float)now, 3800.0f) / 3800.0f;
                    float s = sinf(breath_phase * 3.14159265f);
                    swell = s * s;
                }

                // 3. Стоячі плазмові хвилі (Fluid Lava)
                float t_sec = (float)song_ms * 0.001f;
                if (!is_playing) t_sec = (float)now * 0.0003f;
                float speed_mult = is_playing ? (0.35f + 0.45f * track_energy) : 0.20f;
                float phi1 = t_sec * 0.73f * speed_mult;
                float phi2 = t_sec * 1.17f * speed_mult;
                float phi3 = t_sec * 0.43f * speed_mult;

                // На ударі біта пляма кольору переднього екрана м\'яко розширюється вшир
                float expand = swell * (0.08f + 0.12f * track_energy);
                // Дихаюча яскравість (+15..35% на ударі)
                float beat_boost = is_playing ? (swell * (0.15f + 0.20f * track_energy)) : (swell * 0.10f);

                // 4. Просторове зонування (Front-Locked Halo) по кільцю світлодіодів
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float theta = (2.0f * 3.14159265f * (float)i) / (float)Config::NUM_LEDS;
                    float cos_th = cosf(theta);

                    // 3 некратні повільні стоячі хвилі, що зміщують межу кольорів вперед-назад
                    float wave1 = sinf(theta * 1.0f + phi1);
                    float wave2 = sinf(theta * 2.0f - phi2);
                    float wave3 = sinf(theta * 3.0f + phi3);
                    float wave_disp = 0.20f * wave1 + 0.12f * wave2 + 0.08f * wave3;

                    // Модульована координата вздовж передньо-задньої осі
                    float coord = cos_th + wave_disp + expand;

                    // Плавний перехід між передньою зоною col1 та боковими/задньою col2
                    // cos(theta) > 0.2 — передня дуга прямо під екраном GC9A01
                    float t = (coord - (-0.15f)) / (0.35f - (-0.15f));
                    t = constrain(t, 0.0f, 1.0f);
                    float blend = t * t * (3.0f - 2.0f * t); // smoothstep

                    // Просторовий рельєф яскравості (spatial_luma):
                    // Між зоною col1 і col2 формуємо 2 темніші долини (провал mid_dip до 28%),
                    // щоб обидва кольори читалися на столі роздільно й об\'ємно
                    float dist_from_boundary = fabsf(blend - 0.5f) * 2.0f; // 0.0 на межі, 1.0 в глибині
                    float spatial_luma = 0.28f + 0.72f * (dist_from_boundary * dist_from_boundary);

                    float total_scale = spatial_luma * (1.0f + beat_boost);

                    // Інтерполяція RGB
                    float r_lin = ((float)ambi_r * blend + (float)ambi2_r * (1.0f - blend)) * total_scale;
                    float g_lin = ((float)ambi_g * blend + (float)ambi2_g * (1.0f - blend)) * total_scale;
                    float b_lin = ((float)ambi_b * blend + (float)ambi2_b * (1.0f - blend)) * total_scale;

                    // Перцептивна гамма-корекція gamma ~2.5
                    uint8_t r_out = ambiGamma8(r_lin);
                    uint8_t g_out = ambiGamma8(g_lin);
                    uint8_t b_out = ambiGamma8(b_lin);

                    // Повне блокування білого каналу W для кольорових обкладинок (W = 0)
                    // Тільки для монохромних обкладинок дозволяємо ніжний бурштиново-перлинний W = 4..8
                    uint8_t w_out = 0;
                    if (is_mono) {
                        float w_val = 16.0f * spatial_luma * (1.0f + beat_boost * 0.5f);
                        w_out = (uint8_t)constrain((int)ambiGamma8(w_val), 4, 8);
                    }

                    strip.setPixelColor(i, strip.Color(r_out, g_out, b_out, w_out));
                }
                break;
            }
        }
    }
}
