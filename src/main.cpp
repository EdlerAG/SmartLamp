#include <Arduino.h>
#include "Config.h"
#include "AppState.h"
#include "InputService.h"
#include "SensorService.h"
#include "LedService.h"
#include "NetworkService.h"
#include "TelegramService.h"
#include "DisplayService.h"

AppState appState;

InputService    inputService(appState);
SensorService   sensorService(appState);
LedService      ledService(appState);
NetworkService  networkService(appState);
TelegramService telegramService(appState);
DisplayService  displayService(appState);

// ==============================================================
// CORE 0 (Потік 1): Локальна мережа, Blackout Eco та I2C датчики
// ==============================================================
void backgroundCore0Task(void* pvParameters) {
    networkService.init();

    for (;;) {
        sensorService.update();
        networkService.update();

        bool fast_net = appState.ota_in_progress || appState.ap_mode_active;
        vTaskDelay(pdMS_TO_TICKS(fast_net ? 2 : 12));
    }
}

// ==============================================================
// CORE 0 (Потік 2): Ізольований потік Telegram API (Dual-Role)
// ==============================================================
void telegramCore0Task(void* pvParameters) {
    telegramService.init();

    for (;;) {
        telegramService.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==============================================================
// Диспетчер подій (Core 1)
// ==============================================================
void processEvents() {
    AppEvent ev;
    while (xQueueReceive(appState.eventQueue, &ev, 0) == pdTRUE) {
        switch (ev.type) {
            // Ліва (коротко): Гортання екранів
            case EventType::BTN_LEFT_SHORT:
                appState.screen_mode = (appState.screen_mode + 1) % SCREEN_COUNT;
                break;

            // Ліва (утримання 1 с): Примусовий запуск точки доступу з QR-кодом
            case EventType::BTN_LEFT_LONG:
                appState.cmd_start_ap = true;
                appState.triggerLedNotification(NOTIFY_CYAN_WAVE);
                break;

            // Кришка (коротко): Прочитати повідомлення (і відповісти Ярославу) або змінити світло
            case EventType::BTN_TOP_SHORT:
                if (appState.screen_mode == SCREEN_TELEGRAM && appState.tg_unread) {
                    appState.tg_unread = false;
                    appState.cmd_send_tg_ack = true;
                    appState.triggerLedNotification(NOTIFY_LOVE_PULSE);
                } else {
                    appState.light_mode = (appState.light_mode + 1) % AppState::LIGHT_MODE_COUNT;
                    appState.markSettingsDirty();
                }
                break;

            // Кришка (утримання 1 с): Швидке вимкнення/увімкнення підсвітки
            case EventType::BTN_TOP_LONG:
                appState.light_mode = (appState.light_mode == 5) ? 0 : 5;
                appState.markSettingsDirty();
                break;

            // Права (коротко): Режим яскравості AUTO / Ручний
            case EventType::BTN_RIGHT_SHORT:
                if (appState.auto_brt) {
                    appState.auto_brt = false;
                    appState.manual_brt_idx = 0;
                } else {
                    appState.manual_brt_idx++;
                    if (appState.manual_brt_idx >= 3) appState.auto_brt = true;
                }
                appState.markSettingsDirty();
                break;

            // Права (утримання 1 с): Надіслати "Сердечко" зі шкатулки Ярославу в Telegram!
            case EventType::BTN_RIGHT_LONG:
                appState.cmd_send_love_ping = true;
                appState.triggerLedNotification(NOTIFY_LOVE_PULSE);
                break;

            default:
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    appState.initQueue();

    inputService.init();
    sensorService.init();
    ledService.init();
    displayService.init();

    xTaskCreatePinnedToCore(
        backgroundCore0Task,
        "Core0Net",
        8192,
        nullptr,
        2,
        nullptr,
        0
    );

    xTaskCreatePinnedToCore(
        telegramCore0Task,
        "Core0Telegram",
        10240,
        nullptr,
        1,
        nullptr,
        0
    );
}

void loop() {
    inputService.update();
    processEvents();
    ledService.update();
    displayService.update();

    bool active = ledService.isAnimating() || displayService.isHighFpsNeeded();
    vTaskDelay(pdMS_TO_TICKS(active ? 4 : 15));
}