#pragma once
#include "core/IService.h"
#include "Config.h"
#include "hardware/AudioService.h"
#include "core/AppState.h"

class InputService : public IService {
private:
    AppState& state;

    struct ButtonTracker {
        uint8_t   pin;
        bool      last_state;
        bool      long_fired;
        uint32_t  press_time;
        uint32_t  last_release;
        EventType short_evt;
        EventType long_evt;

        ButtonTracker() = default;
        ButtonTracker(uint8_t p, EventType s_evt, EventType l_evt)
            : pin(p), last_state(false), long_fired(false),
              press_time(0), last_release(0),
              short_evt(s_evt), long_evt(l_evt) {}
    };

    ButtonTracker btns[3];

    // 10s Combo tracking (Left + Right for Emergency Wi-Fi Reset / AP)
    uint32_t combo_start_ms = 0;
    bool     combo_active   = false;
    bool     combo_fired    = false;
    AudioService* audio_srv = nullptr;


public:
    InputService(AppState& st) : state(st) {
        btns[0] = ButtonTracker(Config::PIN_TOUCH_LEFT,  EventType::BTN_LEFT_SHORT,  EventType::BTN_LEFT_LONG);
        btns[1] = ButtonTracker(Config::PIN_TOUCH_TOP,   EventType::BTN_TOP_SHORT,   EventType::BTN_TOP_LONG);
        btns[2] = ButtonTracker(Config::PIN_TOUCH_RIGHT, EventType::BTN_RIGHT_SHORT, EventType::BTN_RIGHT_LONG);
    }

    void setAudioService(AudioService* srv) { audio_srv = srv; }

    const char* getName() const override { return "Input"; }

    void init() override {
        for (int i = 0; i < 3; i++) {
            pinMode(btns[i].pin, INPUT_PULLDOWN);
        }
    }

    void update() override {
        uint32_t now = millis();

        // 1. Опитування та оновлення стану кожної кнопки з надійним антибрязкотом
        for (int i = 0; i < 3; i++) {
            bool cur = (digitalRead(btns[i].pin) == HIGH);

            if (i == 0) state.btn_l_pressed = cur;
            if (i == 1) state.btn_t_pressed = cur;
            if (i == 2) state.btn_r_pressed = cur;

            if (cur && !btns[i].last_state && (now - btns[i].last_release >= 90)) {
                // Leading edge press (чутливий, миттєвий, без брязкоту)
                btns[i].press_time = now;
                btns[i].long_fired = false;

                // Миттєва реєстрація активності для Governor частоти CPU
                state.registerActivity();

                // Миттєвий студійний клік з просторовою інтонацією (0: Left, 1: Top, 2: Right)
                if (audio_srv) {
                    audio_srv->playClickSample(state.touch_click_profile, 1);
                }

                // Блок 3: Touch Ripple на дотик до сенсорів
                if (i == 0) state.triggerTouchRipple(-90);
                else if (i == 1) state.triggerTouchRipple(0);
                else if (i == 2) state.triggerTouchRipple(90);

            } else if (cur && btns[i].last_state && !btns[i].long_fired) {
                // Утримання клавіші
                if (now - btns[i].press_time >= 850) {
                    // Якщо зараз затиснуті обидві кнопки (Left + Right), не запускаємо звичайний long_evt!
                    bool both_lr_down = (state.btn_l_pressed && state.btn_r_pressed);
                    if ((i == 0 || i == 2) && both_lr_down) {
                        // Очікуємо Wi-Fi combo
                    } else {
                        btns[i].long_fired = true;
                        state.registerActivity();
                        // Подвійний тактильний імпульс «тук-тік» на підтвердження довгого затискання
                        // (окрім перемикання профілю кліка правою кнопкою на SCREEN_CLIMATE, де грає чисте прев'ю нового профілю)
                        if (audio_srv) {
                            bool is_climate_profile_switch = (i == 2 && state.sys_ui_state == SystemUiState::SYS_NORMAL && state.screen_mode == SCREEN_CLIMATE);
                            if (!is_climate_profile_switch) {
                                audio_srv->playClickSample(state.touch_click_profile, 3);
                            }
                        }
                        state.sendEvent(btns[i].long_evt);
                    }
                }
            } else if (!cur && btns[i].last_state) {
                // Відпускання клавіші з фільтрацією надкороткого брязкоту (<40мс)
                btns[i].last_release = now;
                if (!btns[i].long_fired && (now - btns[i].press_time >= 40)) {
                    state.sendEvent(btns[i].short_evt);
                }
            }
            btns[i].last_state = cur;
        }

        // 2. Блок 4: Обробка 10-секундного комбо Left + Right (Emergency AP Mode)
        bool lr_down = (state.btn_l_pressed && state.btn_r_pressed);
        if (lr_down) {
            if (!combo_active) {
                combo_active = true;
                combo_fired = false;
                combo_start_ms = now;
            }

            uint32_t hold_time = now - combo_start_ms;
            if (hold_time >= 1500) {
                // Пригнічуємо будь-які поодинокі кліки
                btns[0].long_fired = true;
                btns[2].long_fired = true;

                if (hold_time < 10000) {
                    float prog = (float)(hold_time - 1500) / 8500.0f;
                    state.combo_wifi_prog = (prog > 1.0f) ? 1.0f : prog;
                } else {
                    state.combo_wifi_prog = 1.0f;
                    if (!combo_fired) {
                        combo_fired = true;
                        state.sendEvent(EventType::BTN_COMBO_WIFI_RESET);
                    }
                }
            } else {
                state.combo_wifi_prog = 0.0f;
            }
        } else {
            if (combo_active) {
                combo_active = false;
                combo_fired = false;
                combo_start_ms = 0;
                state.combo_wifi_prog = 0.0f;
            }
        }
    }
};


