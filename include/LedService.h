#pragma once
#include "IService.h"
#include "Config.h"
#include "AppState.h"
#include <Adafruit_NeoPixel.h>

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

    const uint8_t manual_levels[4] = {20, 48, 88, Config::MAX_SAFE_BRT};
    const uint8_t lcd_levels[4]    = {60, 130, 200, 255};

    void renderStrip(bool force) {
        uint32_t now = millis();
        bool mode_changed = (state.light_mode != last_mode);
        last_mode = state.light_mode;

        // ==========================================================
        // 1. ВІДМАЛЬОВКА 5 МЕМ-РЕАКЦІЙ ТА СПОВІЩЕНЬ
        // ==========================================================
        if (notify_frames > 0) {
            if (now - last_anim_ms >= 20) {
                last_anim_ms = now;
                notify_frames--;
                uint8_t wave = strip.sine8(notify_frames * 12);

                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    if (active_notify_type == NOTIFY_FIRE) {
                        uint8_t spark = strip.sine8((notify_frames * 19) + (i * 23));
                        strip.setPixelColor(i, strip.Color(200 + (spark >> 3), (spark * 110) >> 8, 0, (spark > 210) ? 60 : 0));
                    }
                    else if (active_notify_type == NOTIFY_LIGHTNING) {
                        bool flash = ((notify_frames % 14 == 0) || (notify_frames % 14 == 2));
                        if (flash) strip.setPixelColor(i, strip.Color(80, 160, 255, 220));
                        else {
                            uint8_t glow = strip.sine8((notify_frames * 9) + (i * 14));
                            strip.setPixelColor(i, strip.Color((glow * 40) >> 8, (glow * 90) >> 8, glow, 0));
                        }
                    }
                    else if (active_notify_type == NOTIFY_CAT) {
                        uint8_t paw = strip.sine8((notify_frames * 8) + (i * 18));
                        if ((i / 6) % 2 == 0) strip.setPixelColor(i, strip.Color(0, paw, (paw * 170) >> 8, (paw * 40) >> 8));
                        else strip.setPixelColor(i, strip.Color(paw, (paw * 140) >> 8, 0, (paw * 30) >> 8));
                    }
                    else if (active_notify_type == NOTIFY_UFO) {
                        // 👽 НЛО: інопланетний неоново-зелений промінь з фіолетовими спалахами
                        uint8_t beam = strip.sine8((notify_frames * 14) - (i * 16));
                        strip.setPixelColor(i, strip.Color((255 - beam) >> 2, beam, (255 - beam) >> 1, 0));
                    }
                    else if (active_notify_type == NOTIFY_CROWN) {
                        // 👑 КОРОНА: королівське золото та пурпур
                        uint8_t sh = strip.sine8((notify_frames * 10) + (i * 20));
                        strip.setPixelColor(i, strip.Color(220, (sh * 160) >> 8, (255 - sh) >> 1, (sh > 200) ? 80 : 10));
                    }
                    else {
                        strip.setPixelColor(i, strip.Color(0, (wave * 180) >> 8, wave, (wave * 70) >> 8));
                    }
                }
                strip.show();
                if (notify_frames == 0) last_mode = -1;
            }
            return;
        }

        // ==========================================================
        // 2. СТАТИЧНІ РЕЖИМИ (Zero-CPU)
        // ==========================================================
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

        // ==========================================================
        // 3. ДИНАМІЧНІ РЕЖИМИ ТА СВІЧКА (45 FPS)
        // ==========================================================
        if (now - last_anim_ms < 22 && !mode_changed && !force) return;
        last_anim_ms = now;

        switch (state.light_mode) {
            case MODE_CANDLE: { // 🕯 ЖИВА СВІЧКА
                uint8_t wind = strip.sine8(anim_step * 2);
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t flk1 = strip.sine8((anim_step * 5) + (i * 9));
                    uint8_t flk2 = strip.sine8((anim_step * 11) - (i * 5) + (wind >> 2));
                    uint8_t flame = ((uint16_t)flk1 + (uint16_t)flk2) >> 1;
                    strip.setPixelColor(i, strip.Color(180 + ((flame * 75) >> 8), 45 + ((flame * 65) >> 8), 0, 35 + ((flame * 95) >> 8)));
                }
                anim_step++;
                break;
            }

            case MODE_AMBER_EMBER: { // 🍯 БУРШТИНОВА СВІЧКА
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t breath = strip.sine8((anim_step * 2) + (i * 4));
                    strip.setPixelColor(i, strip.Color(150 + ((breath * 105) >> 8), 25 + ((breath * 55) >> 8), 0, 10 + ((breath * 45) >> 8)));
                }
                anim_step++;
                break;
            }

            case MODE_FIREPLACE: { // 🔥 КАМІН
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t f1 = strip.sine8((anim_step * 5) + (i * 17));
                    uint8_t f2 = strip.sine8((anim_step * 9) - (i * 11));
                    uint8_t flk = (f1 + f2) >> 1;
                    strip.setPixelColor(i, strip.Color(170 + ((flk * 85) >> 8), 15 + ((flk * 80) >> 8), 0, (flk > 200) ? ((flk - 200) >> 1) : 0));
                }
                anim_step++;
                break;
            }

            case MODE_SUNSET_VIBE: { // 🌅 ЗАХІД СОНЦЯ
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t wave = strip.sine8((anim_step * 2) + (i * 6));
                    strip.setPixelColor(i, strip.Color(190 + ((wave * 65) >> 8), (wave * 70) >> 8, ((255 - wave) * 110) >> 8, (wave * 35) >> 8));
                }
                anim_step++;
                break;
            }

            case MODE_SAKURA_NEON: { // 🌸 САКУРА НЕОН
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t wv = strip.sine8((anim_step * 2) + (i * 8));
                    uint8_t r = 130 + ((wv * 125) >> 8);
                    uint8_t g = ((255 - wv) * 140) >> 8;
                    uint8_t b = 120 + ((wv * 110) >> 8);
                    uint8_t w = (wv > 190) ? 25 : 5;
                    strip.setPixelColor(i, strip.Color(r, g, b, w));
                }
                anim_step++;
                break;
            }

            case MODE_AURORA_WAVE: { // 🌌 ПІВНІЧНЕ СЯЙВО
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t s1 = strip.sine8((anim_step * 2) + (i * 7));
                    uint8_t s2 = strip.sine8(64 - (anim_step * 2) + (i * 5));
                    strip.setPixelColor(i, strip.Color(0, 15 + ((s1 * 215) >> 8), 40 + ((s2 * 210) >> 8), (s1 > 180) ? ((s1 - 180) >> 1) : 0));
                }
                anim_step++;
                break;
            }

            case MODE_OCEAN_BREEZE: { // 🌊 ОКЕАНСЬКИЙ БРИЗ
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint8_t tide = strip.sine8((anim_step * 3) + (i * 6));
                    uint8_t foam = (tide > 215) ? ((tide - 215) * 3) : 0;
                    strip.setPixelColor(i, strip.Color(0, (tide * 165) >> 8, 80 + ((tide * 175) >> 8), foam));
                }
                anim_step++;
                break;
            }

            case MODE_CYBER_RUNNER: { // ⚡ КІБЕР-КОМЕТА
                uint32_t period = (uint32_t)Config::NUM_LEDS * 100;
                uint32_t head_pos = ((uint32_t)anim_step * 64) % period;
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint32_t dist = (uint32_t)abs((int32_t)(i * 100) - (int32_t)head_pos);
                    if (dist > period / 2) dist = period - dist;

                    uint8_t intensity = (dist < 800) ? (uint8_t)(255 - (dist * 255) / 800) : 0;
                    uint8_t sq = (intensity * intensity) >> 8;
                    strip.setPixelColor(i, strip.Color((sq * 190) >> 8, ((sq * 45) >> 8) + 6, ((sq * 235) >> 8) + 20, 0));
                }
                anim_step++;
                break;
            }

            case MODE_RAINBOW_FLOW: { // 🌈 ВЕСЕЛКА
                for (int i = 0; i < Config::NUM_LEDS; i++) {
                    uint16_t hue = (anim_step * 220 + (i * 65536L / Config::NUM_LEDS)) & 0xFFFF;
                    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, 255)));
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
        return (notify_frames > 0) ||
               (state.light_mode >= MODE_CANDLE && state.light_mode <= MODE_RAINBOW_FLOW);
    }

    void update() override {
        uint32_t now = millis();
        bool brt_changed = false;

        // 1. Перевірка Таймера сну (Sleep Timer)
        if (state.sleep_timer_active) {
            int32_t rem = (int32_t)(state.sleep_timer_end_ms - now);
            if (rem <= 0) {
                state.sleep_timer_active = false;
                state.light_mode = MODE_LIGHT_OFF;
                state.registerActivity();
            }
        }

        // 2. Плавне регулювання яскравості з урахуванням таймера сну
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

            // Якщо активний таймер сну — плавно приглушуємо яскравість пропорційно залишку часу
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