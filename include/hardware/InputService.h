#pragma once
#include "core/IService.h"
#include "Config.h"
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

    static volatile bool isr_pin_changed[3];
    static void isr_btn_left()  { isr_pin_changed[0] = true; }
    static void isr_btn_top()   { isr_pin_changed[1] = true; }
    static void isr_btn_right() { isr_pin_changed[2] = true; }

public:
    InputService(AppState& st) : state(st) {
        btns[0] = ButtonTracker(Config::PIN_TOUCH_LEFT,  EventType::BTN_LEFT_SHORT,  EventType::BTN_LEFT_LONG);
        btns[1] = ButtonTracker(Config::PIN_TOUCH_TOP,   EventType::BTN_TOP_SHORT,   EventType::BTN_TOP_LONG);
        btns[2] = ButtonTracker(Config::PIN_TOUCH_RIGHT, EventType::BTN_RIGHT_SHORT, EventType::BTN_RIGHT_LONG);
    }

    const char* getName() const override { return "Input"; }

    void init() override {
        for (int i = 0; i < 3; i++) {
            pinMode(btns[i].pin, INPUT);
        }
        attachInterrupt(digitalPinToInterrupt(btns[0].pin), isr_btn_left, CHANGE);
        attachInterrupt(digitalPinToInterrupt(btns[1].pin), isr_btn_top, CHANGE);
        attachInterrupt(digitalPinToInterrupt(btns[2].pin), isr_btn_right, CHANGE);
    }

    void update() override {
        uint32_t now = millis();
        for (int i = 0; i < 3; i++) {
            // Опитуємо піни лише якщо апаратне переривання зафіксувало edge, або клавіша утримується
            if (!isr_pin_changed[i] && !btns[i].last_state) {
                continue;
            }
            isr_pin_changed[i] = false;

            bool cur = (digitalRead(btns[i].pin) == HIGH);

            if (i == 0) state.btn_l_pressed = cur;
            if (i == 1) state.btn_t_pressed = cur;
            if (i == 2) state.btn_r_pressed = cur;

            if (cur && !btns[i].last_state && (now - btns[i].last_release > 120)) {
                btns[i].press_time = now;
                btns[i].long_fired = false;
            } else if (cur && btns[i].last_state && !btns[i].long_fired) {
                if (now - btns[i].press_time >= 850) {
                    btns[i].long_fired = true;
                    state.sendEvent(btns[i].long_evt);
                }
            } else if (!cur && btns[i].last_state) {
                btns[i].last_release = now;
                if (!btns[i].long_fired && (now - btns[i].press_time >= 35)) {
                    state.sendEvent(btns[i].short_evt);
                }
            }
            btns[i].last_state = cur;
        }
    }
};

__attribute__((unused)) volatile bool InputService::isr_pin_changed[3] = { false, false, false };
