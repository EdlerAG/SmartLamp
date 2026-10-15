#include <Arduino.h>
#include "Config.h"
#include "core/Types.h"
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "core/ServiceRegistry.h"
#include "hardware/InputService.h"
#include "hardware/SensorService.h"
#include "led/LedService.h"
#include "network/NetworkService.h"
#include "cloud/TelegramService.h"
#include "cloud/OracleCloudService.h"
#include "ui/DisplayService.h"

AppState appState;

InputService       inputService(appState);
SensorService      sensorService(appState);
LedService         ledService(appState);
DisplayService     displayService(appState);
NetworkService     networkService(appState);
TelegramService    telegramService(appState);
OracleCloudService oracleService(appState);

ServiceRegistry<4> core1FastServices;  // Core 1: Кнопки, LED-стрічка, Дисплей
ServiceRegistry<4> core0LocalServices; // Core 0 (Пріоритет 2): I2C датчики, Wi-Fi, WebServer
ServiceRegistry<4> core0CloudServices; // Core 0 (Пріоритет 1): Telegram API та Oracle Cloud

void backgroundCore0Task(void* pvParameters) {
    core0LocalServices.initAll();

    for (;;) {
        core0LocalServices.tickAll();
        bool fast_net = appState.ota_in_progress || appState.ap_mode_active;
        vTaskDelay(pdMS_TO_TICKS(fast_net ? 2 : 12));
    }
}

void cloudCore0Task(void* pvParameters) {
    core0CloudServices.initAll();

    for (;;) {
        core0CloudServices.tickAll();
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

            case EventType::BTN_RIGHT_SHORT:
                if (appState.screen_mode == SCREEN_TELEGRAM && appState.inbox_count > 1) {
                    CommandRouter::nextInboxMessage(appState);
                } else {
                    CommandRouter::cycleBrightness(appState);
                }
                break;

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

    core1FastServices.add(&inputService);
    core1FastServices.add(&ledService);
    core1FastServices.add(&displayService);
    core1FastServices.initAll();

    core0LocalServices.add(&sensorService);
    core0LocalServices.add(&networkService);

    core0CloudServices.add(&telegramService);
    core0CloudServices.add(&oracleService);

    xTaskCreatePinnedToCore(backgroundCore0Task, "Core0Net",   8192,  nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(cloudCore0Task,      "Core0Cloud", 10240, nullptr, 1, nullptr, 0);
}

void loop() {
    core1FastServices.tickAll();
    processEvents();

    bool active = ledService.isAnimating() || displayService.isHighFpsNeeded();
    vTaskDelay(pdMS_TO_TICKS(active ? 4 : 15));
}