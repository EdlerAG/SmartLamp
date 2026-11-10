#pragma once
#include "ui/DisplayDriver.h"
#include "ui/ReactionRenderer.h"
#include "ui/ScreenRenderer.h"
#include "core/IService.h"
#include "core/AppState.h"
#include "core/StorageService.h"
#include "utils/Crc32.h"

class DisplayService : public IService {
private:
    AppState& state;
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas{&lcd};
    LGFX_Sprite qr_cache{&canvas};
    LGFX_Sprite vinylSprite{&canvas};
    LGFX_Sprite bgSprite{&canvas};
    LGFX_Sprite slideSprite{&lcd};
    bool        slide_snapshot_taken = false;

    uint32_t last_frame_ms      = 0;
    uint32_t last_anim_tick_ms  = 0;
    uint32_t last_saver_tick_ms = 0;
    uint8_t  cur_anim_frame     = 0;
    uint8_t  cur_saver_frame    = 0;
    uint8_t  applied_lcd_brt    = 220;
    char     cached_qr_str[160] = "";

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

    void resetAnimationFrames() {
        cur_anim_frame = 0;
        cur_saver_frame = 0;
        last_anim_tick_ms = millis();
        last_saver_tick_ms = millis();
    }

    LGFX_Sprite& getVinylSprite() {
        return vinylSprite;
    }

    LGFX_Sprite& getBgSprite() {
        return bgSprite;
    }

    // Розмита текстура обкладинки більше не пишеться у bgSprite: сервер шле big-endian RGB565,
    // а ESP читає little-endian, тож ця інтерполяція давала кольоровий шум замість фону.
    // Фон Spotify-деку повністю генерує ScreenRenderer::drawSmartHub.
    void updateBlurredBackground(const uint16_t* blur32) {
        if (!blur32) return;
        state.spotify_blur_ready = true;
        state.spotify_bg_dirty = true;
    }

    void loadScreensaverFromFs(StorageService& storage) {
        if (!state.psram_saver_buf) return;
        size_t bytes_read = storage.loadSaverFile(state.psram_saver_buf, sizeof(MbxHeader) + 18 * Config::FRAME_BYTES_RGB565);
        if (bytes_read >= sizeof(MbxHeader)) {
            MbxHeader* hdr = (MbxHeader*)state.psram_saver_buf;
            if (memcmp(hdr->magic, "MBX5", 4) == 0 && hdr->num_frames > 0) {
                // Перевірка цілісності CRC32 збереженої заставки у LittleFS
                if (hdr->crc32 != 0 && bytes_read > sizeof(MbxHeader)) {
                    uint32_t calc_crc = Crc32::calculate(state.psram_saver_buf + sizeof(MbxHeader), bytes_read - sizeof(MbxHeader));
                    if (calc_crc != hdr->crc32) {
                        state.saver_loaded = false;
                        state.saver_num_frames = 0;
                        return;
                    }
                }
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
        lcd.setPivot(120, 120);

        canvas.setPsram(false);
        canvas.setColorDepth(16);
        if (!canvas.createSprite(240, 240)) {
            canvas.setPsram(true);
            canvas.createSprite(240, 240);
        }
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setPivot(120, 120);

        state.shared_canvas_buf = (uint8_t*)canvas.getBuffer();

        // Розміщення QR-кешу та Spotify Vinyl у швидкісній Octal OPI PSRAM (80 MHz)
        // Це миттєво вивільняє +49 КБ внутрішньої SRAM для стабільності сокетів Wi-Fi та TLS!
        qr_cache.setPsram(true);
        qr_cache.setColorDepth(8);
        qr_cache.createSprite(110, 110);

        vinylSprite.setPsram(true);
        vinylSprite.setColorDepth(16);
        vinylSprite.createSprite(Config::VINYL_DIAMETER, Config::VINYL_DIAMETER);
        vinylSprite.setPivot(68, 68);

        bgSprite.setPsram(true);
        bgSprite.setColorDepth(16);
        bgSprite.createSprite(240, 240);
        bgSprite.fillScreen(0x0000);

        slideSprite.setPsram(true);
        slideSprite.setColorDepth(16);
        slideSprite.createSprite(240, 240);
        slideSprite.fillScreen(HallmarkTheme::BG_PRIMARY);

        vinylSprite.fillScreen(0x0000);
        // Непрозора основа вінілового диска (R = 67 px), темно-графітовий колір без 0x0000
        vinylSprite.fillCircle(68, 68, 67, canvas.color565(18, 20, 26));
        // Початкові канавки вінілу
        for (int r = 10; r < 66; r += 3) {
            vinylSprite.drawCircle(68, 68, r, HallmarkTheme::BORDER_HAIRLINE);
        }
        vinylSprite.fillCircle(68, 68, 14, canvas.color565(32, 36, 46));
        vinylSprite.fillCircle(68, 68, 4, 0x0821);
        vinylSprite.drawCircle(68, 68, 4, HallmarkTheme::BORDER_HAIRLINE);

        // Вітальний екран
        if (state.onboarded && !state.ap_mode_active) {
            state.sys_ui_state = SystemUiState::SYS_NORMAL;
            state.ui_force_dirty = true;
        } else {
            state.boot_splash_start_ms = millis();
            ScreenRenderer::drawBootSplash(canvas, 0, Config::BOOT_SPLASH_MS);
            pushCanvas();
        }
    }

    void pushCanvas() {
        int16_t ang = state.screen_angle;
        ang = (ang % 360 + 360) % 360;

        // Визначаємо найближчу апаратну орієнтацію (0°, 90°, 180°, 270°)
        uint8_t hw_rot = ((ang + 45) / 90) % 4;
        int16_t residual = ang - (hw_rot * 90);
        if (residual < -180) residual += 360;
        if (residual > 180)  residual -= 360;

        // Апаратна фіксація на контролері GC9A01 (0..3):
        // Якщо кут кратний 90° або відхилення в межах +/- 10°, фіксуємо апаратний поворот матриці.
        // Це дає 100% кришталево чистий 1:1 рендеринг по SPI DMA без жодного програмного розмиття пікселів!
        if (abs(residual) <= 10) {
            if (lcd.getRotation() != hw_rot) {
                lcd.setRotation(hw_rot);
            }
            canvas.pushSprite(0, 0);
        } else {
            // Швидкий цілочисельний поворот замість надважкого 4-точкового плаваючого AA
            // Скорочує час кадру з 35 мс до 8 мс, усуваючи просідання FPS та ривки!
            if (lcd.getRotation() != 0) {
                lcd.setRotation(0);
            }
            canvas.pushRotated(&lcd, (float)ang);
        }
    }

    bool isHighFpsNeeded() const {
        uint32_t now = millis();
        return state.slide_active ||
               state.sys_ui_state != SystemUiState::SYS_NORMAL ||
               state.ota_in_progress ||
               state.isCustomFrameActive(now) ||
               state.isReactionActive(now) ||
               state.isHudActive(now) ||
               (state.screen_mode == SCREEN_POMODORO && state.pomo_active && !state.pomo_paused) ||
               (state.screen_mode == SCREEN_LIVING_GIF && (state.saver_loaded || state.anim_frames_count > 0)) ||
               (state.screen_mode == SCREEN_SMART_HUB && (state.spotify.playing || state.tg_unread)) ||
               (now - state.last_screen_switch_ms < 350) ||
               (now - state.last_user_activity_ms < 3000) ||
               state.btn_l_pressed || state.btn_t_pressed || state.btn_r_pressed;
    }

    void renderCurrentScreen(uint32_t now) {
        canvas.fillScreen(HallmarkTheme::BG_PRIMARY);
        switch (state.screen_mode) {
            case SCREEN_CLIMATE:
                ScreenRenderer::drawClimate(canvas, state);
                break;

            case SCREEN_POMODORO:
                ScreenRenderer::drawPomodoro(canvas, state, now);
                break;

            case SCREEN_LIVING_GIF:
                if (state.saver_loaded && state.saver_num_frames > 0 && state.psram_saver_buf && state.shared_canvas_buf) {
                    uint32_t saver_delay = 1000 / (state.saver_fps > 0 ? state.saver_fps : 12);
                    if (now - last_saver_tick_ms >= saver_delay) {
                        last_saver_tick_ms = now;
                        cur_saver_frame = (cur_saver_frame + 1) % state.saver_num_frames;
                    }
                    if (cur_saver_frame >= state.saver_num_frames) cur_saver_frame = 0;
                    const uint8_t* frame_ptr = state.psram_saver_buf + sizeof(MbxHeader) + ((size_t)cur_saver_frame * Config::FRAME_BYTES_RGB565);
                    memcpy(state.shared_canvas_buf, frame_ptr, Config::FRAME_BYTES_RGB565);
                } else if (state.psram_anim_buf && state.anim_frames_count > 0 && state.shared_canvas_buf) {
                    uint32_t anim_delay = 1000 / (state.anim_fps > 0 ? state.anim_fps : 12);
                    if (now - last_anim_tick_ms >= anim_delay) {
                        last_anim_tick_ms = now;
                        cur_anim_frame = (cur_anim_frame + 1) % state.anim_frames_count;
                    }
                    if (cur_anim_frame >= state.anim_frames_count) cur_anim_frame = 0;
                    const uint8_t* frame_ptr = state.psram_anim_buf + sizeof(MbxHeader) + ((size_t)cur_anim_frame * Config::FRAME_BYTES_RGB565);
                    memcpy(state.shared_canvas_buf, frame_ptr, Config::FRAME_BYTES_RGB565);
                }
                ScreenRenderer::drawLivingGif(canvas, state, now);
                break;

            case SCREEN_SMART_HUB:
                ScreenRenderer::drawSmartHub(canvas, vinylSprite, bgSprite, state, vinyl_angle, now);
                break;

            default:
                ScreenRenderer::drawClimate(canvas, state);
                break;
        }
    }

    void renderNormalScreens(uint32_t now) {
        // Динамічний розрахунок FPS для 4 основних вікон
        uint32_t target_fps = 10;
        if (state.slide_active) {
            target_fps = 60; // 60 FPS для ідеально плавного переходу Slide
        } else if (state.isHudActive(now) || (now - state.last_screen_switch_ms < 250)) {
            target_fps = 30; // 30 FPS для плавного відображення преміального HUD-тосту
        } else if (state.screen_mode == SCREEN_POMODORO) {
            target_fps = (state.pomo_active && !state.pomo_paused) ? 10 : 4;
        } else if (state.screen_mode == SCREEN_LIVING_GIF) {
            uint8_t fps = 25;
            if (state.saver_loaded && state.saver_fps > 0) fps = state.saver_fps;
            else if (state.anim_frames_count > 0 && state.anim_fps > 0) fps = state.anim_fps;
            target_fps = fps;
        } else if (state.screen_mode == SCREEN_SMART_HUB) {
            if (state.tg_unread) target_fps = 15;
            else if (state.spotify.playing || state.spotify.isSwitchingOptimistic(now) || state.tonearm_moving || (now - state.last_user_activity_ms < 1500)) target_fps = 30;
            else target_fps = 6;
        }

        uint32_t frame_interval = 1000 / target_fps;
        if ((now - last_frame_ms < frame_interval) && !state.ui_force_dirty && !state.slide_active) return;
        last_frame_ms = now;
        state.ui_force_dirty = false;

        // Плавний перехід між екранами (Slide Transition 60 FPS, 240 мс)
        if (state.slide_active) {
            uint32_t elapsed = now - state.slide_start_ms;
            if (elapsed >= AppState::SLIDE_DURATION_MS) {
                state.slide_active = false;
                slide_snapshot_taken = false;
            } else {
                if (!slide_snapshot_taken) {
                    memcpy(slideSprite.getBuffer(), canvas.getBuffer(), Config::FRAME_BYTES_RGB565);
                    slide_snapshot_taken = true;
                }

                renderCurrentScreen(now);
                ScreenRenderer::drawPageIndicator(canvas, state);

                float p = (float)elapsed / (float)AppState::SLIDE_DURATION_MS;
                float ease = 1.0f - powf(1.0f - p, 3.0f); // Cubic ease out
                int16_t offset = (int16_t)(ease * 240.0f);
                if (offset < 0)   offset = 0;
                if (offset > 240) offset = 240;

                // Зшиваємо кадри зсуву прямо в canvas, щоб передати через pushCanvas()
                // Це гарантує 100% збереження кута повороту екрана (state.screen_angle, наприклад 294°)
                // без жодного смикання чи скидання на 0°!
                uint16_t* old_buf = (uint16_t*)slideSprite.getBuffer();
                uint16_t* new_buf = (uint16_t*)canvas.getBuffer();
                if (old_buf && new_buf && offset > 0 && offset < 240) {
                    if (state.slide_dir > 0) {
                        // Зсув вліво (новий екран насувається справа)
                        for (int y = 0; y < 240; ++y) {
                            uint16_t* n_row = new_buf + y * 240;
                            const uint16_t* o_row = old_buf + y * 240;
                            memmove(n_row + (240 - offset), n_row, offset * sizeof(uint16_t));
                            memcpy(n_row, o_row + offset, (240 - offset) * sizeof(uint16_t));
                        }
                    } else {
                        // Зсув вправо (новий екран насувається зліва)
                        for (int y = 0; y < 240; ++y) {
                            uint16_t* n_row = new_buf + y * 240;
                            const uint16_t* o_row = old_buf + y * 240;
                            memmove(n_row, n_row + (240 - offset), offset * sizeof(uint16_t));
                            memcpy(n_row + offset, o_row, (240 - offset) * sizeof(uint16_t));
                        }
                    }
                } else if (old_buf && new_buf && offset == 0) {
                    memcpy(new_buf, old_buf, Config::FRAME_BYTES_RGB565);
                }

                pushCanvas();
                return;
            }
        }

        renderCurrentScreen(now);
        ScreenRenderer::drawPageIndicator(canvas, state);
        ScreenRenderer::drawHudToast(canvas, state, now);
        pushCanvas();
    }

    void update() override {
        uint32_t now = millis();

        state.updatePomodoroLifecycle(now);

        uint8_t effective_lcd_brt = state.current_lcd_brt;
        // Нічний комфорт для очей (Night Eye Comfort без фоторезистора):
        // Якщо настала ніч (23:00..07:00), увімкнено тихі години або DND,
        // і користувач не взаємодіє з лампою понад 25 с — делікатно приглушуємо дисплей до 45 одиниць (~18%)
        bool is_night = state.isNightHours() || state.quiet_hours_active || state.dnd_mode;
        bool user_idle = (now - state.last_user_activity_ms > 25000UL) && !state.isHudActive(now);
        if (is_night && user_idle && effective_lcd_brt > 45) {
            effective_lcd_brt = 45;
        }

        if (state.isHudActive(now) && effective_lcd_brt == 0) {
            effective_lcd_brt = 120;
        }

        if (applied_lcd_brt != effective_lcd_brt) {
            bool was_off = (applied_lcd_brt == 0);
            applied_lcd_brt = effective_lcd_brt;
            if (applied_lcd_brt == 0) {
                lcd.setBrightness(0);
                lcd.sleep();
            } else {
                if (was_off) lcd.wakeup();
                lcd.setBrightness(applied_lcd_brt);
            }
        }

        // ⚡️ Енергозбереження: якщо дисплей повністю вимкнений / спить —
        // не навантажуємо CPU обчисленням графіки та шину SPI холостим DMA
        if (applied_lcd_brt == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            return;
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
                    pushCanvas();
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
            // Автоматичні переходи між кроками Onboarding v5.5 Sovereign Edition
            if (state.onboarding_step == OnboardingStep::WIZ_STEP_WIFI_AP && state.wifi_connected) {
                state.onboarding_step = OnboardingStep::WIZ_STEP_CONNECTING;
                state.ui_force_dirty = true;
            }
            if (state.onboarding_step == OnboardingStep::WIZ_STEP_CONNECTING && state.oracle_online) {
                state.onboarding_step = OnboardingStep::WIZ_STEP_WAITING_CHECKLIST;
                state.ui_force_dirty = true;
            }
            if (state.onboarding_step == OnboardingStep::WIZ_STEP_WAITING_CHECKLIST) {
                if ((state.setup_bitmask & 0x07) == 0x07) {
                    state.onboarding_step = OnboardingStep::WIZ_STEP_DONE;
                    state.trig_boot_chord = true;
                    state.ui_force_dirty = true;
                }
            }
            if (state.onboarding_step == OnboardingStep::WIZ_STEP_DONE) {
                if (state.onboarding_ready_ms == 0) {
                    state.onboarding_ready_ms = now;
                } else if (now - state.onboarding_ready_ms >= 3000) {
                    state.onboarded = true;
                    state.sys_ui_state = SystemUiState::SYS_NORMAL;
                    state.screen_mode = SCREEN_CLIMATE;
                    state.ui_force_dirty = true;
                    return;
                }
            }

            if (now - last_frame_ms >= 100 || state.ui_force_dirty) {
                last_frame_ms = now;
                state.ui_force_dirty = false;
                ScreenRenderer::drawOnboardingWizard(canvas, qr_cache, cached_qr_str, sizeof(cached_qr_str), state);
                pushCanvas();
            }
            return;
        }

        if (state.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
            if (now - last_frame_ms >= 250 || state.ui_force_dirty) {
                last_frame_ms = now;
                state.ui_force_dirty = false;
                ScreenRenderer::drawNetworkDiag(canvas, qr_cache, cached_qr_str, sizeof(cached_qr_str), state);
                pushCanvas();
            }
            return;
        }

        if (state.sys_ui_state == SystemUiState::SYS_BIRTHDAY_CELEBRATE) {
            if (now >= state.birthday_celebrate_end_ms) {
                state.sys_ui_state = SystemUiState::SYS_NORMAL;
                state.ui_force_dirty = true;
            } else {
                if (now - last_frame_ms >= 40 || state.ui_force_dirty) {
                    last_frame_ms = now;
                    state.ui_force_dirty = false;
                    canvas.fillScreen(HallmarkTheme::BG_PRIMARY);
                    ScreenRenderer::drawBirthdayCelebrate(canvas, state, now);
                    pushCanvas();
                }
                return;
            }
        }

        // ====================================================================
        // РІВЕНЬ Б: ОСНОВНІ 4 ВІКНА-ЗАСТАВКИ
        // ====================================================================

        // 1. Пріоритет: OTA оновлення (оновлюємо лише при зміні відсотка або >=500мс, щоб не забивати шину SPI)
        if (state.ota_in_progress) {
            static uint8_t s_last_rendered_ota_pct = 255;
            if (s_last_rendered_ota_pct != state.ota_progress || (now - last_frame_ms >= 500)) {
                s_last_rendered_ota_pct = state.ota_progress;
                last_frame_ms = now;
                ScreenRenderer::drawOta(canvas, state);
                pushCanvas();
            }
            return;
        }

        // 2. Пріоритет: Оверлей реакції
        if (state.isReactionActive(now)) {
            if (now - last_frame_ms >= 25) {
                last_frame_ms = now;
                ReactionRenderer::draw(canvas, state.screen_reaction_type, now);
                pushCanvas();
            }
            return;
        }

        // 3. Пріоритет: Тимчасове спливаюче медіа з Telegram
        // (ТІЛЬКИ коли діє активний таймер показу media_expire_ms > 0 і ми НЕ на екрані заставки SCREEN_LIVING_GIF)
        if (state.isCustomFrameActive(now) && state.media_expire_ms > 0 && state.screen_mode != SCREEN_LIVING_GIF) {
            uint8_t fps = state.anim_fps;
            uint32_t frame_delay = 1000 / (fps > 0 ? fps : 1);
            bool advance_frame = (state.anim_frames_count > 1) && (now - last_anim_tick_ms >= frame_delay);
            if (advance_frame) {
                last_anim_tick_ms = now;
                cur_anim_frame = (cur_anim_frame + 1) % state.anim_frames_count;
            }

            if (state.ui_force_dirty || advance_frame) {
                state.ui_force_dirty = false;

                if (state.psram_anim_buf && state.shared_canvas_buf) {
                    if (cur_anim_frame >= state.anim_frames_count) cur_anim_frame = 0;
                    const uint8_t* frame_ptr = state.psram_anim_buf + sizeof(MbxHeader) + ((size_t)cur_anim_frame * Config::FRAME_BYTES_RGB565);
                    memcpy(state.shared_canvas_buf, frame_ptr, Config::FRAME_BYTES_RGB565);
                }

                uint16_t ambi_col = canvas.color565(state.ambi_r, state.ambi_g, state.ambi_b);
                canvas.drawCircle(120, 120, 118, ambi_col);
                canvas.drawCircle(120, 120, 117, ambi_col);

                // Якщо активний таймер показу — відмальовуємо елегантний бейдж з рештою часу
                uint16_t rem_med = state.getMediaRemainingMin();
                if (rem_med > 0) {
                    canvas.fillRoundRect(80, 180, 80, 18, 9, HallmarkTheme::BG_PRIMARY);
                    canvas.drawRoundRect(80, 180, 80, 18, 9, HallmarkTheme::BORDER_HAIRLINE);
                    char med_lbl[32];
                    snprintf(med_lbl, sizeof(med_lbl), "⏱ %u хв", rem_med);
                    canvas.setFont(&UkrainianFont12Bold);
                    canvas.setTextDatum(middle_center);
                    canvas.setTextColor(HallmarkTheme::ACCENT_WARM);
                    canvas.drawString(med_lbl, 120, 189);
                }

                pushCanvas();
            }
            return;
        } else if (state.anim_frames_count > 0 && !state.canvas_loading && state.media_expire_ms > 0 && now >= state.media_expire_ms) {
            // Час дії таймера сплив — скидаємо активне тимчасове медіа
            state.stopActiveMedia();
        }

        renderNormalScreens(now);
    }
};
