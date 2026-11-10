#pragma once
#include <Arduino.h>
#include <NeoPixelBus.h>
#include "Config.h"
#include "core/Types.h"

// Hardware SK6812 RGBW (GRBW) on isolated ESP32-S3 RMT0
typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Sk6812Method> SmartLampStrip;

inline float fastPow4(float x) {
    float x2 = x * x;
    return x2 * x2;
}

inline float fastPow6(float x) {
    float x2 = x * x;
    return x2 * x2 * x2;
}

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
        float r[Config::MAX_LED_COUNT];
        float g[Config::MAX_LED_COUNT];
        float b[Config::MAX_LED_COUNT];
        float w[Config::MAX_LED_COUNT];

        void clear() {
            memset(r, 0, sizeof(r));
            memset(g, 0, sizeof(g));
            memset(b, 0, sizeof(b));
            memset(w, 0, sizeof(w));
        }

        void set(int i, float cr, float cg, float cb, float cw) {
            if (i >= 0 && i < Config::MAX_LED_COUNT) {
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

        float scaled_r[Config::MAX_LED_COUNT];
        float scaled_g[Config::MAX_LED_COUNT];
        float scaled_b[Config::MAX_LED_COUNT];
        float scaled_w[Config::MAX_LED_COUNT];

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
        float max_allowed_ma = 2200.0f,
        const LedLayoutConfig& layout = LedLayoutConfig(),
        bool audio_playing = false,
        float wifi_combo_prog = 0.0f,
        int16_t touch_ripple_angle = -999,
        uint32_t touch_ripple_elapsed_ms = 9999
    ) {
        stageBuf.clear();
        float speed_scale = constrain((float)anim_speed / 50.0f, 0.1f, 2.5f);
        float t = (float)step * 0.045f * speed_scale;

        uint16_t total_cnt = layout.totalCount();
        if (total_cnt == 0 || total_cnt > Config::MAX_LED_COUNT) total_cnt = 60;
        uint16_t bot_cnt = layout.bottom_count;
        uint16_t bak_cnt = layout.back_count;
        if (bot_cnt == 0 || bot_cnt >= total_cnt) {
            bot_cnt = (total_cnt * 3) / 5;
            bak_cnt = total_cnt - bot_cnt;
        }

        switch (mode) {
            case MODE_WARM_COZY: {
                for (int i = 0; i < total_cnt; i++) {
                    stageBuf.set(i, 90.0f, 32.0f, 0.0f, 255.0f);
                }
                break;
            }

            case MODE_CANDLE: {
                float wind = sinf(t * 0.45f) * 0.35f;
                for (int i = 0; i < total_cnt; i++) {
                    float f1 = sinf(t * 1.5f + (float)i * 0.38f);
                    float f2 = sinf(t * 3.1f - (float)i * 0.22f + wind);
                    float f3 = sinf(t * 5.4f + (float)i * 0.52f) * 0.35f;
                    float flame = (f1 + f2 + f3) / 2.35f;
                    float flame_norm = (flame + 1.0f) * 0.5f;

                    float w = 110.0f + flame_norm * 140.0f;
                    float r = 210.0f + flame_norm * 45.0f;
                    float g = 65.0f + flame_norm * 40.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_AMBER_EMBER: {
                for (int i = 0; i < total_cnt; i++) {
                    float breath = (sinf(t * 0.75f + (float)i * 0.18f) + 1.0f) * 0.5f;
                    float w = 35.0f + breath * 75.0f;
                    float r = 230.0f + breath * 25.0f;
                    float g = 75.0f + breath * 45.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_FIREPLACE: {
                // Блок 2: Дно — розпечене вугілля; Задня стінка — язики полум'я та іскри на стіні
                for (int i = 0; i < bot_cnt; i++) {
                    float base_w1 = sinf(t * 2.1f + (float)i * 0.32f);
                    float base_w2 = sinf(t * 1.2f - (float)i * 0.45f);
                    float heat = (base_w1 + base_w2 + 2.0f) * 0.25f;
                    float r = 190.0f + heat * 65.0f;
                    float g = 25.0f + heat * 45.0f;
                    float w = heat * 20.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                for (int i = bot_cnt; i < total_cnt; i++) {
                    uint16_t k = i - bot_cnt;
                    float v = (bak_cnt > 1) ? ((float)k / (float)(bak_cnt - 1)) : 0.5f;
                    float dist_ctr = fabsf(v - 0.5f) * 2.0f;
                    float spark_val = sinf(t * 3.8f + (float)k * 0.65f);
                    float spark = (spark_val > 0.0f) ? fastPow6(spark_val) : 0.0f;
                    float r = 160.0f + (1.0f - dist_ctr) * 80.0f + spark * 40.0f;
                    float g = 20.0f + (1.0f - dist_ctr) * 45.0f + spark * 80.0f;
                    float w = spark * 75.0f;
                    stageBuf.set(i, r, g, 0.0f, w);
                }
                break;
            }

            case MODE_SUNSET_VIBE: {
                for (int i = 0; i < total_cnt; i++) {
                    float phase = sinf(t * 0.65f + ((float)i / (float)total_cnt) * 6.28318f);
                    float norm = (phase + 1.0f) * 0.5f;
                    float r = 240.0f + norm * 15.0f;
                    float g = 40.0f + norm * 135.0f;
                    float b = (1.0f - norm) * 110.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_SAKURA_NEON: {
                for (int i = 0; i < total_cnt; i++) {
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
                for (int i = 0; i < total_cnt; i++) {
                    float w1 = sinf(t * 0.75f + (float)i * 0.22f);
                    float w2 = sinf(t * 1.15f - (float)i * 0.34f);
                    float w3 = sinf(t * 0.45f + (float)i * 0.52f);
                    float plasma = (w1 + w2 + w3 + 3.0f) / 6.0f;

                    float r = plasma * 25.0f;
                    float g = 90.0f + plasma * 165.0f;
                    float b = 80.0f + (1.0f - plasma) * 175.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_OCEAN_BREEZE: {
                for (int i = 0; i < total_cnt; i++) {
                    float swell = (sinf(t * 0.85f + (float)i * 0.24f) + 1.0f) * 0.5f;
                    float r = swell * 15.0f;
                    float g = swell * 175.0f;
                    float b = 90.0f + swell * 165.0f;
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_CYBER_RUNNER: {
                float speed_px_per_sec = 26.0f * speed_scale;
                float head_pos = fmodf((float)step * (0.02f * speed_px_per_sec), (float)total_cnt);
                constexpr float TAIL_LEN = 18.0f;

                for (int i = 0; i < total_cnt; i++) {
                    float dist = (float)i - head_pos;
                    while (dist < 0.0f) dist += (float)total_cnt;
                    while (dist >= (float)total_cnt) dist -= (float)total_cnt;

                    float tail_dist = (float)total_cnt - dist;
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
                for (int i = 0; i < total_cnt; i++) {
                    float hue = fmodf(t * 0.35f + ((float)i / (float)total_cnt), 1.0f);
                    float r, g, b;
                    hsv2rgb(hue, 1.0f, 1.0f, r, g, b);
                    stageBuf.set(i, r, g, b, 0.0f);
                }
                break;
            }

            case MODE_LIGHT_OFF: {
                stageBuf.clear();
                break;
            }

            case MODE_SPOTIFY_AMBI: {
                // Блок 2: 2.5D Dual-Zone AmbiLight
                uint32_t now = millis();
                bool is_playing = (sp && sp->playing);

                HsvColor c_prim = rgb2hsv(ambi_r, ambi_g, ambi_b);
                HsvColor c_sec  = rgb2hsv(ambi2_r, ambi2_g, ambi2_b);
                HsvColor c_acc  = rgb2hsv(ambi3_r, ambi3_g, ambi3_b);
                c_prim.s = max(0.80f, c_prim.s); c_prim.v = 1.0f;
                c_sec.s  = max(0.80f, c_sec.s);  c_sec.v = 1.0f;
                c_acc.s  = max(0.85f, c_acc.s);  c_acc.v = 1.0f;

                if (is_playing) {
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
                    float beat_swell = fastPow4(sinf(beat_phase * 3.14159265f));

                    // 1. ЗОНА 1: Дно (Bottom Underglow) — стікання кольору з екрана на стіл + sin^4 біт-пульсація
                    for (int i = 0; i < bot_cnt; i++) {
                        int16_t rel = layout.reversed ? (layout.bottom_offset - i) : (i - layout.bottom_offset);
                        while (rel < 0) rel += bot_cnt;
                        rel %= bot_cnt;
                        float frac = (float)rel / (float)bot_cnt;
                        float rad = frac * 6.2831853f;
                        float cos_front = cosf(rad); // +1.0 під екраном, -1.0 ззаду

                        float spot_width = 0.30f + 0.40f * beat_swell * track_energy;
                        float front_blend = constrain((cos_front + spot_width) / (1.0f + spot_width), 0.0f, 1.0f);
                        float r, g, b;
                        interpolateHsv(c_sec, c_prim, front_blend, r, g, b);

                        float boost = 1.0f + 0.32f * beat_swell * track_energy;
                        r = constrain(r * boost, 0.0f, 255.0f);
                        g = constrain(g * boost, 0.0f, 255.0f);
                        b = constrain(b * boost, 0.0f, 255.0f);
                        float w = (sp->is_mono) ? (35.0f * beat_swell) : 0.0f;
                        stageBuf.set(i, r, g, b, w);
                    }

                    // 2. ЗОНА 2: Задня стінка (Backlight Wall-Wash) — плазмові переливи c_sec/c_acc на стіну
                    for (int i = bot_cnt; i < total_cnt; i++) {
                        uint16_t k = i - bot_cnt;
                        float v = (bak_cnt > 1) ? ((float)k / (float)(bak_cnt - 1)) : 0.5f;
                        float plasma = sinf(t * 0.65f + v * 6.28318f) * 0.5f + sinf(t * 1.2f - v * 3.14159f) * 0.35f;
                        float plasma_norm = constrain((plasma + 0.85f) / 1.7f, 0.0f, 1.0f);

                        float r, g, b;
                        interpolateHsv(c_sec, c_acc, plasma_norm, r, g, b);
                        float valley = powf(sinf(v * 3.14159265f), 1.4f);
                        float depth = 0.40f + 0.60f * valley;
                        r *= depth; g *= depth; b *= depth;
                        float w = (sp->is_mono) ? 15.0f : 0.0f;
                        stageBuf.set(i, r, g, b, w);
                    }
                } else {
                    // Fallback: затишний спокійний ореол при паузі
                    float breath = (sinf(t * 0.5f) + 1.0f) * 0.5f;
                    for (int i = 0; i < bot_cnt; i++) {
                        float r, g, b;
                        hsv2rgb(c_prim.h, c_prim.s, 1.0f, r, g, b);
                        stageBuf.set(i, r * (0.8f + breath * 0.2f), g * (0.8f + breath * 0.2f), b * (0.8f + breath * 0.2f), 0.0f);
                    }
                    for (int i = bot_cnt; i < total_cnt; i++) {
                        float r, g, b;
                        hsv2rgb(c_sec.h, c_sec.s, 1.0f, r, g, b);
                        stageBuf.set(i, r * (0.7f + breath * 0.3f), g * (0.7f + breath * 0.3f), b * (0.7f + breath * 0.3f), 0.0f);
                    }
                }
                break;
            }

            default: {
                for (int i = 0; i < total_cnt; i++) {
                    stageBuf.set(i, (float)custom_r, (float)custom_g, (float)custom_b, (float)custom_w);
                }
                break;
            }
        }

        // Блок 3 & 4: Оверлеї (Voice Envelope, Touch Ripple, Wi-Fi Combo)
        // 1. Voice Envelope під час мовлення TTS
        if (audio_playing) {
            float voice_breath = (sinf((float)step * 0.22f) + 1.0f) * 0.5f;
            for (int i = 0; i < bot_cnt; i++) {
                int16_t rel = layout.reversed ? (layout.bottom_offset - i) : (i - layout.bottom_offset);
                while (rel < 0) rel += bot_cnt;
                rel %= bot_cnt;
                float cos_f = cosf((float)rel * (6.2831853f / (float)bot_cnt));
                if (cos_f > 0.1f) {
                    float add_val = cos_f * voice_breath * 35.0f;
                    stageBuf.r[i] = constrain(stageBuf.r[i] + add_val * 0.8f, 0.0f, 255.0f);
                    stageBuf.g[i] = constrain(stageBuf.g[i] + add_val, 0.0f, 255.0f);
                    stageBuf.b[i] = constrain(stageBuf.b[i] + add_val * 1.2f, 0.0f, 255.0f);
                }
            }
            for (int i = bot_cnt; i < total_cnt; i++) {
                uint16_t k = i - bot_cnt;
                float v = (bak_cnt > 1) ? ((float)k / (float)(bak_cnt - 1)) : 0.5f;
                float dist_ctr = fabsf(v - 0.5f) * 2.0f;
                if (dist_ctr < 0.45f) {
                    float add_val = (1.0f - dist_ctr / 0.45f) * voice_breath * 30.0f;
                    stageBuf.r[i] = constrain(stageBuf.r[i] + add_val * 0.8f, 0.0f, 255.0f);
                    stageBuf.g[i] = constrain(stageBuf.g[i] + add_val, 0.0f, 255.0f);
                    stageBuf.b[i] = constrain(stageBuf.b[i] + add_val * 1.2f, 0.0f, 255.0f);
                }
            }
        }

        // 2. Touch Ripple хвиля від дотику до кнопок (з м'яким 30-мс плавним входом для захисту шини 5V під час аудіо-кліка)
        if (touch_ripple_angle != -999 && touch_ripple_elapsed_ms < 1100) {
            float btn_rad = ((float)touch_ripple_angle / 180.0f) * 3.14159265f;
            float wave_dist_leds = (float)touch_ripple_elapsed_ms * 0.024f;
            float attack = (touch_ripple_elapsed_ms < 30) ? ((float)touch_ripple_elapsed_ms / 30.0f) : 1.0f;
            float fade = (1.0f - ((float)touch_ripple_elapsed_ms / 1100.0f)) * attack;

            for (int i = 0; i < bot_cnt; i++) {
                int16_t rel = layout.reversed ? (layout.bottom_offset - i) : (i - layout.bottom_offset);
                while (rel < 0) rel += bot_cnt;
                rel %= bot_cnt;
                float rad = (float)rel * (6.2831853f / (float)bot_cnt);
                float diff = fabsf(rad - btn_rad);
                if (diff > 3.14159265f) diff = 6.2831853f - diff;
                float arc_leds = diff * ((float)bot_cnt / 6.2831853f);
                float d = fabsf(arc_leds - wave_dist_leds);
                if (d < 3.2f) {
                    float intn = (1.0f - d / 3.2f) * fade;
                    stageBuf.r[i] = constrain(stageBuf.r[i] + intn * 70.0f, 0.0f, 255.0f);
                    stageBuf.g[i] = constrain(stageBuf.g[i] + intn * 190.0f, 0.0f, 255.0f);
                    stageBuf.b[i] = constrain(stageBuf.b[i] + intn * 240.0f, 0.0f, 255.0f);
                }
            }
        }

        // 3. Wi-Fi Combo прогрес-бар (Emergency AP Reset)
        if (wifi_combo_prog > 0.01f) {
            uint16_t fill_leds = (uint16_t)(wifi_combo_prog * (float)bot_cnt);
            for (int i = 0; i < bot_cnt; i++) {
                int16_t rel = layout.reversed ? (layout.bottom_offset - i) : (i - layout.bottom_offset);
                while (rel < 0) rel += bot_cnt;
                rel %= bot_cnt;
                if (rel <= fill_leds) {
                    stageBuf.set(i, 0.0f, 210.0f, 255.0f, 0.0f);
                } else if (rel == fill_leds + 1) {
                    stageBuf.set(i, 220.0f, 255.0f, 255.0f, 0.0f); // Іскра на кінці
                }
            }
        }

        // 4. Повне гасіння незмонтованих діодів
        for (int i = total_cnt; i < Config::MAX_LED_COUNT; i++) {
            stageBuf.set(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }

        commitFrameWithBudgetAndGamma(strip, stageBuf, brightness_factor, max_allowed_ma);
    }
}
