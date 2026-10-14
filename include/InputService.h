#pragma once
#include "IService.h"
#include "Config.h"
#include "AppState.h"

class InputService : public IService {
private:
    AppState& state;

    struct ButtonTracker {
        uint8_t  pin;
        bool     last_state;
        bool     long_fired;
        uint32_t press_time;
        uint32_t last_release;
        EventType short_evt;
        EventType long_evt;

        // Порожній конструктор за замовчуванням (вимагається для створення масиву)
        ButtonTracker() = default;

        // Зручний конструктор для ініціалізації
        ButtonTracker(uint8_t p, EventType s_evt, EventType l_evt) 
            : pin(p), last_state(false), long_fired(false), 
              press_time(0), last_release(0), 
              short_evt(s_evt), long_evt(l_evt) {}
    };

    ButtonTracker btns[3];

public:
    InputService(AppState& st) : state(st) {
        // Тепер ініціалізація виглядає дуже чисто і C++ компілятор щасливий
        btns[0] = ButtonTracker(Config::PIN_TOUCH_LEFT,  EventType::BTN_LEFT_SHORT,  EventType::BTN_LEFT_LONG);
        btns[1] = ButtonTracker(Config::PIN_TOUCH_TOP,   EventType::BTN_TOP_SHORT,   EventType::BTN_TOP_LONG);
        btns[2] = ButtonTracker(Config::PIN_TOUCH_RIGHT, EventType::BTN_RIGHT_SHORT, EventType::BTN_RIGHT_LONG);
    }

    void init() override {
        for (auto& b : btns) {
            pinMode(b.pin, INPUT);
        }
    }

    void update() override {
        uint32_t now = millis();

        for (int i = 0; i < 3; i++) {
            bool cur = (digitalRead(btns[i].pin) == HIGH);

            if (i == 0) state.btn_l_pressed = cur;
            if (i == 1) state.btn_t_pressed = cur;
            if (i == 2) state.btn_r_pressed = cur;

            // Момент натискання
            if (cur && !btns[i].last_state && (now - btns[i].last_release > 150)) {
                btns[i].press_time = now;
                btns[i].long_fired = false;
            }
            // Утримання понад 900 мс -> Довгий клік
            else if (cur && btns[i].last_state && !btns[i].long_fired) {
                if (now - btns[i].press_time >= 900) {
                    btns[i].long_fired = true;
                    state.sendEvent(btns[i].long_evt);
                }
            }
            // Відпускання -> Короткий клік (якщо не спрацював довгий)
            else if (!cur && btns[i].last_state) {
                btns[i].last_release = now;
                if (!btns[i].long_fired && (now - btns[i].press_time >= 40)) {
                    state.sendEvent(btns[i].short_evt);
                }
            }

            btns[i].last_state = cur;
        }
    }
};