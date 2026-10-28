#pragma once
#include "ui/DisplayDriver.h"
#include "ui/ReactionRenderer.h"
#include "ui/ScreenRenderer.h"
#include "core/IService.h"
#include "core/AppState.h"
#include "core/StorageService.h"

class DisplayService : public IService {
private:
    AppState& state;
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas{&lcd};
    LGFX_Sprite qr_cache{&canvas};
    LGFX_Sprite vinylSprite{&canvas};

    uint32_t last_frame_ms      = 0;
    uint32_t last_anim_tick_ms  = 0;
    uint32_t last_saver_tick_ms = 0;
    uint8_t  cur_anim_frame     = 0;
    uint8_t  cur_saver_frame    = 0;
    uint8_t  applied_lcd_brt    = 220;
    char     cached_qr_str[80]  = "";

    float smooth_hum_angle   = 180.0f;
    float smooth_pres_angle  = 270.0f;
    float vinyl_angle        = 0.0f;

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

    LGFX_Sprite& getVinylSprite() {
        return vinylSprite;
    }

    void loadScreensaverFromFs(StorageService& storage) {
        if (!state.psram_saver_buf) return;
        size_t bytes_read = storage.loadSaverFile(state.psram_saver_buf, 18 * Config::FRAME_BYTES_RGB565);
        if (bytes_read >= sizeof(MbxHeader)) {
            MbxHeader* hdr = (MbxHeader*)state.psram_saver_buf;
            if (memcmp(hdr->magic, "MBX5", 4) == 0 && hdr->num_frames > 0) {
                state.saver_num_frames = hdr->num_frames;
                state.saver_fps        = hdr->fps > 0 ? hdr->fps : 12;
                state.saver_crc        = hdr->crc32;
                state.saver_buf_size   = bytes_read;
                state.saver_loaded     = true;
                return;
            }
        }
        state.saver_loaded = false;
        state.saver_num_frames = 0;
    }

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

        // Spotify Vinyl Sprite 136x136 у PSRAM (36992 байти)
        vinylSprite.setPsram(true);
        vinylSprite.setColorDepth(16);
        vinylSprite.createSprite(Config::VINYL_DIAMETER, Config::VINYL_DIAMETER);
        vinylSprite.fillScreen(HallmarkTheme::BG_PRIMARY);
        // Початкові канавки вінілу
        for (int r = 10; r < 66; r += 3) {
            vinylSprite.drawCircle(68, 68, r, HallmarkTheme::BORDER_HAIRLINE);
        }
        vinylSprite.fillCircle(68, 68, 6, HallmarkTheme::BG_PRIMARY);
        vinylSprite.drawCircle(68, 68, 6, HallmarkTheme::BORDER_HAIRLINE);

        // Вітальний екран
        ScreenRenderer::drawBootSplash(canvas, 0, Config::BOOT_SPLASH_MS);
        canvas.pushSprite(0, 0);
    }

    bool isHighFpsNeeded() const {
        uint32_t now = millis();
        return state.sys_ui_state != SystemUiState::SYS_NORMAL ||
               state.ota_in_progress ||
               state.isCustomFrameActive(now) ||
               state.isReactionActive(now) ||
               (state.screen_mode == SCREEN_LIVING_GIF && state.saver_loaded) ||
               (state.screen_mode == SCREEN_SMART_HUB && (state.spotify.playing || state.tg_unread)) ||
               (now - state.last_user_activity_ms < 3000) ||
               state.btn_l_pressed || state.btn_t_pressed || state.btn_r_pressed;
    }

    void update() override {
        uint32_t now = millis();

        if (applied_lcd_brt != state.current_lcd_brt) {
            applied_lcd_brt = state.current_lcd_brt;
            lcd.setBrightness(applied_lcd_brt);
        }

        // ====================================================================
        // РІВЕНЬ А: СИСТЕМНІ СТАНИ ЗАПУСКУ ТА ДІАГНОСТИКИ
        // ====================================================================
        if (state.sys_ui_state == SystemUiState::SYS_BOOT_SPLASH) {
            uint32_t elapsed = now - state.boot_splash_start_ms;
            if (elapsed < Config::BOOT_SPLASH_MS) {
                if (now - last_frame_ms >= 16) {
                    last_frame_ms = now;
                    ScreenRenderer::drawBootSplash(canvas, elapsed, Config::BOOT_SPLASH_MS);
                    canvas.pushSprite(0, 0);
                }
                return;
            } else {
                if (!state.onboarded || state.ap_mode_active) {
                    state.sys_ui_state = SystemUiState::SYS_ONBOARDING_WIZARD;
                } else {
                    state.sys_ui_state = SystemUiState::SYS_NORMAL;
                }
                state.ui_force_dirty = true;
            }
        }

        if (state.sys_ui_state == SystemUiState::SYS_ONBOARDING_WIZARD) {
            uint8_t step = 1;
            if (state.wifi_connected) {
                step = 3;
                if (state.onboarding_ready_ms == 0) {
                    state.onboarding_ready_ms = now;
                    state.trig_boot_chord = true; // Акорд успіху
                } else if (now - state.onboarding_ready_ms >= 2000) {
                    state.onboarded = true;
                    state.sys_ui_state = SystemUiState::SYS_NORMAL;
                    state.screen_mode = SCREEN_CLIMATE;
                    state.ui_force_dirty = true;
                    return;
                }
            } else if (WiFi.softAPgetStationNum() > 0) {
                step = 2;
            }

            if (now - last_frame_ms >= 200 || state.ui_force_dirty) {
                last_frame_ms = now;
                state.ui_force_dirty = false;
                ScreenRenderer::drawOnboardingWizard(canvas, qr_cache, cached_qr_str, sizeof(cached_qr_str), step, state);
                canvas.pushSprite(0, 0);
            }
            return;
        }

        if (state.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
            if (now - last_frame_ms >= 250 || state.ui_force_dirty) {
                last_frame_ms = now;
                state.ui_force_dirty = false;
                ScreenRenderer::drawNetworkDiag(canvas, qr_cache, cached_qr_str, sizeof(cached_qr_str), state);
                canvas.pushSprite(0, 0);
            }
            return;
        }

        // ====================================================================
        // РІВЕНЬ Б: ОСНОВНІ 4 ВІКНА-ЗАСТАВКИ
        // ====================================================================

        // 1. Пріоритет: OTA оновлення
        if (state.ota_in_progress) {
            if (now - last_frame_ms >= 33) {
                last_frame_ms = now;
                ScreenRenderer::drawOta(canvas, state);
                canvas.pushSprite(0, 0);
            }
            return;
        }

        // 2. Пріоритет: Оверлей реакції
        if (state.isReactionActive(now)) {
            if (now - last_frame_ms >= 25) {
                last_frame_ms = now;
                ReactionRenderer::draw(canvas, state.screen_reaction_type, now);
                canvas.pushSprite(0, 0);
            }
            return;
        }

        // 3. Пріоритет: Тимчасовий стрім мультикадрового .mbx із PSRAM
        if (state.isCustomFrameActive(now)) {
            uint8_t fps = state.anim_fps;
            uint32_t frame_delay = 1000 / (fps > 0 ? fps : 1);
            if (state.ui_force_dirty || (state.anim_frames_count > 1 && (now - last_anim_tick_ms >= frame_delay))) {
                state.ui_force_dirty = false;
                last_anim_tick_ms = now;

                if (state.psram_anim_buf && state.shared_canvas_buf) {
                    cur_anim_frame = (cur_anim_frame + 1) % state.anim_frames_count;
                    const uint8_t* frame_ptr = state.psram_anim_buf + ((size_t)cur_anim_frame * Config::FRAME_BYTES_RGB565);
                    memcpy(state.shared_canvas_buf, frame_ptr, Config::FRAME_BYTES_RGB565);
                }

                uint16_t ambi_col = canvas.color565(state.ambi_r, state.ambi_g, state.ambi_b);
                canvas.drawCircle(120, 120, 118, ambi_col);
                canvas.drawCircle(120, 120, 117, ambi_col);
                canvas.pushSprite(0, 0);
            }
            return;
        }

        // Динамічний розрахунок FPS для 4 основних вікон
        uint32_t target_fps = 10;
        if (state.screen_mode == SCREEN_LIVING_GIF) {
            target_fps = (state.saver_loaded && state.saver_fps > 0) ? state.saver_fps : 25;
        } else if (state.screen_mode == SCREEN_SMART_HUB) {
            if (state.tg_unread) target_fps = 15;
            else if (state.spotify.playing) target_fps = 30; // 30 FPS для плавного обертання вінілу
            else target_fps = 4;
        } else if (state.screen_mode == SCREEN_BAROMETER) {
            target_fps = 4;
        }

        uint32_t frame_interval = 1000 / target_fps;
        if ((now - last_frame_ms < frame_interval) && !state.ui_force_dirty) return;
        last_frame_ms = now;
        state.ui_force_dirty = false;

        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);

        switch (state.screen_mode) {
            case SCREEN_CLIMATE:
                ScreenRenderer::drawClimate(canvas, state, smooth_hum_angle);
                break;

            case SCREEN_LIVING_GIF:
                if (state.saver_loaded && state.saver_num_frames > 0 && state.psram_saver_buf && state.shared_canvas_buf) {
                    uint32_t saver_delay = 1000 / (state.saver_fps > 0 ? state.saver_fps : 12);
                    if (now - last_saver_tick_ms >= saver_delay) {
                        last_saver_tick_ms = now;
                        cur_saver_frame = (cur_saver_frame + 1) % state.saver_num_frames;
                    }
                    const uint8_t* frame_ptr = state.psram_saver_buf + sizeof(MbxHeader) + ((size_t)cur_saver_frame * Config::FRAME_BYTES_RGB565);
                    memcpy(state.shared_canvas_buf, frame_ptr, Config::FRAME_BYTES_RGB565);
                }
                ScreenRenderer::drawLivingGif(canvas, state, now);
                break;

            case SCREEN_SMART_HUB:
                ScreenRenderer::drawSmartHub(canvas, vinylSprite, state, vinyl_angle, now);
                break;

            case SCREEN_BAROMETER:
                ScreenRenderer::drawBarometer(canvas, state, smooth_pres_angle);
                break;

            default:
                ScreenRenderer::drawClimate(canvas, state, smooth_hum_angle);
                break;
        }

        ScreenRenderer::drawFooterNav(canvas, state);
        canvas.pushSprite(0, 0);
    }
};
