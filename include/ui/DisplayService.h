#pragma once
#include "ui/DisplayDriver.h"
#include "ui/ReactionRenderer.h"
#include "ui/ScreenRenderer.h"
#include "core/IService.h"
#include "core/AppState.h"

class DisplayService : public IService {
private:
    AppState& state;
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas{&lcd};
    LGFX_Sprite qr_cache{&canvas};

    uint32_t last_frame_ms     = 0;
    uint8_t  applied_lcd_brt   = 200;
    char     cached_qr_str[80] = "";

    float smooth_hum_angle  = 180.0f;
    float smooth_pres_angle = 270.0f;

    // Перевіряє, чи плавні стрілки вже дійшли до цільових значень
    bool isGaugeSettled() const {
        float target_hum = 180.0f + constrain(state.humidity, 0.0f, 100.0f) * 1.8f;
        float clamped_p  = constrain(state.pressure, 730.0f, 770.0f);
        float target_p   = 135.0f + ((clamped_p - 730.0f) / 40.0f * 270.0f);
        return (fabs(target_hum - smooth_hum_angle) < 0.3f) &&
               (fabs(target_p - smooth_pres_angle) < 0.3f);
    }

public:
    DisplayService(AppState& st) : state(st) {}

    const char* getName() const override { return "Display"; }

    void init() override {
        lcd.init();
        lcd.setRotation(0);
        lcd.setBrightness(state.current_lcd_brt);

        canvas.setPsram(false);
        canvas.setColorDepth(16);
        canvas.createSprite(240, 240);
        canvas.setTextDatum(textdatum_t::middle_center);

        qr_cache.setPsram(false);
        qr_cache.setColorDepth(8);
        qr_cache.createSprite(110, 110);
    }

    bool isHighFpsNeeded() const {
        uint32_t now = millis();
        return state.ota_in_progress ||
               (now < state.screen_reaction_until) ||
               (now - state.last_user_activity_ms < 4000) ||
               state.btn_l_pressed || state.btn_t_pressed || state.btn_r_pressed;
    }

    void update() override {
        uint32_t now = millis();

        // Оновлюємо апаратну яскравість підсвітки навіть без перемальовки кадру
        if (applied_lcd_brt != state.current_lcd_brt) {
            applied_lcd_brt = state.current_lcd_brt;
            lcd.setBrightness(applied_lcd_brt);
        }

        bool high_fps = isHighFpsNeeded();
        bool pulsing_inbox = (state.screen_mode == SCREEN_TELEGRAM && state.tg_unread);

        // ZERO-SPI IDLE: якщо немає активності, немає пульсуючого повідомлення і шкали стабілізувалися —
        // пропускаємо рендеринг та SPI-транзакцію повністю (0% CPU)!
        if (!high_fps && !state.ui_force_dirty && !pulsing_inbox && isGaugeSettled()) {
            return;
        }

        uint32_t frame_interval = high_fps ? 16 : 120;
        if (now - last_frame_ms < frame_interval && !state.ui_force_dirty) return;
        last_frame_ms = now;
        state.ui_force_dirty = false;

        canvas.fillScreen(canvas.color565(6, 9, 16));

        if (state.ota_in_progress) {
            ScreenRenderer::drawOta(canvas, state);
            canvas.pushSprite(0, 0);
            return;
        }

        if (now < state.screen_reaction_until && state.screen_reaction_type >= NOTIFY_FIRE) {
            ReactionRenderer::draw(canvas, state.screen_reaction_type, now);
            canvas.pushSprite(0, 0);
            return;
        }

        switch (state.screen_mode) {
            case SCREEN_CLIMATE:   ScreenRenderer::drawClimate(canvas, state, smooth_hum_angle); break;
            case SCREEN_BAROMETER: ScreenRenderer::drawBarometer(canvas, state, smooth_pres_angle); break;
            case SCREEN_TELEGRAM:  ScreenRenderer::drawTelegram(canvas, state); break;
            case SCREEN_NETWORK:   ScreenRenderer::drawNetworkQr(canvas, qr_cache, cached_qr_str, sizeof(cached_qr_str), state); break;
        }

        ScreenRenderer::drawFooterNav(canvas, state);
        canvas.pushSprite(0, 0);
    }
};