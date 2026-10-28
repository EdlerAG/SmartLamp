#include <Arduino.h>
#include "Config.h"
#include "core/Types.h"
#include "core/AppState.h"
#include "core/StorageService.h"
#include "core/CommandRouter.h"
#include "core/ServiceRegistry.h"
#include "hardware/InputService.h"
#include "hardware/SensorService.h"
#include "hardware/AudioService.h"
#include "led/LedService.h"
#include "network/NetworkService.h"
#include "cloud/TelegramService.h"
#include "cloud/OracleCloudService.h"
#include "cloud/StandaloneOta.h"
#include "ui/DisplayService.h"

AppState       appState;
StorageService storageService;

InputService       inputService(appState);
SensorService      sensorService(appState);
AudioService       audioService(appState);
LedService         ledService(appState);
DisplayService     displayService(appState);
NetworkService     networkService(appState);
TelegramService    telegramService(appState);
OracleCloudService oracleService(appState);

ServiceRegistry<4> core1FastServices;  // Core 1: Кнопки, LED-стрічка, Дисплей
ServiceRegistry<4> core0LocalServices; // Core 0 (Пріоритет 2): Датчики, Wi-Fi/Web, Аудіо I2S
ServiceRegistry<4> core0CloudServices; // Core 0 (Пріоритет 1): Telegram API та Oracle Cloud

void backgroundCore0Task(void* pvParameters) {
    core0LocalServices.initAll();

    for (;;) {
        core0LocalServices.tickAll();

        bool fast_net = appState.ota_in_progress || appState.ap_mode_active || audioService.isPlaying();
        vTaskDelay(pdMS_TO_TICKS(fast_net ? 1 : 12));
    }
}

void cloudCore0Task(void* pvParameters) {
    core0CloudServices.initAll();

    for (;;) {
        core0CloudServices.tickAll();
        storageService.checkDelayedSave(appState);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

void handleOnboardingButton(EventType ev_type) {
    if (ev_type == EventType::BTN_LEFT_SHORT) {
        // [ ◄ НІ / ПРОПУСТИТИ / НАЗАД ]
        switch (appState.onboarding_step) {
            case OnboardingStep::WIZ_STEP_WELCOME_SOUND:
                appState.audio_muted = true;
                appState.audio_volume = 0;
                storageService.saveAudioSettings(0, true, appState.touch_beep_enabled);
                appState.onboarding_step = appState.wifi_connected
                    ? (appState.sp_linked ? OnboardingStep::WIZ_STEP_DONE : OnboardingStep::WIZ_STEP_SPOTIFY_ASK)
                    : OnboardingStep::WIZ_STEP_WIFI_AP;
                break;

            case OnboardingStep::WIZ_STEP_WIFI_AP:
                // Пропустити підключення точки доступу
                appState.onboarding_step = appState.sp_linked ? OnboardingStep::WIZ_STEP_DONE : OnboardingStep::WIZ_STEP_SPOTIFY_ASK;
                break;

            case OnboardingStep::WIZ_STEP_WIFI_PORTAL:
                // Повернутися до QR-коду AP
                appState.onboarding_step = OnboardingStep::WIZ_STEP_WIFI_AP;
                break;

            case OnboardingStep::WIZ_STEP_SPOTIFY_ASK:
            case OnboardingStep::WIZ_STEP_SPOTIFY_QR:
                // Пропустити налаштування Spotify
                appState.onboarding_step = OnboardingStep::WIZ_STEP_DONE;
                break;

            case OnboardingStep::WIZ_STEP_DONE:
                appState.onboarded = true;
                storageService.setOnboarded(true);
                appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                appState.screen_mode = SCREEN_CLIMATE;
                break;
        }
        appState.ui_force_dirty = true;
        appState.registerActivity();
        return;
    }

    if (ev_type == EventType::BTN_RIGHT_SHORT) {
        // [ ТАК / ДАЛІ ► ]
        switch (appState.onboarding_step) {
            case OnboardingStep::WIZ_STEP_WELCOME_SOUND:
                appState.audio_muted = false;
                appState.audio_volume = 12;
                storageService.saveAudioSettings(12, false, appState.touch_beep_enabled);
                audioService.playBootChord();
                appState.onboarding_step = appState.wifi_connected
                    ? (appState.sp_linked ? OnboardingStep::WIZ_STEP_DONE : OnboardingStep::WIZ_STEP_SPOTIFY_ASK)
                    : OnboardingStep::WIZ_STEP_WIFI_AP;
                break;

            case OnboardingStep::WIZ_STEP_WIFI_AP:
                appState.onboarding_step = OnboardingStep::WIZ_STEP_WIFI_PORTAL;
                break;

            case OnboardingStep::WIZ_STEP_WIFI_PORTAL:
                appState.onboarding_step = appState.sp_linked ? OnboardingStep::WIZ_STEP_DONE : OnboardingStep::WIZ_STEP_SPOTIFY_ASK;
                break;

            case OnboardingStep::WIZ_STEP_SPOTIFY_ASK:
                appState.onboarding_step = OnboardingStep::WIZ_STEP_SPOTIFY_QR;
                break;

            case OnboardingStep::WIZ_STEP_SPOTIFY_QR:
                appState.onboarding_step = OnboardingStep::WIZ_STEP_DONE;
                break;

            case OnboardingStep::WIZ_STEP_DONE:
                appState.onboarded = true;
                storageService.setOnboarded(true);
                appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                appState.screen_mode = SCREEN_CLIMATE;
                break;
        }
        appState.ui_force_dirty = true;
        appState.registerActivity();
        return;
    }

    if (ev_type == EventType::BTN_TOP_SHORT) {
        if (appState.onboarding_step == OnboardingStep::WIZ_STEP_WELCOME_SOUND) {
            audioService.playBootChord();
        }
        appState.registerActivity();
    }
}

void processEvents() {
    AppEvent ev;
    static uint8_t next_meme_cycle = NOTIFY_CAT;

    while (xQueueReceive(appState.eventQueue, &ev, 0) == pdTRUE) {
        // Тактильний звуковий клік на будь-яке натискання сенсора (якщо увімкнено та не MUTE)
        audioService.playTouchClick();

        // 1. Якщо відкритий майстер Onboarding — кнопки мають специфічну навігацію
        if (appState.sys_ui_state == SystemUiState::SYS_ONBOARDING_WIZARD) {
            handleOnboardingButton(ev.type);
            continue;
        }

        switch (ev.type) {
            case EventType::BTN_LEFT_SHORT:
                if (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
                    appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                } else {
                    appState.screen_mode = (appState.screen_mode + 1) % SCREEN_COUNT;
                }
                appState.registerActivity();
                break;

            case EventType::BTN_LEFT_LONG:
                // Довге утримання лівої кнопки відкриває/закриває сервісний екран Web Studio та діагностики
                appState.sys_ui_state = (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG)
                                        ? SystemUiState::SYS_NORMAL
                                        : SystemUiState::SYS_NETWORK_DIAG;
                appState.registerActivity();
                break;

            case EventType::BTN_TOP_SHORT:
                if (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
                    appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                } else if (appState.anim_frames_count > 0) {
                    // Якщо зараз крутиться активна гіфка/наліпка — скидаємо показ за дотиком до кришки
                    appState.stopActiveMedia();
                } else if (appState.screen_mode == SCREEN_SMART_HUB && appState.tg_unread) {
                    // Дотик до кришки позначає вхідне повідомлення прочитаним
                    appState.tg_unread = false;
                    appState.cmd_send_tg_ack = true;
                    appState.triggerLedNotification(NOTIFY_FIRE, true);
                } else if (appState.screen_mode == SCREEN_SMART_HUB && appState.spotify.playing) {
                    // Play / Pause для Spotify
                    appState.cmd_spotify_action = 3;
                } else {
                    CommandRouter::nextLightMode(appState);
                }
                appState.registerActivity();
                break;

            case EventType::BTN_TOP_LONG:
                // Якщо світло ввімкнено — вимикаємо його; якщо вже вимкнено — тривале утримання вмикає/вимикає DND «Не турбувати»
                if (appState.light_mode != MODE_LIGHT_OFF) {
                    CommandRouter::togglePower(appState);
                } else {
                    appState.setDnd(!appState.dnd_mode);
                    audioService.playTouchClick();
                }
                break;

            case EventType::BTN_RIGHT_SHORT:
                if (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
                    appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                } else if (appState.screen_mode == SCREEN_LIVING_GIF) {
                    // Перемикання оверлею годинника поверх заставки
                    appState.saver_show_clock = !appState.saver_show_clock;
                    storageService.setSaverShowClock(appState.saver_show_clock);
                } else if (appState.screen_mode == SCREEN_SMART_HUB) {
                    if (appState.tg_unread && appState.inbox_count > 1) {
                        CommandRouter::nextInboxMessage(appState);
                    } else if (appState.spotify.playing) {
                        // Play / Pause Spotify за кліком правої кнопки на екрані плеєра
                        appState.cmd_spotify_action = 3;
                    } else if (appState.inbox_count > 1) {
                        CommandRouter::nextInboxMessage(appState);
                    }
                } else {
                    CommandRouter::cycleBrightness(appState);
                }
                appState.registerActivity();
                break;

            case EventType::BTN_RIGHT_LONG:
                if (appState.screen_mode == SCREEN_SMART_HUB && appState.spotify.playing) {
                    // Наступний трек у Spotify за довгим кліком
                    appState.cmd_spotify_action = 1;
                } else {
                    appState.cmd_send_meme_ping = next_meme_cycle;
                    appState.triggerLedNotification((NotifyLedType)next_meme_cycle, true);
                    audioService.playReactionSynth(next_meme_cycle);
                    next_meme_cycle++;
                    if (next_meme_cycle > NOTIFY_CROWN) next_meme_cycle = NOTIFY_FIRE;
                }
                break;

            default:
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Критичний фундамент безпеки: підтвердження валідності прошивки
    StandaloneOta::confirmBootValid();

    // 1. Ініціалізація сховища NVS та LittleFS
    storageService.initFs();
    storageService.loadUserSettings(appState);

    appState.initQueue();
    oracleService.setDisplayService(&displayService);
    oracleService.setStorageService(&storageService);

    // 2. Швидкі сервіси реального часу на Core 1
    core1FastServices.add(&inputService);
    core1FastServices.add(&ledService);
    core1FastServices.add(&displayService);
    core1FastServices.initAll();

    // 3. Завантаження заставки LittleFS у PSRAM одразу після старту
    displayService.loadScreensaverFromFs(storageService);

    // 4. Локальні сервіси та I2S-звук на Core 0
    core0LocalServices.add(&sensorService);
    core0LocalServices.add(&networkService);
    core0LocalServices.add(&audioService);

    // 5. Хмарні сервіси (Telegram + Oracle Cloud) на Core 0
    core0CloudServices.add(&telegramService);
    core0CloudServices.add(&oracleService);

    // Гармонійний процедурний акорд старту (якщо звук не вимкнено)
    audioService.playBootChord();

    xTaskCreatePinnedToCore(backgroundCore0Task, "Core0Net",   8192,  nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(cloudCore0Task,      "Core0Cloud", 10240, nullptr, 1, nullptr, 0);
}

void loop() {
    core1FastServices.tickAll();
    processEvents();

    bool active = ledService.isAnimating() || displayService.isHighFpsNeeded();
    vTaskDelay(pdMS_TO_TICKS(active ? 4 : 15));
}
