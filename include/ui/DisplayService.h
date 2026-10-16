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
        if (!canvas.createSprite(240, 240)) {
            canvas.setPsram(true);
            canvas.createSprite(240, 240);
        }
        canvas.setTextDatum(textdatum_t::middle_center);

        state.shared_canvas_buf = (uint8_t*)canvas.getBuffer();

        qr_cache.setPsram(false);
        qr_cache.setColorDepth(8);
        qr_cache.createSprite(110, 110);
    }

    bool isHighFpsNeeded() const {
        uint32_t now = millis();
        return state.ota_in_progress ||
               state.isReactionActive(now) ||
               (now - state.last_user_activity_ms < 4000) ||
               state.btn_l_pressed || state.btn_t_pressed || state.btn_r_pressed;
    }

    void update() override {
        // Не чіпаємо відеобуфер, поки Core 0 записує туди байти RGB565 з мережі
        if (state.canvas_loading) return;

        uint32_t now = millis();

        if (applied_lcd_brt != state.current_lcd_brt) {
            applied_lcd_brt = state.current_lcd_brt;
            lcd.setBrightness(applied_lcd_brt);
        }

        if (state.isCustomFrameActive(now)) {
            if (state.ui_force_dirty) {
                state.ui_force_dirty = false;
                canvas.drawCircle(120, 120, 118, canvas.color565(56, 189, 248));
                canvas.drawCircle(120, 120, 117, canvas.color565(56, 189, 248));
                canvas.pushSprite(0, 0);
            }
            return;
        }

        bool high_fps = isHighFpsNeeded();
        bool pulsing_inbox = (state.screen_mode == SCREEN_TELEGRAM && state.tg_unread);

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

        if (state.isReactionActive(now)) {
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