#pragma once
#include "IService.h"
#include "Config.h"
#include "AppState.h"
#include <Adafruit_NeoPixel.h>

class LedService : public IService {
private:
    AppState& state;
    Adafruit_NeoPixel strip;
    uint32_t last_anim_ms  = 0;
    uint32_t last_brt_ms   = 0;
    uint16_t anim_step     = 0;
    int      last_mode     = -1;
    uint16_t notify_frames = 0;
    uint8_t  active_notify_type = NOTIFY_NONE;

    const uint8_t manual_levels[3] = {25, 65, 125};

    void renderStrip(bool force) {
        uint32_t now = millis();
        bool mode_changed = (state.light_mode != last_mode);
        last_mode = state.light_mode;

        // Відмальовка спец-ефектів сповіщень (Бірюзова хвиля або Рожевий пульс сердечка)
        if (notify_frames > 0) {
            if (now - last_anim_ms >= 20) {
                last_anim_ms = now;
                notify_frames--;
                uint8_t wave = strip.sine8(notify_frames * 11);

                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    if (active_notify_type == NOTIFY_LOVE_PULSE) {
                        // Ніжний рожево-рубіновий пульс сердечка
                        strip.setPixelColor(i, strip.Color(wave, (wave * 20) >> 8, (wave * 95) >> 8, (wave * 35) >> 8));
                    } else {
                        // Бірюзова хвиля нового повідомлення
                        strip.setPixelColor(i, strip.Color(0, (wave * 180) >> 8, wave, (wave * 70) >> 8));
                    }
                }
                strip.show();
                if (notify_frames == 0) last_mode = -1;
            }
            return;
        }

        if (state.light_mode == 5) {
            if (mode_changed) {
                strip.clear();
                strip.show();
            }
            return;
        }

        if (state.light_mode == 0) {
            if (mode_changed || force) {
                strip.fill(strip.Color(0, 0, 0, 220), 0, Config::NUM_LEDS);
                strip.show();
            }
            return;
        }

        if (now - last_anim_ms < 22 && !mode_changed && !force) return;
        last_anim_ms = now;

        switch (state.light_mode) {
            case 1: { // RAINBOW FLOW
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint16_t hue = (anim_step * 220 + (i * 65536L / Config::NUM_LEDS)) & 0xFFFF;
                    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, 255)));
                }
                anim_step++;
                break;
            }
            case 2: { // AURORA WAVE
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t s1 = strip.sine8((anim_step * 2) + (i * 7));
                    uint8_t s2 = strip.sine8(64 - (anim_step * 2) + (i * 5));
                    uint8_t g  = 15 + ((s1 * 215) >> 8);
                    uint8_t b  = 40 + ((s2 * 210) >> 8);
                    uint8_t w  = (s1 > 180) ? ((s1 - 180) >> 1) : 0;
                    strip.setPixelColor(i, strip.Color(0, g, b, w));
                }
                anim_step++;
                break;
            }
            case 3: { // CYBER RUNNER — uint32_t щоб уникнути переповнення при довгій роботі
                uint32_t period = (uint32_t)Config::NUM_LEDS * 100;
                uint32_t head_pos = ((uint32_t)anim_step * 64) % period;
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint32_t dist = (uint32_t)abs((int32_t)(i * 100) - (int32_t)head_pos);
                    if (dist > period / 2) dist = period - dist;

                    uint8_t intensity = (dist < 800) ? (uint8_t)(255 - (dist * 255) / 800) : 0;
                    uint8_t sq = (intensity * intensity) >> 8;
                    uint8_t r  = (sq * 190) >> 8;
                    uint8_t g  = ((sq * 45) >> 8) + 6;
                    uint8_t b  = ((sq * 235) >> 8) + 20;
                    strip.setPixelColor(i, strip.Color(r, g, b, 0));
                }
                anim_step++;
                break;
            }
            case 4: { // FIRE GLOW
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t f1 = strip.sine8((anim_step * 4) + (i * 15));
                    uint8_t f2 = strip.sine8((anim_step * 7) - (i * 9));
                    uint8_t flk = (f1 + f2) >> 1;
                    uint8_t r = 165 + ((flk * 90) >> 8);
                    uint8_t g = 20  + ((flk * 85) >> 8);
                    uint8_t w = (flk * 45) >> 8;
                    strip.setPixelColor(i, strip.Color(r, g, 0, w));
                }
                anim_step++;
                break;
            }
        }
        strip.show();
    }

public:
    LedService(AppState& st)
        : state(st), strip(Config::NUM_LEDS, Config::PIN_LED, NEO_GRBW + NEO_KHZ800) {}

    void init() override {
        strip.begin();
        strip.setBrightness(state.current_led_brt);
        renderStrip(true);
    }

    bool isAnimating() const {
        return (notify_frames > 0) || (state.light_mode >= 1 && state.light_mode <= 4);
    }

    void update() override {
        uint32_t now = millis();
        bool brt_changed = false;

        if (now - last_brt_ms >= 45) {
            last_brt_ms = now;
            uint8_t target_led = state.current_led_brt;
            uint8_t target_lcd = state.current_lcd_brt;

            if (state.auto_brt) {
                float factor = constrain((state.smooth_ldr - 200.0f) / 3500.0f, 0.0f, 1.0f);
                target_led = (uint8_t)(Config::MIN_SAFE_BRT + factor * (Config::MAX_SAFE_BRT - Config::MIN_SAFE_BRT));
                target_lcd = (uint8_t)(35 + factor * 220.0f);
            } else {
                target_led = manual_levels[state.manual_brt_idx];
                target_lcd = (state.manual_brt_idx == 0) ? 70 : ((state.manual_brt_idx == 1) ? 175 : 255);
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
            notify_frames = 85;
        }

        renderStrip(brt_changed);
    }
};