#pragma once
#include <Arduino.h>
#include <NeoPixelBus.h>
#include "Config.h"
#include "core/Types.h"

// Hardware SK6812 RGBW (GRBW) on isolated ESP32-S3 RMT0
typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Sk6812Method> SmartLampStrip;

namespace LedEffects {

    // 1. Fast 8-bit LUT Gamma = 2.8 for R, G, B
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

    // 2. Perceptual Gamma = 2.2 LUT for W channel
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

    struct HsvColor {
        float h; // 0..1
        float s; // 0..1
        float v; // 0..1
    };

    inline HsvColor rgb2hsv(float r, float g, float b) {
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float max_c = max(rf, max(gf, bf));
        float min_c = min(rf, min(gf, bf));
        float delta = max_c - min_c;

        HsvColor out{0.0f, 0.0f, max_c};
        if (max_c > 1e-5f) {
            out.s = delta / max_c;
        } else {
            out.s = 0.0f;
            out.h = 0.0f;
            return out;
        }

        if (delta < 1e-5f) {
            out.h = 0.0f;
            return out;
        }

        if (max_c == rf) {
            out.h = (gf - bf) / delta;
            if (out.h < 0.0f) out.h += 6.0f;
        } else if (max_c == gf) {
            out.h = ((bf - rf) / delta) + 2.0f;
        } else {
            out.h = ((rf - gf) / delta) + 4.0f;
        }
        out.h /= 6.0f;
        return out;
    }

    inline void hsv2rgb(float h, float s, float v, float& r, float& g, float& b) {
        h = fmodf(h, 1.0f);
        if (h < 0.0f) h += 1.0f;
        s = constrain(s, 0.0f, 1.0f);
        v = constrain(v, 0.0f, 1.0f);

        if (s <= 1e-5f) {
            r = g = b = v * 255.0f;
            return;
        }

        float var_h = h * 6.0f;
        int i = (int)var_h;
        float f = var_h - (float)i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        float ro = 0, go = 0, bo = 0;
        switch (i % 6) {
            case 0: ro = v; go = t; bo = p; break;
            case 1: ro = q; go = v; bo = p; break;
            case 2: ro = p; go = v; bo = t; break;
            case 3: ro = p; go = q; bo = v; break;
            case 4: ro = t; go = p; bo = v; break;
            case 5: ro = v; go = p; bo = q; break;
        }
        r = ro * 255.0f;
        g = go * 255.0f;
        b = bo * 255.0f;
    }

    // Cosine interpolation in HSV space: 100% pure saturated colors, zero muddy midpoints!
    inline void interpolateHsv(const HsvColor& c1, const HsvColor& c2, float t, float& r, float& g, float& b) {
        t = constrain(t, 0.0f, 1.0f);
        float ease = (1.0f - cosf(t * 3.14159265f)) * 0.5f;

        float dh = c2.h - c1.h;
        if (dh > 0.5f) dh -= 1.0f;
        if (dh < -0.5f) dh += 1.0f;

        float h = fmodf(c1.h + dh * ease + 1.0f, 1.0f);
        float s = c1.s + (c2.s - c1.s) * ease;
        float v = c1.v + (c2.v - c1.v) * ease;

        hsv2rgb(h, s, v, r, g, b);
    }

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
    };

    static PixelBuffer stageBuf;

    // Smart Power Budget Limiter (I <= 2200mA) + Gamma mapping
    inline void commitFrameWithBudgetAndGamma(SmartLampStrip& strip, const PixelBuffer& buf, float brightness_factor, float max_allowed_ma = 2200.0f) {
        constexpr float QUIESCENT_CURRENT_MA = 60.0f; // 1mA quiescent * 60 LEDs
        constexpr float MAX_CHANNEL_MA       = 15.0f; // 15mA per channel at 255
        float max_total_ma                   = (max_allowed_ma >= 400.0f && max_allowed_ma <= 3000.0f) ? max_allowed_ma : 2200.0f;

        float scaled_r[Config::NUM_LEDS];
        float scaled_g[Config::NUM_LEDS];
        float scaled_b[Config::NUM_LEDS];
        float scaled_w[Config::NUM_LEDS];

        float sum_channels = 0.0f;
        float brt = constrain(brightness_factor, 0.01f, 1.0f);

        for (int i = 0; i < Config::NUM_LEDS; i++) {
            scaled_r[i] = constrain(buf.r[i] * brt, 0.0f, 255.0f);
            scaled_g[i] = constrain(buf.g[i] * brt, 0.0f, 255.0f);
            scaled_b[i] = constrain(buf.b[i] * brt, 0.0f, 255.0f);
            scaled_w[i] = constrain(buf.w[i] * brt, 0.0f, 255.0f);
            sum_channels += (scaled_r[i] + scaled_g[i] + scaled_b[i] + scaled_w[i]);
        }

        float total_current_est = QUIESCENT_CURRENT_MA + (sum_channels * (MAX_CHANNEL_MA / 255.0f));
        float pwr_scale = 1.0f;
        if (total_current_est > max_total_ma && (total_current_est - QUIESCENT_CURRENT_MA) > 1.0f) {
            pwr_scale = (max_total_ma - QUIESCENT_CURRENT_MA) / (total_current_est - QUIESCENT_CURRENT_MA);
        }

        for (int i = 0; i < Config::NUM_LEDS; i++) {
            uint8_t raw_r = (uint8_t)constrain(scaled_r[i] * pwr_scale, 0.0f, 255.0f);
            uint8_t raw_g = (uint8_t)constrain(scaled_g[i] * pwr_scale, 0.0f, 255.0f);
            uint8_t raw_b = (uint8_t)constrain(scaled_b[i] * pwr_scale, 0.0f, 255.0f);
            uint8_t raw_w = (uint8_t)constrain(scaled_w[i] * pwr_scale, 0.0f, 255.0f);

            uint8_t out_r = pgm_read_byte(&gamma28_rgb_lut[raw_r]);
            uint8_t out_g = pgm_read_byte(&gamma28_rgb_lut[raw_g]);
            uint8_t out_b = pgm_read_byte(&gamma28_rgb_lut[raw_b]);
            uint8_t out_w = pgm_read_byte(&gamma_w_lut[raw_w]);

            strip.SetPixelColor(i, RgbwColor(out_r, out_g, out_b, out_w));
        }
    }

    inline void renderNotification(
        SmartLampStrip& strip,
        uint8_t notify_type,
        uint16_t frame,
        float brightness_factor,
        uint8_t col_r = 56, uint8_t col_g = 189, uint8_t col_b = 248,
        float max_allowed_ma = 2200.0f
    ) {
        stageBuf.clear();
        float pulse = sinf((float)frame * 0.14f);
        float norm_p = (pulse + 1.0f) * 0.5f;

        switch (notify_type) {
            case NOTIFY_CYAN_WAVE: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, 0.0f, norm_p * 180.0f, norm_p * 255.0f, 0.0f);
                }
                break;
            }
            case NOTIFY_FIRE: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, norm_p * 255.0f, norm_p * 60.0f, 0.0f, 0.0f);
                }
                break;
            }
            case NOTIFY_LIGHTNING: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, norm_p * 200.0f, norm_p * 220.0f, norm_p * 255.0f, norm_p * 80.0f);
                }
                break;
            }
            case NOTIFY_CAT: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, norm_p * 255.0f, norm_p * 120.0f, norm_p * 180.0f, 0.0f);
                }
                break;
            }
            case NOTIFY_UFO: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, norm_p * 50.0f, norm_p * 255.0f, norm_p * 50.0f, 0.0f);
                }
                break;
            }
            case NOTIFY_CROWN: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, norm_p * 255.0f, norm_p * 200.0f, 0.0f, norm_p * 40.0f);
                }
                break;
            }
            case NOTIFY_AMBILIGHT: {
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, (float)col_r, (float)col_g, (float)col_b, 0.0f);
                }
                break;
            }
            default:
                break;
        }
        commitFrameWithBudgetAndGamma(strip, stageBuf, brightness_factor, max_allowed_ma);
    }

    inline void renderDynamicMode(
        SmartLampStrip& strip,
        uint8_t mode,
        uint32_t step,
        float brightness_factor,
        uint8_t anim_speed = 50,
        uint8_t ambi_r = 56, uint8_t ambi_g = 189, uint8_t ambi_b = 248,
        uint8_t ambi2_r = 248, uint8_t ambi2_g = 113, uint8_t ambi2_b = 113,
        uint8_t ambi3_r = 250, uint8_t ambi3_g = 204, uint8_t ambi3_b = 21,
        const SpotifyTrackInfo* sp = nullptr,
        uint8_t custom_r = 255, uint8_t custom_g = 180, uint8_t custom_b = 80, uint8_t custom_w = 120,
        uint8_t screen_mode = 0, float screen_temp = 22.5f, float screen_pomo_prog = 0.0f, float screen_pres_diff = 0.0f,
        float max_allowed_ma = 2200.0f
    ) {
        stageBuf.clear();
        float speed_scale = constrain((float)anim_speed / 50.0f, 0.1f, 2.5f);
        float t = (float)step * 0.045f * speed_scale;

        switch (mode) {
            case MODE_WARM_COZY: {
                // Mode 0: Warm Cozy (2700K phosphor W baseline + gentle R/G warmth)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, 90.0f, 32.0f, 0.0f, 255.0f);
                }
                break;
            }

            case MODE_CANDLE: {
                // Mode 1: Live Candle (Smooth multi-frequency draft waves, organic flame drift, zero raw random)
                float wind = sinf(t * 0.45f) * 0.35f;
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float f1 = sinf(t * 1.5f + (float)i * 0.38f);
                    float f2 = sinf(t * 3.1f - (float)i * 0.22f + wind);
                    float f3 = sinf(t * 5.4f + (float)i * 0.52f) * 0.35f;
                    float flame = (f1 + f2 + f3) / 2.35f; // -1..1
                    float flame_norm = (flame + 1.0f) * 0.5f; // 0..1

                    float w = 110.0f + flame_norm * 140.0f;
                    float r = 210.0f + flame_norm * 45.0f;
                    float g = 65.0f + flame_norm * 40.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_AMBER_EMBER: {
                // Mode 2: Amber Candle (Smooth hypnotic sine breathing)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float breath = (sinf(t * 0.75f + (float)i * 0.18f) + 1.0f) * 0.5f;
                    float w = 35.0f + breath * 75.0f;
                    float r = 230.0f + breath * 25.0f;
                    float g = 75.0f + breath * 45.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_FIREPLACE: {
                // Mode 3: Fireplace with Sparks (3 travelling continuous heat waves + travelling spark crests)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float base_w1 = sinf(t * 2.1f + (float)i * 0.32f);
                    float base_w2 = sinf(t * 1.2f - (float)i * 0.45f);
                    float heat = (base_w1 + base_w2 + 2.0f) * 0.25f; // 0..1

                    // Smooth travelling spark crests (power-of-6 sine kernel, smooth across adjacent LEDs)
                    float spark_val = sinf(t * 3.6f + (float)i * 0.75f);
                    float spark = (spark_val > 0.0f) ? powf(spark_val, 6.0f) : 0.0f;

                    float r = 180.0f + heat * 75.0f;
                    float g = 30.0f + heat * 50.0f + spark * 70.0f;
                    float w = heat * 15.0f + spark * 60.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_SUNSET_VIBE: {
                // Mode 4: Sunset Glow (W = 0, rich crimson-violet to warm golden horizon)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float phase = sinf(t * 0.65f + ((float)i / (float)Config::NUM_LEDS) * 6.28318f);
                    float norm = (phase + 1.0f) * 0.5f;
                    float r = 240.0f + norm * 15.0f;
                    float g = 40.0f + norm * 135.0f;
                    float b = (1.0f - norm) * 110.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_SAKURA_NEON: {
                // Mode 5: Sakura Neon (W = 0, delicate cherry blossom magenta to violet plasma)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float s1 = (sinf(t * 0.9f + (float)i * 0.28f) + 1.0f) * 0.5f;
                    float s2 = (sinf(t * 1.4f - (float)i * 0.19f) + 1.0f) * 0.5f;
                    float r = 220.0f + s1 * 35.0f;
                    float g = 20.0f + s2 * 50.0f;
                    float b = 70.0f + s2 * 185.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_AURORA_WAVE: {
                // Mode 6: Northern Lights (W = 0, multi-wave emerald & arctic blue ribbon plasma)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float w1 = sinf(t * 0.75f + (float)i * 0.22f);
                    float w2 = sinf(t * 1.15f - (float)i * 0.34f);
                    float w3 = sinf(t * 0.45f + (float)i * 0.52f);
                    float plasma = (w1 + w2 + w3 + 3.0f) / 6.0f; // 0..1

                    float r = plasma * 25.0f;
                    float g = 90.0f + plasma * 165.0f;
                    float b = 80.0f + (1.0f - plasma) * 175.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_OCEAN_BREEZE: {
                // Mode 7: Ocean Breeze (W = 0, deep navy to azure swells with crest highlights)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float swell = (sinf(t * 0.85f + (float)i * 0.24f) + 1.0f) * 0.5f;
                    float r = swell * 15.0f;
                    float g = swell * 175.0f;
                    float b = 90.0f + swell * 165.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_CYBER_RUNNER: {
                // Mode 8: Cyber Comet (W = 0, subpixel fractional anti-aliasing)
                float speed_px_per_sec = 26.0f * speed_scale;
                float head_pos = fmodf((float)step * (0.02f * speed_px_per_sec), (float)Config::NUM_LEDS);
                constexpr float TAIL_LEN = 18.0f;

                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float dist = (float)i - head_pos;
                    while (dist < 0.0f) dist += (float)Config::NUM_LEDS;
                    while (dist >= (float)Config::NUM_LEDS) dist -= (float)Config::NUM_LEDS;

                    float tail_dist = (float)Config::NUM_LEDS - dist;
                    float intensity = 0.0f;
                    if (tail_dist <= TAIL_LEN) {
                        intensity = tail_dist / TAIL_LEN;
                    }
                    float sq = intensity * intensity;
                    float r = sq * 190.0f;
                    float g = sq * 50.0f + 2.0f;
                    float b = sq * 255.0f + 14.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_RAINBOW_FLOW: {
                // Mode 9: Rainbow Flow (W = 0, continuous smooth plasma spectrum)
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    float hue = fmodf(t * 0.35f + ((float)i / (float)Config::NUM_LEDS), 1.0f);
                    float r, g, b;
                    hsv2rgb(hue, 1.0f, 1.0f, r, g, b);
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_LIGHT_OFF: {
                // Mode 10: Off
                stageBuf.clear();
                break;
            }

            case MODE_SPOTIFY_AMBI: {
                // Mode 11: AmbiLight 2.0 Multi-Zone Spatial Halo (W = 0 strictly)
                uint32_t now = millis();
                bool is_playing = (sp && sp->playing);

                if (is_playing) {
                    // --- Active Music Playback: 3-Zone Spatial Halo ---
                    uint16_t raw_bpm = (sp->bpm >= 40 && sp->bpm <= 240) ? sp->bpm : 120;
                    float track_energy = (sp->energy > 0) ? ((float)sp->energy / 100.0f) : 0.60f;
                    track_energy = constrain(track_energy, 0.20f, 1.00f);

                    uint32_t song_ms = now;
                    if (sp->last_sync_millis > 0) {
                        song_ms = sp->last_server_progress_ms + (now - sp->last_sync_millis);
                    } else {
                        song_ms = sp->progress_ms;
                    }

                    float eff_bpm = (raw_bpm > 124) ? ((float)raw_bpm * 0.5f) : (float)raw_bpm;
                    float beat_period = 60000.0f / eff_bpm;
                    float beat_phase = fmodf((float)song_ms, beat_period) / beat_period;
                    float s = sinf(beat_phase * 3.14159265f);
                    float beat_swell = s * s * s * s; // sin^4 punch

                    // HSV Color Palette: Primary, Secondary, Accent
                    HsvColor c_prim = rgb2hsv(ambi_r, ambi_g, ambi_b);
                    HsvColor c_sec  = rgb2hsv(ambi2_r, ambi2_g, ambi2_b);
                    HsvColor c_acc  = rgb2hsv(ambi3_r, ambi3_g, ambi3_b);

                    // Ensure high saturation & value
                    c_prim.s = max(0.80f, c_prim.s); c_prim.v = 1.0f;
                    c_sec.s  = max(0.80f, c_sec.s);  c_sec.v = 1.0f;
                    c_acc.s  = max(0.85f, c_acc.s);  c_acc.v = 1.0f;

                    // Organic rotation matching vinyl rotation angle
                    float rot_offset = (float)song_ms * 0.00075f;

                    for (int i = 0; i < Config::NUM_LEDS; i++) {
                        float angle = (2.0f * 3.14159265f * (float)i) / (float)Config::NUM_LEDS + rot_offset;
                        float cos_a = cosf(angle); // 1.0 = Front (under screen), -1.0 = Rear

                        // Primary (Front under screen: cos_a > 0.1) vs Secondary (Sides/Rear: cos_a < 0.1)
                        float blend_t = constrain((cos_a + 0.6f) / 1.4f, 0.0f, 1.0f);
                        float r, g, b;
                        interpolateHsv(c_sec, c_prim, blend_t, r, g, b);

                        // Accent wave pulsating upward through the ring
                        float acc_wave = sinf(angle * 2.0f - (float)song_ms * 0.003f + beat_swell * 1.5f);
                        float acc_intensity = max(0.0f, acc_wave) * (0.35f + 0.45f * track_energy * beat_swell);

                        if (acc_intensity > 0.01f) {
                            float ar, ag, ab;
                            hsv2rgb(c_acc.h, c_acc.s, c_acc.v, ar, ag, ab);
                            r = r * (1.0f - acc_intensity) + ar * acc_intensity;
                            g = g * (1.0f - acc_intensity) + ag * acc_intensity;
                            b = b * (1.0f - acc_intensity) + ab * acc_intensity;
                        }

                        stageBuf.set(i, r, g, b, 0.0f);
                    }
                } else {
                    // --- Smart Screen AmbiLight Fallback (When Spotify/Media is Idle) ---
                    switch (screen_mode) {
                        case SCREEN_CLIMATE: {
                            // Temperature gradient (cool sky blue to warm terracotta) + turquoise humidity swell
                            float temp_factor = constrain((screen_temp - 18.0f) / 10.0f, 0.0f, 1.0f);
                            HsvColor c_cool{0.55f, 0.85f, 1.0f}; // Calm Sky Blue
                            HsvColor c_warm{0.07f, 0.90f, 1.0f}; // Terracotta / Amber Warmth
                            float wave = (sinf(t * 0.7f) + 1.0f) * 0.5f;

                            for (int i = 0; i < Config::NUM_LEDS; i++) {
                                float p = (float)i / (float)Config::NUM_LEDS;
                                float local_t = constrain(temp_factor + sinf(p * 6.28318f + t * 0.5f) * 0.25f, 0.0f, 1.0f);
                                float r, g, b;
                                interpolateHsv(c_cool, c_warm, local_t, r, g, b);
                                // Humidity wave tint
                                g = constrain(g + wave * 30.0f, 0.0f, 255.0f);
                                stageBuf.set(i, r, g, b, 0.0f);
                            }
                            break;
                        }

                        case SCREEN_POMODORO: {
                            // Radial progress ring (hot amber focus / emerald break)
                            bool is_break = (screen_pomo_prog < 0.0f);
                            float prog = fabsf(screen_pomo_prog);
                            int lit_leds = (int)(prog * (float)Config::NUM_LEDS);

                            HsvColor active_col = is_break ? HsvColor{0.42f, 0.90f, 1.0f} : HsvColor{0.09f, 0.95f, 1.0f};
                            HsvColor dim_col    = is_break ? HsvColor{0.42f, 0.60f, 0.18f} : HsvColor{0.09f, 0.60f, 0.18f};

                            for (int i = 0; i < Config::NUM_LEDS; i++) {
                                float r, g, b;
                                if (i <= lit_leds) {
                                    hsv2rgb(active_col.h, active_col.s, active_col.v, r, g, b);
                                } else {
                                    hsv2rgb(dim_col.h, dim_col.s, dim_col.v, r, g, b);
                                }
                                stageBuf.set(i, r, g, b, 0.0f);
                            }
                            break;
                        }

                        case SCREEN_BAROMETER: {
                            // Barometric sky: deep indigo falling, azure steady, golden rising
                            HsvColor sky_col;
                            if (screen_pres_diff < -0.6f) {
                                sky_col = HsvColor{0.67f, 0.85f, 1.0f}; // Deep Indigo
                            } else if (screen_pres_diff > 0.6f) {
                                sky_col = HsvColor{0.12f, 0.90f, 1.0f}; // Radiant Golden
                            } else {
                                sky_col = HsvColor{0.54f, 0.85f, 1.0f}; // Calm Azure
                            }

                            for (int i = 0; i < Config::NUM_LEDS; i++) {
                                float wave = (sinf(t * 0.6f + (float)i * 0.15f) + 1.0f) * 0.5f;
                                float r, g, b;
                                hsv2rgb(sky_col.h, sky_col.s * (0.85f + wave * 0.15f), 1.0f, r, g, b);
                                stageBuf.set(i, r, g, b, 0.0f);
                            }
                            break;
                        }

                        default: {
                            // Default idle AmbiLight: calm ambient halo from last known palette
                            HsvColor c_prim = rgb2hsv(ambi_r, ambi_g, ambi_b);
                            HsvColor c_sec  = rgb2hsv(ambi2_r, ambi2_g, ambi2_b);
                            float breath = (sinf(t * 0.5f) + 1.0f) * 0.5f;

                            for (int i = 0; i < Config::NUM_LEDS; i++) {
                                float p = (float)i / (float)Config::NUM_LEDS;
                                float frac = (sinf(p * 6.28318f + t * 0.3f) + 1.0f) * 0.5f;
                                float r, g, b;
                                interpolateHsv(c_sec, c_prim, frac, r, g, b);
                                stageBuf.set(i, r * (0.85f + breath * 0.15f), g * (0.85f + breath * 0.15f), b * (0.85f + breath * 0.15f), 0.0f);
                            }
                            break;
                        }
                    }
                }
                break;
            }

            default: {
                // Manual RGBW Mode
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    stageBuf.set(i, (float)custom_r, (float)custom_g, (float)custom_b, (float)custom_w);
                }
                break;
            }
        }

        commitFrameWithBudgetAndGamma(strip, stageBuf, brightness_factor, max_allowed_ma);
    }
}
