#pragma once
#include <Arduino.h>
#include <NeoPixelBus.h>
#include "Config.h"
#include "core/Types.h"

// Апаратний тип смуги SK6812 RGBW (GRBW) на базі апаратного RMT ESP32-S3
typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Sk6812Method> SmartLampStrip;

namespace LedEffects {

    // 1. Точна 8-бітна LUT гама-корекція: gamma = 2.8 для насичених каналів R, G, B
    const uint8_t PROGMEM gamma28_rgb_lut[256] = {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
          1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
          2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,
          5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,
         10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,
         17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
         25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,
         37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,
         51,  52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,
         69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,
         90,  92,  93,  95,  96,  98,  99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
        115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
        144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
        177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
        215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255,
    };

    // 2. Перцептивна лінійна крива для білого діода W (gamma ~2.2)
    const uint8_t PROGMEM gamma_w_lut[256] = {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
          1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
          3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
          6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
         12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
         20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
         30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
         42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
         56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
         73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
         91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
        113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
        137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
        163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
        192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
        223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255,
    };

    // Проміжний сирий буфер кольорів на кадр перед гамою та лімітером струму
    struct PixelBuffer {
        float r[Config::NUM_LEDS];
        float g[Config::NUM_LEDS];
        float b[Config::NUM_LEDS];
        float w[Config::NUM_LEDS];

        void clear() {
            memset(r, 0, sizeof(r));
            memset(g, 0, sizeof(g));
            memset(b, 0, sizeof(b));
            memset(w, 0, sizeof(w));
        }

        void set(int i, float cr, float cg, float cb, float cw) {
            if (i >= 0 && i < Config::NUM_LEDS) {
                r[i] = cr; g[i] = cg; b[i] = cb; w[i] = cw;
            }
        }

        // Субодиничне накладання кольору (Anti-aliasing subpixel blending)
        void addSubpixel(float pos, float cr, float cg, float cb, float cw) {
            while (pos < 0.0f) pos += (float)Config::NUM_LEDS;
            while (pos >= (float)Config::NUM_LEDS) pos -= (float)Config::NUM_LEDS;

            int i0 = (int)pos;
            int i1 = (i0 + 1) % Config::NUM_LEDS;
            float f = pos - (float)i0;
            float w0 = 1.0f - f;
            float w1 = f;

            r[i0] += cr * w0; g[i0] += cg * w0; b[i0] += cb * w0; w[i0] += cw * w0;
            r[i1] += cr * w1; g[i1] += cg * w1; b[i1] += cb * w1; w[i1] += cw * w1;
        }
    };

    static PixelBuffer stageBuf;

    // Швидка функція синуса 0..255
    inline float fastSin(float rad) {
        return sinf(rad);
    }

    // Застосування Smart Power Budget Limiter та Gamma LUT у вихідний RMT буфер
    inline void commitFrameWithBudgetAndGamma(SmartLampStrip& strip, const PixelBuffer& buf, float brightness_factor) {
        constexpr float MAX_SAFE_MA = 2200.0f;
        constexpr float QUIESCENT_MA = 60.0f; // 1мА на чип у спокої
        constexpr float MA_PER_CHANNEL = 15.0f; // 15мА на кристал при 255

        float scaled_r[Config::NUM_LEDS];
        float scaled_g[Config::NUM_LEDS];
        float scaled_b[Config::NUM_LEDS];
        float scaled_w[Config::NUM_LEDS];

        float sum_channels = 0.0f;
        for (int i = 0; i < Config::NUM_LEDS; i++) {
            scaled_r[i] = constrain(buf.r[i] * brightness_factor, 0.0f, 255.0f);
            scaled_g[i] = constrain(buf.g[i] * brightness_factor, 0.0f, 255.0f);
            scaled_b[i] = constrain(buf.b[i] * brightness_factor, 0.0f, 255.0f);
            scaled_w[i] = constrain(buf.w[i] * brightness_factor, 0.0f, 255.0f);

            sum_channels += (scaled_r[i] + scaled_g[i] + scaled_b[i] + scaled_w[i]);
        }

        float estimated_ma = QUIESCENT_MA + (sum_channels * (MA_PER_CHANNEL / 255.0f));
        float current_scale = 1.0f;
        if (estimated_ma > MAX_SAFE_MA) {
            current_scale = (MAX_SAFE_MA - QUIESCENT_MA) / (estimated_ma - QUIESCENT_MA);
        }

        for (int i = 0; i < Config::NUM_LEDS; i++) {
            uint8_t ri = (uint8_t)constrain((int)(scaled_r[i] * current_scale + 0.5f), 0, 255);
            uint8_t gi = (uint8_t)constrain((int)(scaled_g[i] * current_scale + 0.5f), 0, 255);
            uint8_t bi = (uint8_t)constrain((int)(scaled_b[i] * current_scale + 0.5f), 0, 255);
            uint8_t wi = (uint8_t)constrain((int)(scaled_w[i] * current_scale + 0.5f), 0, 255);

            uint8_t r_out = pgm_read_byte(&gamma28_rgb_lut[ri]);
            uint8_t g_out = pgm_read_byte(&gamma28_rgb_lut[gi]);
            uint8_t b_out = pgm_read_byte(&gamma28_rgb_lut[bi]);
            uint8_t w_out = pgm_read_byte(&gamma_w_lut[wi]);

            strip.SetPixelColor(i, RgbwColor(r_out, g_out, b_out, w_out));
        }
    }

    // Рендер сповіщень поверх активного стану
    inline void renderNotification(
        SmartLampStrip& strip,
        uint8_t type,
        uint16_t frame,
        float brightness_factor,
        uint8_t ambi_r = 56,
        uint8_t ambi_g = 189,
        uint8_t ambi_b = 248
    ) {
        stageBuf.clear();
        float t = (float)frame * 0.1f;
        for (int i = 0; i < Config::NUM_LEDS; i++) {
            float phase = t + (float)i * 0.2f;
            float wave = (sinf(phase) + 1.0f) * 0.5f;

            if (type == NOTIFY_AMBILIGHT) {
                float pulse = 0.5f + 0.5f * sinf(t * 1.5f + (float)i * 0.15f);
                stageBuf.set(i, (float)ambi_r * pulse, (float)ambi_g * pulse, (float)ambi_b * pulse, (pulse > 0.85f) ? 20.0f : 0.0f);
            } else if (type == NOTIFY_FIRE) {
                float spark = (sinf(phase * 2.3f) + 1.0f) * 0.5f;
                stageBuf.set(i, 200.0f + spark * 55.0f, spark * 95.0f, 0.0f, (spark > 0.8f) ? 45.0f : 0.0f);
            } else if (type == NOTIFY_LIGHTNING) {
                bool flash = ((frame % 14 == 0) || (frame % 14 == 2));
                if (flash) stageBuf.set(i, 70.0f, 150.0f, 255.0f, 220.0f);
                else {
                    float glow = (sinf(phase) + 1.0f) * 0.5f;
                    stageBuf.set(i, glow * 30.0f, glow * 70.0f, glow * 180.0f, 0.0f);
                }
            } else if (type == NOTIFY_CAT) {
                float paw = (sinf(phase * 1.2f) + 1.0f) * 0.5f;
                if ((i / 6) % 2 == 0) stageBuf.set(i, 0.0f, paw * 180.0f, paw * 70.0f, paw * 30.0f);
                else stageBuf.set(i, paw * 220.0f, paw * 120.0f, 0.0f, paw * 20.0f);
            } else if (type == NOTIFY_UFO) {
                float beam = (sinf(phase * 1.8f - (float)i * 0.3f) + 1.0f) * 0.5f;
                stageBuf.set(i, (1.0f - beam) * 60.0f, beam * 240.0f, (1.0f - beam) * 120.0f, 0.0f);
            } else if (type == NOTIFY_CROWN) {
                float sh = (sinf(phase * 1.5f) + 1.0f) * 0.5f;
                stageBuf.set(i, 220.0f, sh * 160.0f, (1.0f - sh) * 110.0f, (sh > 0.8f) ? 80.0f : 10.0f);
            } else {
                stageBuf.set(i, 0.0f, wave * 160.0f, wave * 240.0f, wave * 40.0f);
            }
        }
        commitFrameWithBudgetAndGamma(strip, stageBuf, brightness_factor);
    }

    // Головний рендер 12 режимів світла
    inline void renderDynamicMode(
        SmartLampStrip& strip,
        uint8_t mode,
        uint32_t step,
        float brightness_factor,
        uint8_t anim_speed = 50,
        uint8_t ambi_r = 56,
        uint8_t ambi_g = 189,
        uint8_t ambi_b = 248,
        uint8_t ambi2_r = 248,
        uint8_t ambi2_g = 113,
        uint8_t ambi2_b = 113,
        const SpotifyTrackInfo* sp = nullptr,
        uint8_t custom_r = 255,
        uint8_t custom_g = 180,
        uint8_t custom_b = 80,
        uint8_t custom_w = 120
    ) {
        stageBuf.clear();
        float speed_scale = constrain((float)anim_speed / 50.0f, 0.1f, 2.5f);
        float t = (float)step * 0.05f * speed_scale;

        switch (mode) {
            case MODE_WARM_COZY: {
                // ☀️ Теплий затишок: нативний теплий W (2700K) + делікатне R та G для золотистого тепла
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, 90.0f, 32.0f, 0.0f, 255.0f);
                }
                break;
            }

            case MODE_CANDLE: {
                // 🕯 Жива свічка: плавне мерехтіння нативного кристала W + 2200K помаранчевий колір
                float wind = sinf(t * 0.4f) * 0.25f;
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float f1 = sinf(t * 1.8f + (float)i * 0.45f);
                    float f2 = sinf(t * 3.7f - (float)i * 0.25f + wind);
                    float flame = (f1 + f2) * 0.5f; // -1.0 .. 1.0
                    float flame_norm = (flame + 1.0f) * 0.5f; // 0.0 .. 1.0

                    float w = 110.0f + flame_norm * 140.0f;
                    float r = 210.0f + flame_norm * 45.0f;
                    float g = 65.0f + flame_norm * 40.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_AMBER_EMBER: {
                // 🍯 Бурштинова свічка: м'яке басове дихання вуглинок
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float breath = (sinf(t * 0.7f + (float)i * 0.18f) + 1.0f) * 0.5f;
                    float w = 35.0f + breath * 75.0f;
                    float r = 230.0f + breath * 25.0f;
                    float g = 75.0f + breath * 45.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_FIREPLACE: {
                // 🔥 Камін з іскрами: субодиничні іскри та глибоке полум'я
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float f1 = sinf(t * 1.9f + (float)i * 0.7f);
                    float f2 = sinf(t * 3.3f - (float)i * 0.45f);
                    float flk = (f1 + f2) * 0.5f;
                    float flk_norm = (flk + 1.0f) * 0.5f;

                    float w = (flk_norm > 0.82f) ? ((flk_norm - 0.82f) * 500.0f) : 0.0f;
                    float r = 180.0f + flk_norm * 75.0f;
                    float g = 25.0f + flk_norm * 70.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_SUNSET_VIBE: {
                // 🌅 Захід сонця: насичений перелив червоно-рожевого у глибокий пурпур (W = 0)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float wave = (sinf(t * 0.8f + (float)i * 0.22f) + 1.0f) * 0.5f;
                    float r = 210.0f + wave * 45.0f;
                    float g = wave * 65.0f;
                    float b = (1.0f - wave) * 140.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_SAKURA_NEON: {
                // 🌸 Сакура Неон: чистий яскравий неоновий рожево-бірюзовий спектр (W = 0)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float wv = (sinf(t * 0.9f + (float)i * 0.28f) + 1.0f) * 0.5f;
                    float r = 160.0f + wv * 95.0f;
                    float g = (1.0f - wv) * 110.0f;
                    float b = 150.0f + wv * 105.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_AURORA_WAVE: {
                // 🌌 Північне сяйво: смарагдово-зелений та містичний індиго (W = 0)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float s1 = (sinf(t * 0.7f + (float)i * 0.24f) + 1.0f) * 0.5f;
                    float s2 = (sinf(1.5f - t * 0.6f + (float)i * 0.19f) + 1.0f) * 0.5f;
                    float r = 0.0f;
                    float g = 25.0f + s1 * 230.0f;
                    float b = 50.0f + s2 * 205.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_OCEAN_BREEZE: {
                // 🌊 Океанський бриз: глибокий бірюзовий океан з легкою білою піною на гребені
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float tide = (sinf(t * 0.85f + (float)i * 0.21f) + 1.0f) * 0.5f;
                    float foam = (tide > 0.85f) ? ((tide - 0.85f) * 600.0f) : 0.0f;
                    float g = tide * 180.0f;
                    float b = 80.0f + tide * 175.0f;
                    stageBuf.set(i, 0.0f, g, b, foam);
                }
                break;
            }

            case MODE_CYBER_RUNNER: {
                // ⚡ Кібер-комета: субодиничний рух (Fractional anti-aliasing) без дискретних стрибків
                float speed_px_per_sec = 24.0f * speed_scale;
                float head_pos = fmodf((float)step * (0.02f * speed_px_per_sec), (float)Config::NUM_LEDS);
                constexpr float TAIL_LEN = 16.0f;

                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float dist = (float)i - head_pos;
                    while (dist < 0.0f) dist += (float)Config::NUM_LEDS;
                    while (dist >= (float)Config::NUM_LEDS) dist -= (float)Config::NUM_LEDS;

                    // Рух по колу вперед: хвіст тягнеться позаду (відстань від NUM_LEDS-TAIL до NUM_LEDS)
                    float tail_dist = (float)Config::NUM_LEDS - dist;
                    float intensity = 0.0f;
                    if (tail_dist <= TAIL_LEN) {
                        intensity = tail_dist / TAIL_LEN;
                    }

                    float sq = intensity * intensity;
                    float r = sq * 200.0f;
                    float g = sq * 40.0f + 2.0f;
                    float b = sq * 255.0f + 12.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_RAINBOW_FLOW: {
                // 🌈 Веселка: плавний плазмовий спектр (W = 0)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float hue = fmodf(t * 0.4f + ((float)i / (float)Config::NUM_LEDS), 1.0f) * 6.0f;
                    int hi = (int)hue;
                    float f = hue - (float)hi;
                    float p = 0.0f;
                    float q = 1.0f - f;
                    float r = 0, g = 0, b = 0;
                    switch (hi % 6) {
                        case 0: r = 1.0f; g = f;    b = p;    break;
                        case 1: r = q;    g = 1.0f; b = p;    break;
                        case 2: r = p;    g = 1.0f; b = f;    break;
                        case 3: r = p;    g = q;    b = 1.0f; break;
                        case 4: r = f;    g = p;    b = 1.0f; break;
                        case 5: r = 1.0f; g = p;    b = q;    break;
                    }
                    stageBuf.set(i, r * 255.0f, g * 255.0f, b * 255.0f, 0.0f);
                }
                break;
            }

            case MODE_LIGHT_OFF: {
                // 🌑 Вимкнено
                stageBuf.clear();
                break;
            }

            case MODE_SPOTIFY_AMBI: {
                // 🎵 Spotify AmbiLight: фізичний шейдер Underglow з прив'язкою до GC9A01
                uint32_t now = millis();
                bool is_playing = (sp && sp->playing);
                uint16_t raw_bpm = (sp && sp->bpm >= 40 && sp->bpm <= 240) ? sp->bpm : 120;
                float track_energy = (sp && sp->energy > 0) ? ((float)sp->energy / 100.0f) : 0.55f;
                track_energy = constrain(track_energy, 0.20f, 1.00f);
                bool is_mono = sp ? sp->is_mono : false;

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

                // Half-Time Groove (якщо bpm > 122)
                float eff_bpm = (raw_bpm > 122) ? ((float)raw_bpm * 0.5f) : (float)raw_bpm;
                float beat_period_ms = 60000.0f / eff_bpm;
                float swell = 0.0f;

                if (is_playing) {
                    float beat_phase = fmodf((float)song_ms, beat_period_ms) / beat_period_ms;
                    float s = sinf(beat_phase * 3.14159265f);
                    swell = s * s * s * s; // sin^4 еластичний кач
                } else {
                    float breath_phase = fmodf((float)now, 3800.0f) / 3800.0f;
                    float s = sinf(breath_phase * 3.14159265f);
                    swell = s * s;
                }

                float t_sec = (float)song_ms * 0.001f;
                if (!is_playing) t_sec = (float)now * 0.0003f;
                float speed_mult = is_playing ? (0.35f + 0.45f * track_energy) : 0.20f;
                float phi1 = t_sec * 0.73f * speed_mult;
                float phi2 = t_sec * 1.17f * speed_mult;
                float phi3 = t_sec * 0.43f * speed_mult;

                float expand = swell * (0.08f + 0.12f * track_energy);
                float beat_boost = is_playing ? (swell * (0.15f + 0.20f * track_energy)) : (swell * 0.10f);

                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float theta = (2.0f * 3.14159265f * (float)i) / (float)Config::NUM_LEDS;
                    float cos_th = cosf(theta);

                    float wave1 = sinf(theta * 1.0f + phi1);
                    float wave2 = sinf(theta * 2.0f - phi2);
                    float wave3 = sinf(theta * 3.0f + phi3);
                    float wave_disp = 0.20f * wave1 + 0.12f * wave2 + 0.08f * wave3;

                    float coord = cos_th + wave_disp + expand;

                    // cos(theta) > 0.2 — передня дуга прямо під екраном GC9A01
                    float t_blend = (coord - (-0.15f)) / (0.35f - (-0.15f));
                    t_blend = constrain(t_blend, 0.0f, 1.0f);
                    float blend = t_blend * t_blend * (3.0f - 2.0f * t_blend);

                    // Провал яскравості (mid_dip до 28%) між зонами
                    float dist_from_boundary = fabsf(blend - 0.5f) * 2.0f;
                    float spatial_luma = 0.28f + 0.72f * (dist_from_boundary * dist_from_boundary);
                    float total_scale = spatial_luma * (1.0f + beat_boost);

                    float r_lin = ((float)ambi_r * blend + (float)ambi2_r * (1.0f - blend)) * total_scale;
                    float g_lin = ((float)ambi_g * blend + (float)ambi2_g * (1.0f - blend)) * total_scale;
                    float b_lin = ((float)ambi_b * blend + (float)ambi2_b * (1.0f - blend)) * total_scale;

                    // Суворе W = 0 для кольору, теплий акцент W = 4..8 лише для монохрому
                    float w_lin = 0.0f;
                    if (is_mono) {
                        w_lin = 16.0f * spatial_luma * (1.0f + beat_boost * 0.5f);
                        w_lin = constrain(w_lin, 4.0f, 8.0f);
                    }

                    stageBuf.set(i, r_lin, g_lin, b_lin, w_lin);
                }
                break;
            }

            default: {
                // Режим за замовчуванням: плавна тепла підсвітка
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, (float)custom_r, (float)custom_g, (float)custom_b, (float)custom_w);
                }
                break;
            }
        }

        commitFrameWithBudgetAndGamma(strip, stageBuf, brightness_factor);
    }
}
