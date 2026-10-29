#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "led/LedEffects.h"

class LedService : public IService {
private:
    AppState& state;
    Adafruit_NeoPixel strip;
    uint32_t last_anim_ms       = 0;
    uint32_t last_brt_ms        = 0;
    uint16_t anim_step          = 0;
    int      last_mode          = -1;
    uint16_t notify_frames      = 0;
    uint8_t  active_notify_type = NOTIFY_NONE;

    float    cur_r              = 0.0f;
    float    cur_g              = 0.0f;
    float    cur_b              = 0.0f;
    uint32_t last_lerp_ms       = 0;

    const uint8_t manual_levels[4] = {20, 48, 88, Config::MAX_SAFE_BRT};
    const uint8_t lcd_levels[4]    = {60, 130, 200, 255};

    void renderStrip(bool force) {
        uint32_t now = millis();
        bool mode_changed = (state.light_mode != last_mode);
        last_mode = state.light_mode;

        uint8_t target_r = state.ambi_r;
        uint8_t target_g = state.ambi_g;
        uint8_t target_b = state.ambi_b;
        if (state.light_mode == MODE_SPOTIFY_AMBI && state.spotify.playing) {
            target_r = state.spotify.ambi_r;
            target_g = state.spotify.ambi_g;
            target_b = state.spotify.ambi_b;
        }

        // Преміальний плавний перехід кольору (RGB lerp ~350мс) замість різкого стрибка
        if (mode_changed || force) {
            cur_r = (float)target_r;
            cur_g = (float)target_g;
            cur_b = (float)target_b;
            last_lerp_ms = now;
        } else if (now - last_lerp_ms >= 20) {
            last_lerp_ms = now;
            cur_r += ((float)target_r - cur_r) * 0.15f;
            cur_g += ((float)target_g - cur_g) * 0.15f;
            cur_b += ((float)target_b - cur_b) * 0.15f;
        }

        uint8_t eff_r = (uint8_t)constrain((int)(cur_r + 0.5f), 0, 255);
        uint8_t eff_g = (uint8_t)constrain((int)(cur_g + 0.5f), 0, 255);
        uint8_t eff_b = (uint8_t)constrain((int)(cur_b + 0.5f), 0, 255);

        if (notify_frames > 0) {
            if (now - last_anim_ms >= 20) {
                last_anim_ms = now;
                notify_frames--;
                LedEffects::renderNotification(strip, active_notify_type, notify_frames, eff_r, eff_g, eff_b);
                strip.show();
                if (notify_frames == 0) last_mode = -1;
            }
            return;
        }

        if (state.light_mode == MODE_LIGHT_OFF) {
            if (mode_changed) {
                strip.clear();
                strip.show();
            }
            return;
        }

        if (state.light_mode == MODE_WARM_COZY) {
            if (mode_changed || force) {
                strip.fill(strip.Color(0, 0, 0, 225), 0, Config::NUM_LEDS);
                strip.show();
            }
            return;
        }

        if (now - last_anim_ms < 22 && !mode_changed && !force) return;
        last_anim_ms = now;

        LedEffects::renderDynamicMode(strip, state.light_mode, anim_step++, eff_r, eff_g, eff_b);
        strip.show();
    }

public:
    LedService(AppState& st)
        : state(st), strip(Config::NUM_LEDS, Config::PIN_LED, NEO_GRBW + NEO_KHZ800) {}

    const char* getName() const override { return "LED"; }

    void init() override {
        strip.begin();
        strip.setBrightness(state.current_led_brt);
        renderStrip(true);
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

        if (now - last_brt_ms >= 40) {
            last_brt_ms = now;
            uint8_t target_led = state.current_led_brt;
            uint8_t target_lcd = state.current_lcd_brt;

            if (state.auto_brt) {
                float factor = constrain((state.smooth_ldr - 200.0f) / 3500.0f, 0.0f, 1.0f);
                target_led = (uint8_t)(Config::MIN_SAFE_BRT + factor * (Config::MAX_SAFE_BRT - Config::MIN_SAFE_BRT));
                target_lcd = (uint8_t)(40 + factor * 215.0f);
            } else {
                uint8_t idx = state.manual_brt_idx % 4;
                target_led = manual_levels[idx];
                target_lcd = lcd_levels[idx];
            }

            // Якщо світло вимкнене:
            // - під час тихих годин (Quiet Hours) після 2 хв неактивності гасимо екран повністю (глибокий сон)
            // - вдень після 20 с м'яко приглушуємо до 18
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
                target_led = max((uint8_t)4, (uint8_t)(target_led * fade));
            }

            if (state.current_led_brt != target_led) {
                state.current_led_brt += (target_led > state.current_led_brt) ? 1 : -1;
                strip.setBrightness(state.current_led_brt);
                brt_changed = true;
                state.ui_force_dirty = true;
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
