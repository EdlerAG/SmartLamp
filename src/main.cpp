#include <Arduino.h>
#include "Config.h"
#include "AppState.h"
#include "InputService.h"
#include "SensorService.h"
#include "LedService.h"
#include "NetworkService.h"
#include "DisplayService.h"

AppState appState;

InputService   inputService(appState);
SensorService  sensorService(appState);
LedService     ledService(appState);
NetworkService networkService(appState);
DisplayService displayService(appState);

// ==============================================================
// CORE 0: Фоновий потік мережі та I2C датчиків
// ==============================================================
void backgroundCore0Task(void* pvParameters) {
    networkService.init();

    for (;;) {
        sensorService.update();
        networkService.update();

        // 2 мс коли активний портал налаштування або OTA (миттєвий відгук сайту!),
        // 12 мс у звичайному фоновому режимі
        bool fast_net = appState.ota_in_progress || appState.ap_mode_active;
        vTaskDelay(pdMS_TO_TICKS(fast_net ? 2 : 12));
    }
}

// ==============================================================
// Диспетчер подій (Core 1)
// ==============================================================
void processEvents() {
    AppEvent ev;
    while (xQueueReceive(appState.eventQueue, &ev, 0) == pdTRUE) {
        switch (ev.type) {
            case EventType::BTN_LEFT_SHORT:
                appState.screen_mode = (appState.screen_mode + 1) % SCREEN_COUNT;
                break;

            case EventType::BTN_LEFT_LONG:
                appState.cmd_start_ap = true;
                appState.led_notify_trigger = true;
                break;

            case EventType::BTN_TOP_SHORT:
                if (appState.screen_mode == SCREEN_TELEGRAM && appState.tg_unread) {
                    appState.tg_unread = false;
                } else {
                    appState.light_mode = (appState.light_mode + 1) % AppState::LIGHT_MODE_COUNT;
                    appState.markSettingsDirty();
                }
                break;

            case EventType::BTN_TOP_LONG:
                appState.light_mode = (appState.light_mode == 5) ? 0 : 5;
                appState.markSettingsDirty();
                break;

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

            case EventType::BTN_RIGHT_LONG:
                appState.tg_unread = true;
                appState.screen_mode = SCREEN_TELEGRAM;
                appState.led_notify_trigger = true;
                break;

            case EventType::WIFI_CONNECTED:
                // WiFi підключився → переходимо на головний екран (мережа вже оновлена)
                appState.screen_mode = SCREEN_CLIMATE;
                appState.ui_force_dirty = true;
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
        "Core0Services",
        8192,
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

    // Енергоощадний сон ядра Core 1:
    // 4 мс коли активна анімація/UI (250 Гц опитування кнопок), 15 мс у повному спокої
    bool active = ledService.isAnimating() || displayService.isHighFpsNeeded();
    vTaskDelay(pdMS_TO_TICKS(active ? 4 : 15));
}