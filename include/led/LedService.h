#pragma once
#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <esp_attr.h>
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "led/LedEffects.h"

class LedService : public IService {
private:
    AppState& state;
    SmartLampStrip strip;

    uint32_t last_anim_ms       = 0;
    uint32_t last_brt_ms        = 0;
    uint32_t last_show_ms       = 0;
    uint32_t last_show_us       = 0;
    uint32_t anim_step          = 0;
    int      last_mode          = -1;
    uint16_t notify_frames      = 0;
    uint8_t  active_notify_type = NOTIFY_NONE;

    // Smooth RGB lerp for base ambient colors
    float    cur_r              = 0.0f;
    float    cur_g              = 0.0f;
    float    cur_b              = 0.0f;
    float    cur_s_r            = 0.0f;
    float    cur_s_g            = 0.0f;
    float    cur_s_b            = 0.0f;
    float    cur_a_r            = 0.0f;
    float    cur_a_g            = 0.0f;
    float    cur_a_b            = 0.0f;
    uint32_t last_lerp_ms       = 0;

    // Fast internal SRAM buffers (DRAM_ATTR) to avoid any PSRAM contention
    DRAM_ATTR static RgbwColor s_shadow_pixels[Config::NUM_LEDS];
    DRAM_ATTR static RgbwColor s_slew_pixels[Config::NUM_LEDS];

    const uint8_t manual_pct_levels[4] = {15, 35, 65, 100};
    const uint8_t lcd_levels[4]        = {60, 130, 200, 255};

    // Zero-Glitch RMT Latch & Slew-Rate Guard
    void showWithLatchGuard(bool force = false) {
        uint32_t now = millis();
        bool is_color_mode = (state.light_mode >= MODE_SUNSET_VIBE && state.light_mode <= MODE_SPOTIFY_AMBI);

        // 1. Per-Pixel Slew-Rate & Anti-Flash Filter
        // Strictly prevents RMT FIFO underrun glitches from causing 1-frame 255,255,255,255 white spikes
        constexpr int16_t MAX_SLEW = 42; // Max step per 16ms frame (smooth transition in ~95ms)

        for (int i = 0; i < Config::NUM_LEDS; i++) {
            RgbwColor raw = strip.GetPixelColor(i);

            // In all color modes, channel W is strictly clamped to 0 (no washing out)
            if (is_color_mode) {
                raw.W = 0;
            }

            RgbwColor cur = s_slew_pixels[i];

            // Anti-flash: if previous was non-white and raw tries to instantly jump to full white
            if (raw.W > 180 && cur.W < 60) raw.W = cur.W + MAX_SLEW;
            if (raw.R > 210 && raw.G > 210 && raw.B > 210 && (cur.R < 100 || cur.G < 100 || cur.B < 100)) {
                raw.R = cur.R + MAX_SLEW;
                raw.G = cur.G + MAX_SLEW;
                raw.B = cur.B + MAX_SLEW;
            }

            // Slew-rate limiting per channel
            auto slewChannel = [](uint8_t target, uint8_t current) -> uint8_t {
                int16_t diff = (int16_t)target - (int16_t)current;
                if (diff > MAX_SLEW)  return current + MAX_SLEW;
                if (diff < -MAX_SLEW) return current - MAX_SLEW;
                return target;
            };

            RgbwColor filtered;
            filtered.R = slewChannel(raw.R, cur.R);
            filtered.G = slewChannel(raw.G, cur.G);
            filtered.B = slewChannel(raw.B, cur.B);
            filtered.W = slewChannel(raw.W, cur.W);

            s_slew_pixels[i] = filtered;
            strip.SetPixelColor(i, filtered);
        }

        // 2. Dirty-flag frame guard
        bool frame_dirty = false;
        for (int i = 0; i < Config::NUM_LEDS; i++) {
            RgbwColor cur = s_slew_pixels[i];
            if (cur != s_shadow_pixels[i]) {
                frame_dirty = true;
                s_shadow_pixels[i] = cur;
            }
        }

        if (!frame_dirty && !force && (now - last_show_ms < 2000)) {
            return;
        }

        // 3. Wait for previous RMT transmission to be 100% complete
        while (!strip.CanShow()) {
            taskYIELD();
        }

        // 4. Strict Latch/Reset guard: >= 100 microseconds pause before and after .Show()
        uint32_t elapsed_us = micros() - last_show_us;
        if (elapsed_us < 105) {
            delayMicroseconds(105 - elapsed_us);
        }

        strip.Show();
        last_show_ms = now;
        last_show_us = micros();
    }

    void renderStrip(bool force) {
        uint32_t now = millis();
        bool mode_changed = (state.light_mode != last_mode);
        last_mode = state.light_mode;

        uint8_t target_r = state.ambi_r;
        uint8_t target_g = state.ambi_g;
        uint8_t target_b = state.ambi_b;
        uint8_t target_s_r = state.ambi2_r;
        uint8_t target_s_g = state.ambi2_g;
        uint8_t target_s_b = state.ambi2_b;
        uint8_t target_a_r = state.ambi3_r;
        uint8_t target_a_g = state.ambi3_g;
        uint8_t target_a_b = state.ambi3_b;

        if (state.light_mode == MODE_SPOTIFY_AMBI && state.spotify.playing) {
            target_r = state.spotify.ambi_r;
            target_g = state.spotify.ambi_g;
            target_b = state.spotify.ambi_b;
            target_s_r = state.spotify.ambi2_r;
            target_s_g = state.spotify.ambi2_g;
            target_s_b = state.spotify.ambi2_b;
            target_a_r = state.spotify.ambi3_r;
            target_a_g = state.spotify.ambi3_g;
            target_a_b = state.spotify.ambi3_b;
        }

        // Smooth color lerp across ~250-400ms
        if (mode_changed || force) {
            cur_r = (float)target_r;   cur_g = (float)target_g;   cur_b = (float)target_b;
            cur_s_r = (float)target_s_r; cur_s_g = (float)target_s_g; cur_s_b = (float)target_s_b;
            cur_a_r = (float)target_a_r; cur_a_g = (float)target_a_g; cur_a_b = (float)target_a_b;
            last_lerp_ms = now;
        } else if (now - last_lerp_ms >= 16) {
            last_lerp_ms = now;
            constexpr float ALPHA = 0.085f;
            cur_r += ((float)target_r - cur_r) * ALPHA;
            cur_g += ((float)target_g - cur_g) * ALPHA;
            cur_b += ((float)target_b - cur_b) * ALPHA;
            cur_s_r += ((float)target_s_r - cur_s_r) * ALPHA;
            cur_s_g += ((float)target_s_g - cur_s_g) * ALPHA;
            cur_s_b += ((float)target_s_b - cur_s_b) * ALPHA;
            cur_a_r += ((float)target_a_r - cur_a_r) * ALPHA;
            cur_a_g += ((float)target_a_g - cur_a_g) * ALPHA;
            cur_a_b += ((float)target_a_b - cur_a_b) * ALPHA;
        }

        uint8_t eff_r = (uint8_t)constrain((int)(cur_r + 0.5f), 0, 255);
        uint8_t eff_g = (uint8_t)constrain((int)(cur_g + 0.5f), 0, 255);
        uint8_t eff_b = (uint8_t)constrain((int)(cur_b + 0.5f), 0, 255);
        uint8_t eff_s_r = (uint8_t)constrain((int)(cur_s_r + 0.5f), 0, 255);
        uint8_t eff_s_g = (uint8_t)constrain((int)(cur_s_g + 0.5f), 0, 255);
        uint8_t eff_s_b = (uint8_t)constrain((int)(cur_s_b + 0.5f), 0, 255);
        uint8_t eff_a_r = (uint8_t)constrain((int)(cur_a_r + 0.5f), 0, 255);
        uint8_t eff_a_g = (uint8_t)constrain((int)(cur_a_g + 0.5f), 0, 255);
        uint8_t eff_a_b = (uint8_t)constrain((int)(cur_a_b + 0.5f), 0, 255);

        float brt_factor = (float)state.manual_brt_pct / 100.0f;
        brt_factor = constrain(brt_factor, 0.01f, 1.0f);

        // 1. Notification overlay
        if (notify_frames > 0) {
            if (now - last_anim_ms >= 16) {
                last_anim_ms = now;
                notify_frames--;
                LedEffects::renderNotification(strip, active_notify_type, notify_frames, brt_factor, eff_r, eff_g, eff_b);
                showWithLatchGuard(true);
                if (notify_frames == 0) last_mode = -1;
            }
            return;
        }

        // 2. Off mode
        if (state.light_mode == MODE_LIGHT_OFF) {
            if (mode_changed || force) {
                strip.ClearTo(RgbwColor(0, 0, 0, 0));
                showWithLatchGuard(true);
            }
            return;
        }

        // 3. Dynamic 60 FPS update
        if (now - last_anim_ms < 16 && !mode_changed && !force) return;
        last_anim_ms = now;

        // Context screen parameters for Smart AmbiLight fallback
        float pomo_prog = 0.0f;
        if (state.pomo_active && state.pomo_duration_min > 0) {
            float frac = (float)state.getPomoRemainingSeconds() / (float)(state.pomo_duration_min * 60);
            pomo_prog = state.pomo_is_break ? -frac : frac;
        }

        LedEffects::renderDynamicMode(
            strip,
            state.light_mode,
            anim_step++,
            brt_factor,
            state.anim_speed,
            eff_r, eff_g, eff_b,
            eff_s_r, eff_s_g, eff_s_b,
            eff_a_r, eff_a_g, eff_a_b,
            (state.light_mode == MODE_SPOTIFY_AMBI) ? &state.spotify : nullptr,
            state.custom_r, state.custom_g, state.custom_b, state.custom_w,
            state.screen_mode,
            state.temperature,
            pomo_prog,
            state.pressure_diff
        );

        showWithLatchGuard(mode_changed || force);
    }

public:
    LedService(AppState& st)
        : state(st), strip(Config::NUM_LEDS, Config::PIN_LED) {
    }

    const char* getName() const override { return "LED"; }

    void init() override {
        // Set maximum GPIO drive capability (40mA drive strength) for sharp signal edges
        gpio_set_drive_capability((gpio_num_t)Config::PIN_LED, GPIO_DRIVE_CAP_3);

        // Assign maximum hardware memory blocks (all 4 blocks on ESP32-S3) to RMT Channel 0
        rmt_set_mem_block_num(RMT_CHANNEL_0, 4);

        strip.Begin();
        strip.ClearTo(RgbwColor(0, 0, 0, 0));
        strip.Show();

        memset(s_shadow_pixels, 0, sizeof(s_shadow_pixels));
        memset(s_slew_pixels, 0, sizeof(s_slew_pixels));

        last_show_ms = millis();
        last_show_us = micros();
        renderStrip(true);
    }

    bool isTransmitting() const {
        return !strip.CanShow();
    }

    bool isAnimating() const {
        return (notify_frames > 0) ||
               (state.light_mode >= MODE_CANDLE && state.light_mode <= MODE_RAINBOW_FLOW) ||
               (state.light_mode == MODE_SPOTIFY_AMBI);
    }

    void update() override {
        uint32_t now = millis();
        bool brt_changed = false;

        if (state.sleep_timer_active) {
            int32_t rem = (int32_t)(state.sleep_timer_end_ms - now);
            if (rem <= 0) {
                state.sleep_timer_active = false;
                state.light_mode = MODE_LIGHT_OFF;
                state.registerActivity();
            }
        }

        if (now - last_brt_ms >= 35) {
            last_brt_ms = now;
            uint8_t target_pct = state.manual_brt_pct;
            uint8_t target_lcd = state.current_lcd_brt;

            if (state.auto_brt) {
                float factor = constrain((state.smooth_ldr - 200.0f) / 3500.0f, 0.0f, 1.0f);
                uint8_t desired_pct = (uint8_t)(10 + factor * 90.0f);
                uint8_t desired_lcd = (uint8_t)(40 + factor * 215.0f);

                if (abs((int)desired_pct - (int)state.manual_brt_pct) >= 3) {
                    target_pct = desired_pct;
                }
                if (abs((int)desired_lcd - (int)state.current_lcd_brt) >= 4) {
                    target_lcd = desired_lcd;
                }
            } else {
                target_pct = state.manual_brt_pct;
                uint8_t idx = state.manual_brt_idx % 4;
                target_lcd = lcd_levels[idx];
            }

            if (state.light_mode == MODE_LIGHT_OFF) {
                if (state.quiet_hours_active && (now - state.last_user_activity_ms > 120000)) {
                    target_lcd = 0;
                } else if (now - state.last_user_activity_ms > 20000) {
                    target_lcd = 18;
                }
            }

            if (state.sleep_timer_active && state.sleep_timer_total_ms > 0) {
                int32_t rem = max((int32_t)0, (int32_t)(state.sleep_timer_end_ms - now));
                float fade = constrain((float)rem / (float)state.sleep_timer_total_ms, 0.05f, 1.0f);
                target_pct = max((uint8_t)4, (uint8_t)(target_pct * fade));
            }

            if (state.manual_brt_pct != target_pct) {
                state.manual_brt_pct += (target_pct > state.manual_brt_pct) ? 1 : -1;
                state.current_led_brt = (uint8_t)((state.manual_brt_pct * Config::MAX_SAFE_BRT) / 100);
                brt_changed = true;
            }

            if (state.current_lcd_brt != target_lcd) {
                if (abs((int)state.current_lcd_brt - (int)target_lcd) > 4)
                    state.current_lcd_brt += (target_lcd > state.current_lcd_brt) ? 4 : -4;
                else
                    state.current_lcd_brt = target_lcd;
            }
        }

        if (state.led_notify_type != NOTIFY_NONE) {
            active_notify_type = state.led_notify_type;
            state.led_notify_type = NOTIFY_NONE;
            notify_frames = 95;
        }

        renderStrip(brt_changed);
    }
};

// Static SRAM buffer definitions
DRAM_ATTR RgbwColor LedService::s_shadow_pixels[Config::NUM_LEDS];
DRAM_ATTR RgbwColor LedService::s_slew_pixels[Config::NUM_LEDS];
