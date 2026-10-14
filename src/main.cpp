#include <Arduino.h>
#include "Config.h"
#include "AppState.h"
#include "CommandRouter.h"
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

void backgroundCore0Task(void* pvParameters) {
    networkService.init();

    for (;;) {
        sensorService.update();
        networkService.update();

        bool fast_net = appState.ota_in_progress || appState.ap_mode_active;
        vTaskDelay(pdMS_TO_TICKS(fast_net ? 2 : 12));
    }
}

void telegramCore0Task(void* pvParameters) {
    telegramService.init();

    for (;;) {
        telegramService.update();
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

void processEvents() {
    AppEvent ev;
    static uint8_t next_meme_cycle = NOTIFY_CAT;

    while (xQueueReceive(appState.eventQueue, &ev, 0) == pdTRUE) {
        switch (ev.type) {
            case EventType::BTN_LEFT_SHORT:
                appState.screen_mode = (appState.screen_mode + 1) % SCREEN_COUNT;
                break;

            case EventType::BTN_LEFT_LONG:
                appState.cmd_start_ap = true;
                appState.triggerLedNotification(NOTIFY_CYAN_WAVE, false);
                break;

            case EventType::BTN_TOP_SHORT:
                if (appState.screen_mode == SCREEN_TELEGRAM && appState.tg_unread) {
                    appState.tg_unread = false;
                    appState.cmd_send_tg_ack = true;
                    appState.triggerLedNotification(NOTIFY_FIRE, true);
                } else {
                    CommandRouter::nextLightMode(appState);
                }
                break;

            case EventType::BTN_TOP_LONG:
                CommandRouter::togglePower(appState);
                break;

            // Права (коротко): На екрані повідомлень гортає історію Inbox (1/5..5/5), на інших — змінює яскравість
            case EventType::BTN_RIGHT_SHORT:
                if (appState.screen_mode == SCREEN_TELEGRAM && appState.inbox_count > 1) {
                    CommandRouter::nextInboxMessage(appState);
                } else {
                    CommandRouter::cycleBrightness(appState);
                }
                break;

            // Права (утримання 1 с): Запускає по колу всі 5 мем-реакцій (Кіт -> НЛО -> Корона -> Вогник -> Блискавка)
            case EventType::BTN_RIGHT_LONG:
                appState.cmd_send_meme_ping = next_meme_cycle;
                appState.triggerLedNotification((NotifyLedType)next_meme_cycle, true);
                next_meme_cycle++;
                if (next_meme_cycle > NOTIFY_CROWN) next_meme_cycle = NOTIFY_FIRE;
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