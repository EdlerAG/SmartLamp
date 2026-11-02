#pragma once
#include <Arduino.h>
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

    // Плавний RGB lerp для зміни палітри (~350-600мс)
    float    cur_r              = 0.0f;
    float    cur_g              = 0.0f;
    float    cur_b              = 0.0f;
    float    cur_s_r            = 0.0f;
    float    cur_s_g            = 0.0f;
    float    cur_s_b            = 0.0f;
    uint32_t last_lerp_ms       = 0;

    // Тіньовий буфер для Dirty-flag frame guard (Zero redundant bus transactions)
    RgbwColor shadow_pixels[Config::NUM_LEDS];

    // Швидкі пресети ручної яскравості
    const uint8_t manual_pct_levels[4] = {15, 35, 65, 100};
    const uint8_t lcd_levels[4]        = {60, 130, 200, 255};

    void showWithLatchGuard(bool force = false) {
        uint32_t now = millis();

        // Перевіряємо чи змінився кадр у буфері порівняно з тіньовим
        bool frame_dirty = false;
        for (int i = 0; i < Config::NUM_LEDS; i++) {
            RgbwColor cur = strip.GetPixelColor(i);
            if (cur != shadow_pixels[i]) {
                frame_dirty = true;
                shadow_pixels[i] = cur;
            }
        }

        // Пропускаємо виклик Show(), якщо кадр статичний і не минуло 2 секунди
        if (!frame_dirty && !force && (now - last_show_ms < 2000)) {
            return;
        }

        // Суворий контроль Reset/Latch guard: чекаємо завершення попереднього RMT кадру
        while (!strip.CanShow()) {
            taskYIELD();
        }

        // Гарантований час скидання лінії SK6812 >= 85 мкс між викликами
        uint32_t elapsed_us = micros() - last_show_us;
        if (elapsed_us < 90) {
            delayMicroseconds(90 - elapsed_us);
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
        uint8_t target_s_r = state.spotify.ambi2_r;
        uint8_t target_s_g = state.spotify.ambi2_g;
        uint8_t target_s_b = state.spotify.ambi2_b;

        if (state.light_mode == MODE_SPOTIFY_AMBI) {
            if (state.spotify.ambi_r != 0 || state.spotify.ambi_g != 0 || state.spotify.ambi_b != 0) {
                target_r = state.spotify.ambi_r;
                target_g = state.spotify.ambi_g;
                target_b = state.spotify.ambi_b;
            }
        }

        // Плавний перехід кольору (RGB lerp ~350-500мс)
        if (mode_changed || force) {
            cur_r = (float)target_r;
            cur_g = (float)target_g;
            cur_b = (float)target_b;
            cur_s_r = (float)target_s_r;
            cur_s_g = (float)target_s_g;
            cur_s_b = (float)target_s_b;
            last_lerp_ms = now;
        } else if (now - last_lerp_ms >= 20) {
            last_lerp_ms = now;
            cur_r += ((float)target_r - cur_r) * 0.075f;
            cur_g += ((float)target_g - cur_g) * 0.075f;
            cur_b += ((float)target_b - cur_b) * 0.075f;
            cur_s_r += ((float)target_s_r - cur_s_r) * 0.075f;
            cur_s_g += ((float)target_s_g - cur_s_g) * 0.075f;
            cur_s_b += ((float)target_s_b - cur_s_b) * 0.075f;
        }

        uint8_t eff_r = (uint8_t)constrain((int)(cur_r + 0.5f), 0, 255);
        uint8_t eff_g = (uint8_t)constrain((int)(cur_g + 0.5f), 0, 255);
        uint8_t eff_b = (uint8_t)constrain((int)(cur_b + 0.5f), 0, 255);
        uint8_t eff_s_r = (uint8_t)constrain((int)(cur_s_r + 0.5f), 0, 255);
        uint8_t eff_s_g = (uint8_t)constrain((int)(cur_s_g + 0.5f), 0, 255);
        uint8_t eff_s_b = (uint8_t)constrain((int)(cur_s_b + 0.5f), 0, 255);

        float brt_factor = (float)state.manual_brt_pct / 100.0f;
        brt_factor = constrain(brt_factor, 0.01f, 1.0f);

        // 1. Сповіщення / реакція поверх усього
        if (notify_frames > 0) {
            if (now - last_anim_ms >= 20) {
                last_anim_ms = now;
                notify_frames--;
                LedEffects::renderNotification(strip, active_notify_type, notify_frames, brt_factor, eff_r, eff_g, eff_b);
                showWithLatchGuard(true);
                if (notify_frames == 0) last_mode = -1;
            }
            return;
        }

        // 2. Режим ВИМКНЕНО
        if (state.light_mode == MODE_LIGHT_OFF) {
            if (mode_changed || force) {
                strip.ClearTo(RgbwColor(0, 0, 0, 0));
                showWithLatchGuard(true);
            }
            return;
        }

        // 3. Обмеження кадрової частоти анімацій (45 FPS max)
        if (now - last_anim_ms < 22 && !mode_changed && !force) return;
        last_anim_ms = now;

        LedEffects::renderDynamicMode(
            strip,
            state.light_mode,
            anim_step++,
            brt_factor,
            state.anim_speed,
            eff_r, eff_g, eff_b,
            eff_s_r, eff_s_g, eff_s_b,
            (state.light_mode == MODE_SPOTIFY_AMBI) ? &state.spotify : nullptr,
            state.custom_r, state.custom_g, state.custom_b, state.custom_w
        );

        showWithLatchGuard(mode_changed || force);
    }

public:
    LedService(AppState& st)
        : state(st), strip(Config::NUM_LEDS, Config::PIN_LED) {
        memset(shadow_pixels, 0, sizeof(shadow_pixels));
    }

    const char* getName() const override { return "LED"; }

    void init() override {
        strip.Begin();
        strip.ClearTo(RgbwColor(0, 0, 0, 0));
        strip.Show();
        last_show_ms = millis();
        last_show_us = micros();
        renderStrip(true);
    }

    // Чи зайнятий RMT передачею даних на діоди (Zero-Glitch lock для регулятора частоти CPU)
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

        if (now - last_brt_ms >= 40) {
            last_brt_ms = now;
            uint8_t target_pct = state.manual_brt_pct;
            uint8_t target_lcd = state.current_lcd_brt;

            if (state.auto_brt) {
                float factor = constrain((state.smooth_ldr - 200.0f) / 3500.0f, 0.0f, 1.0f);
                uint8_t desired_pct = (uint8_t)(10 + factor * 90.0f);
                uint8_t desired_lcd = (uint8_t)(40 + factor * 215.0f);

                // Вікно гістерезису для автояскравості (запобігає джиттеру через шум фоторезистора)
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

            // Якщо світло вимкнене:
            // - під час тихих годин після 2 хв неактивності гасимо екран повністю
            // - вдень після 20 с приглушуємо до 18
            if (state.light_mode == MODE_LIGHT_OFF) {
                if (state.quiet_hours_active && (now - state.last_user_activity_ms > 120000)) {
                    target_lcd = 0;
                } else if (now - state.last_user_activity_ms > 20000) {
                    target_lcd = 18;
                }
            }

            // Загасання таймера сну
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
