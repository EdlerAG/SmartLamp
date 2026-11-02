#include <Arduino.h>
#include <esp_task_wdt.h>
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
#include "cloud/OracleCloudService.h"
#include "cloud/StandaloneOta.h"
#include "core/BootSafetyGuard.h"
#include "ui/DisplayService.h"

AppState       appState;
StorageService storageService;

InputService       inputService(appState);
SensorService      sensorService(appState);
AudioService       audioService(appState);
LedService         ledService(appState);
DisplayService     displayService(appState);
NetworkService     networkService(appState, storageService);
OracleCloudService oracleService(appState);

ServiceRegistry<4> core1FastServices;  // Core 1: Кнопки, LED-стрічка, Дисплей
ServiceRegistry<4> core0LocalServices; // Core 0 (Пріоритет 2): Датчики, Wi-Fi/Web, Аудіо I2S
ServiceRegistry<2> core0CloudServices; // Core 0 (Пріоритет 1): Oracle Cloud Gateway

static TaskHandle_t s_netTaskHandle   = nullptr;
static TaskHandle_t s_cloudTaskHandle = nullptr;

void backgroundCore0Task(void* pvParameters) {
    esp_task_wdt_add(NULL);
    core0LocalServices.initAll();

    for (;;) {
        esp_task_wdt_reset();
        core0LocalServices.tickAll();

        bool fast_net = appState.ota_in_progress || appState.ap_mode_active || audioService.isPlaying();
        vTaskDelay(pdMS_TO_TICKS(fast_net ? 1 : 12));
    }
}

void cloudCore0Task(void* pvParameters) {
    esp_task_wdt_add(NULL);
    core0CloudServices.initAll();

    for (;;) {
        esp_task_wdt_reset();
        core0CloudServices.tickAll();
        vTaskDelay(pdMS_TO_TICKS(15));
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
                appState.onboarding_step = OnboardingStep::WIZ_STEP_TG_BOT;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_TG_BOT);
                break;

            case OnboardingStep::WIZ_STEP_TG_BOT:
                // Повернутися до вибору звуку
                appState.onboarding_step = OnboardingStep::WIZ_STEP_WELCOME_SOUND;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_WELCOME_SOUND);
                break;

            case OnboardingStep::WIZ_STEP_WIFI_AP:
                // Повернутися до QR-коду бота
                appState.onboarding_step = OnboardingStep::WIZ_STEP_TG_BOT;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_TG_BOT);
                break;

            case OnboardingStep::WIZ_STEP_CONNECTING:
                // Повернутися до вибору Wi-Fi
                appState.onboarding_step = OnboardingStep::WIZ_STEP_WIFI_AP;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_WIFI_AP);
                break;

            case OnboardingStep::WIZ_STEP_WAITING_CHECKLIST:
                // Пропустити очікування налаштування в боті -> перехід до завершення
                appState.onboarding_step = OnboardingStep::WIZ_STEP_DONE;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_DONE);
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
                appState.onboarding_step = OnboardingStep::WIZ_STEP_TG_BOT;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_TG_BOT);
                break;

            case OnboardingStep::WIZ_STEP_TG_BOT:
                // Перехід до підключення Wi-Fi
                appState.onboarding_step = appState.wifi_connected
                    ? OnboardingStep::WIZ_STEP_CONNECTING
                    : OnboardingStep::WIZ_STEP_WIFI_AP;
                storageService.setOnboardingStep((uint8_t)appState.onboarding_step);
                break;

            case OnboardingStep::WIZ_STEP_WIFI_AP:
                if (appState.wifi_connected) {
                    appState.onboarding_step = OnboardingStep::WIZ_STEP_CONNECTING;
                    storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_CONNECTING);
                }
                break;

            case OnboardingStep::WIZ_STEP_CONNECTING:
                if (appState.oracle_online) {
                    appState.onboarding_step = OnboardingStep::WIZ_STEP_WAITING_CHECKLIST;
                    storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_WAITING_CHECKLIST);
                }
                break;

            case OnboardingStep::WIZ_STEP_WAITING_CHECKLIST:
                appState.onboarding_step = OnboardingStep::WIZ_STEP_DONE;
                storageService.setOnboardingStep((uint8_t)OnboardingStep::WIZ_STEP_DONE);
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

        // 0. Якщо активне вікно святкування Дня Народження — будь-який клік закриває оверлей
        if (appState.sys_ui_state == SystemUiState::SYS_BIRTHDAY_CELEBRATE) {
            appState.sys_ui_state = SystemUiState::SYS_NORMAL;
            appState.birthday_celebrate_active = false;
            appState.registerActivity();
            continue;
        }

        // 0.1. Якщо активний ранковий будильник / схід сонця — дотик до БУДЬ-ЯКОЇ кнопки вимикає його
        if (appState.isAlarmActive(millis())) {
            appState.alarm_active = false;
            appState.alarm_dismissed = true;
            appState.alarm_active_until_ms = 0;
            appState.stopActiveMedia();
            CommandRouter::setLightMode(appState, MODE_WARM_COZY);
            CommandRouter::setBrightnessPreset(appState, false, 2); // 65% комфортне світло
            appState.registerActivity();
            continue;
        }

        // 1. Якщо відкритий майстер Onboarding — кнопки мають специфічну навігацію
        if (appState.sys_ui_state == SystemUiState::SYS_ONBOARDING_WIZARD) {
            handleOnboardingButton(ev.type);
            continue;
        }

        switch (ev.type) {
            case EventType::BTN_LEFT_SHORT:
                if (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
                    appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                } else if (appState.screen_mode == SCREEN_POMODORO) {
                    // На екрані Pomodoro ліва кнопка зменшує час (-5 хв у режимі налаштування, -1 хв під час бігу)
                    appState.pomoAdjustMinutes(appState.pomo_active ? -1 : -5);
                } else {
                    appState.screen_mode = (appState.screen_mode + 1) % SCREEN_COUNT;
                }
                appState.registerActivity();
                break;

            case EventType::BTN_LEFT_LONG:
                if (appState.screen_mode == SCREEN_SMART_HUB) {
                    // На екрані Spotify/Smart Hub перемикаємо режим світіння між AmbiLight та Затишним теплим
                    if (appState.light_mode == MODE_SPOTIFY_AMBI) {
                        CommandRouter::setLightMode(appState, MODE_WARM_COZY);
                        appState.showHudToast("Light", "Warm Cozy");
                    } else {
                        CommandRouter::setLightMode(appState, MODE_SPOTIFY_AMBI);
                        appState.showHudToast("Light", "Spotify Ambi");
                    }
                } else if (appState.screen_mode == SCREEN_POMODORO) {
                    // З екрана Pomodoro довгий клік повертає на головний годинник
                    appState.screen_mode = SCREEN_CLIMATE;
                } else {
                    // З будь-якого іншого екрана довге утримання лівої кнопки перемикає на Pomodoro
                    appState.screen_mode = SCREEN_POMODORO;
                }
                appState.registerActivity();
                break;

            case EventType::BTN_COMBO_WIFI_RESET:
                // Блок 4: 10-секундне утримання Left + Right -> аварійний скид та запуск точки доступу
                appState.sys_ui_state = SystemUiState::SYS_NETWORK_DIAG;
                appState.cmd_start_ap = true;
                audioService.playBootChord();
                appState.triggerLedNotification(NOTIFY_CYAN_WAVE, true);
                appState.showHudToast("Emergency AP", "Setup Mode Active");
                appState.registerActivity();
                break;

            case EventType::BTN_TOP_SHORT:
                if (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
                    // Натискання на кришку на екрані діагностики вмикає точку доступу MagicBox-Setup
                    appState.cmd_start_ap = true;
                    audioService.playBootChord();
                    appState.triggerLedNotification(NOTIFY_CYAN_WAVE, true);
                    return;
                } else if (appState.anim_frames_count > 0) {
                    // Якщо зараз крутиться активна гіфка/наліпка — скидаємо показ за дотиком до кришки
                    appState.stopActiveMedia();
                } else if (appState.screen_mode == SCREEN_POMODORO) {
                    // Дотик до сенсорної кришки вмикає Старт / Паузу Pomodoro
                    appState.pomoToggleStartPause();
                } else if (appState.screen_mode == SCREEN_SMART_HUB && appState.tg_unread) {
                    // Дотик до кришки позначає вхідне повідомлення прочитаним
                    appState.tg_unread = false;
                    appState.cmd_send_tg_ack = true;
                    appState.triggerLedNotification(NOTIFY_FIRE, true);
                } else if (appState.screen_mode == SCREEN_SMART_HUB && (appState.spotify.playing || appState.spotify.track_id[0] != '\0')) {
                    // Play / Pause для Spotify за дотиком до сенсорної кришки
                    appState.cmd_spotify_action = 3;
                } else {
                    CommandRouter::nextLightMode(appState);
                }
                appState.registerActivity();
                break;

            case EventType::BTN_TOP_LONG:
                if (appState.screen_mode == SCREEN_POMODORO) {
                    // Тривалий дотик до кришки скидає таймер Pomodoro
                    appState.pomoStop();
                } else if (appState.light_mode != MODE_LIGHT_OFF) {
                    // Якщо світло ввімкнено — вимикаємо його; якщо вже вимкнено — тривале утримання вмикає/вимикає DND «Не турбувати»
                    CommandRouter::togglePower(appState);
                } else {
                    appState.setDnd(!appState.dnd_mode);
                    audioService.playTouchClick();
                }
                break;

            case EventType::BTN_RIGHT_SHORT:
                if (appState.sys_ui_state == SystemUiState::SYS_NETWORK_DIAG) {
                    appState.sys_ui_state = SystemUiState::SYS_NORMAL;
                } else if (appState.screen_mode == SCREEN_POMODORO) {
                    // На екрані Pomodoro права кнопка збільшує час (+5 хв у режимі налаштування, +1 хв під час бігу)
                    appState.pomoAdjustMinutes(appState.pomo_active ? 1 : 5);
                } else if (appState.screen_mode == SCREEN_LIVING_GIF) {
                    // Перемикання оверлею годинника поверх заставки
                    appState.saver_show_clock = !appState.saver_show_clock;
                    storageService.setSaverShowClock(appState.saver_show_clock);
                } else if (appState.screen_mode == SCREEN_SMART_HUB) {
                    if (appState.tg_unread && appState.inbox_count > 1) {
                        CommandRouter::nextInboxMessage(appState);
                    } else if (appState.spotify.playing || appState.spotify.track_id[0] != '\0') {
                        // Наступний трек Spotify за коротким кліком правої кнопки
                        appState.cmd_spotify_action = 1;
                    } else if (appState.inbox_count > 1) {
                        CommandRouter::nextInboxMessage(appState);
                    }
                } else {
                    CommandRouter::cycleBrightness(appState);
                }
                appState.registerActivity();
                break;

            case EventType::BTN_RIGHT_LONG:
                if (appState.screen_mode == SCREEN_POMODORO) {
                    // Довгий клік правої кнопки скидає Pomodoro
                    appState.pomoStop();
                } else if (appState.screen_mode == SCREEN_SMART_HUB && (appState.spotify.playing || appState.spotify.track_id[0] != '\0')) {
                    // Попередній трек Spotify за довгим кліком правої кнопки
                    appState.cmd_spotify_action = 2;
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

static TaskHandle_t s_ledTaskHandle = nullptr;

static void ledCore1Task(void* pvParameters) {
    esp_task_wdt_add(NULL);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(16); // 60 FPS (16.6ms)

    while (true) {
        esp_task_wdt_reset();
        ledService.update();
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

void setup() {
    Serial.begin(115200);

    // 🛡️ Критичний захист від збоїв (Autonomous Boot Safety & Rollback Guard)
    BootSafetyGuard::onEarlyBoot();

    // Діагностика причини перезапуску
    esp_reset_reason_t rst = esp_reset_reason();
    switch (rst) {
        case ESP_RST_POWERON:   strncpy(appState.reset_reason, "POWERON", sizeof(appState.reset_reason)); break;
        case ESP_RST_SW:        strncpy(appState.reset_reason, "SOFTWARE", sizeof(appState.reset_reason)); break;
        case ESP_RST_PANIC:     strncpy(appState.reset_reason, "PANIC", sizeof(appState.reset_reason)); break;
        case ESP_RST_INT_WDT:   strncpy(appState.reset_reason, "INT_WDT", sizeof(appState.reset_reason)); break;
        case ESP_RST_TASK_WDT:  strncpy(appState.reset_reason, "TASK_WDT", sizeof(appState.reset_reason)); break;
        case ESP_RST_BROWNOUT:  strncpy(appState.reset_reason, "BROWNOUT", sizeof(appState.reset_reason)); break;
        case ESP_RST_DEEPSLEEP: strncpy(appState.reset_reason, "DEEPSLEEP", sizeof(appState.reset_reason)); break;
        default:                strncpy(appState.reset_reason, "OTHER", sizeof(appState.reset_reason)); break;
    }

    // Ініціалізація Task Watchdog Timer (TWDT) на 20 секунд для захисту від важких TLS-хендшейків
    esp_task_wdt_init(20, true);
    esp_task_wdt_add(NULL);

    // Якщо активовано аварійний режим порятунку (наприклад, краш-петля без можливості відкату)
    if (BootSafetyGuard::isFailsafeMode()) {
        Serial.println("[Setup] ⚠️ BootSafetyGuard ACTIVE: Booting into Rescue Network Mode only!");
        storageService.initFs();
        storageService.loadUserSettings(appState);
        appState.initQueue();
        oracleService.setStorageService(&storageService);

        core0LocalServices.add(&networkService);
        core0CloudServices.add(&oracleService);
        xTaskCreatePinnedToCore(backgroundCore0Task, "Core0Net",   8192,  nullptr, 2, nullptr, 0);
        xTaskCreatePinnedToCore(cloudCore0Task,      "Core0Cloud", 10240, nullptr, 1, nullptr, 0);
        return;
    }

    // 1. Ініціалізація сховища NVS та LittleFS
    storageService.initFs();
    storageService.loadUserSettings(appState);

    appState.initQueue();
    oracleService.setDisplayService(&displayService);
    oracleService.setStorageService(&storageService);

    // 2. Швидкі сервіси реального часу на Core 1
    // 2. Ініціалізація швидких сервісів Core 1 (Дисплей та кнопки)
    core1FastServices.add(&inputService);
    core1FastServices.add(&displayService);
    core1FastServices.initAll();

    // 2.1 Ізольований апаратний RMT рушій підсвітки (Core 1, Priority 4, 60 FPS)
    ledService.init();
    xTaskCreatePinnedToCore(
        ledCore1Task,
        "LedTask",
        4096,
        nullptr,
        4, // Priority 4 (вище за loop())
        &s_ledTaskHandle,
        1  // Core 1
    );

    // 3. Завантаження заставки LittleFS у PSRAM одразу після старту
    displayService.loadScreensaverFromFs(storageService);

    // 4. Локальні сервіси та I2S-звук на Core 0
    core0LocalServices.add(&sensorService);
    core0LocalServices.add(&networkService);
    core0LocalServices.add(&audioService);

    // 5. Хмарні сервіси (Oracle Cloud Gateway) на Core 0
    core0CloudServices.add(&oracleService);

    // Гармонійний процедурний акорд старту (якщо звук не вимкнено)
    audioService.playBootChord();

    xTaskCreatePinnedToCore(backgroundCore0Task, "Core0Net",   8192,  nullptr, 2, &s_netTaskHandle,   0);
    xTaskCreatePinnedToCore(cloudCore0Task,      "Core0Cloud", 10240, nullptr, 1, &s_cloudTaskHandle, 0);
}

void loop() {
    esp_task_wdt_reset();

    static uint32_t last_stk_check_ms = 0;
    uint32_t now = millis();
    if (now - last_stk_check_ms >= 2000) {
        last_stk_check_ms = now;
        if (s_netTaskHandle)   appState.stk_net   = (uint32_t)uxTaskGetStackHighWaterMark(s_netTaskHandle);
        if (s_cloudTaskHandle) appState.stk_cloud = (uint32_t)uxTaskGetStackHighWaterMark(s_cloudTaskHandle);
        if (s_ledTaskHandle)   appState.stk_led   = (uint32_t)uxTaskGetStackHighWaterMark(s_ledTaskHandle);
    }
    core1FastServices.tickAll();
    processEvents();

    // 🛡️ Перевірка стабільності системи (знімає захист відкату після виходу в хмару або 25с аптайму)
    BootSafetyGuard::tick(appState.oracle_online);

    if (appState.cmd_reboot) {
        delay(150);
        ESP.restart();
    }

    storageService.checkDelayedSave(appState);

    // Рівень 2: Архітектурний частотний регулятор CPU (Governor з гістерезисом)
    static uint32_t last_freq_change_ms = 0;
    static uint32_t high_demand_last_seen_ms = 0;
    uint32_t loop_now = millis();

    bool high_freq_needed = appState.ota_in_progress || 
                            appState.ap_mode_active ||
                            appState.tls_in_flight || 
                            appState.spotify_vinyl_needed ||
                            appState.spotify.playing ||
                            audioService.isPlaying() || 
                            ledService.isAnimating() ||
                            displayService.isHighFpsNeeded() ||
                            (loop_now - appState.last_user_activity_ms < 15000UL) ||
                            (loop_now - appState.last_net_activity_ms < 3000UL);

    if (high_freq_needed) {
        high_demand_last_seen_ms = loop_now;
    }

    // Рівень 3: Апаратний термозахист кремнію у закритому корпусі (Silicon Thermal Throttle)
    static uint32_t last_temp_check_ms = 0;
    static bool thermal_throttled = false;
    if (loop_now - last_temp_check_ms >= 4000UL) {
        last_temp_check_ms = loop_now;
        float die_temp = temperatureRead();
        if (die_temp > 74.0f) {
            thermal_throttled = true;
        } else if (die_temp < 65.0f) {
            thermal_throttled = false;
        }
    }

    uint32_t current_hw_mhz = getCpuFrequencyMhz();

    // Забороняємо зміну частоти ядра, якщо в LedService триває активна передача RMT-кадру,
    // щоб уникнути збою тактування шини APB та спотворення таймінгів SK6812
    if (!ledService.isTransmitting()) {
        if (thermal_throttled) {
            // При перегріві в закритому корпусі обмежуємо частоту до 160 МГц
            if (current_hw_mhz > 160 && (loop_now - last_freq_change_ms >= 1000UL)) {
                setCpuFrequencyMhz(160);
                last_freq_change_ms = loop_now;
            }
        } else if (high_freq_needed) {
            // Миттєвий підйом до 240 МГц без затримок для плавності UI та звуку
            if (current_hw_mhz < 240 && (loop_now - last_freq_change_ms >= 500UL)) {
                setCpuFrequencyMhz(240);
                last_freq_change_ms = loop_now;
            }
        } else {
            // Зниження до 80 МГц лише при стійкому спокої (>= 15 с безперервного спокою)
            if (current_hw_mhz > 80 && 
                (loop_now - high_demand_last_seen_ms >= 15000UL) && 
                (loop_now - last_freq_change_ms >= 3000UL)) {
                setCpuFrequencyMhz(80);
                last_freq_change_ms = loop_now;
            }
        }
    }

    bool active = ledService.isAnimating() || displayService.isHighFpsNeeded();
    vTaskDelay(pdMS_TO_TICKS(active ? 4 : 15));
}
