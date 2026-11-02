#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include "Config.h"
#include "cloud/StandaloneOta.h"

/**
 * ============================================================================
 * BootSafetyGuard — Автономний захист від очорніння (Anti-Brick System v5.0)
 * 
 * 1. Відслідковує поспіль краші через NVS-лічильник (Boot Streak).
 * 2. Апаратне тестування прошивки (Probation Window 75с): після OTA-оновлення
 *    нова прошивка зобов'язана встановити зв'язок із сервером Oracle Cloud.
 *    Якщо зв'язку немає протягом 75 секунд — автоматичний апаратний відкат!
 * 3. Crash loop detection (streak >= 3): апаратний відкат на попередню прошивку.
 * 4. Hardware Button Rescue: утримання сенсорної клавіші при подачі живлення
 *    примусово піднімає Emergency AP Hotspot (192.168.4.1) без використання ПК.
 * 5. Failsafe Rescue Mode: якщо відкат неможливий, запуск ізольованого Web/OTA.
 * ============================================================================
 */
class BootSafetyGuard {
private:
    static bool& isConfirmed() {
        static bool s_confirmed = false;
        return s_confirmed;
    }

    static uint32_t& getStreak() {
        static uint32_t s_streak = 0;
        return s_streak;
    }

    static uint32_t& getBootMs() {
        static uint32_t s_ms = 0;
        return s_ms;
    }

    static bool& getFailsafe() {
        static bool s_failsafe = false;
        return s_failsafe;
    }

    static bool& getForceAp() {
        static bool s_force_ap = false;
        return s_force_ap;
    }

    static bool& isUnderProbation() {
        static bool s_probation = false;
        return s_probation;
    }

public:
    static void onEarlyBoot() {
        getBootMs() = millis();

        // 1. Апаратна перевірка затискання кнопки при старті (Zero-Wire Recovery)
        // Використовуємо внутрішній PULLDOWN та багаторазове семплування протягом 1500 мс,
        // щоб повністю виключити хибні спрацьовування через перехідні процеси живлення.
        pinMode(Config::PIN_TOUCH_LEFT, INPUT_PULLDOWN);
        if (digitalRead(Config::PIN_TOUCH_LEFT) == HIGH) {
            bool sustained = true;
            for (int i = 0; i < 15; i++) {
                delay(100);
                esp_task_wdt_reset();
                if (digitalRead(Config::PIN_TOUCH_LEFT) != HIGH) {
                    sustained = false;
                    break;
                }
            }
            if (sustained) {
                getForceAp() = true;
                Serial.println("[BootSafetyGuard] 🛠️ Hardware LEFT touch held for 1.5s at boot -> Force Emergency AP mode!");
            }
        }

        // 2. Перевірка статусу випробувального терміну OTA (ESP_OTA_IMG_PENDING_VERIFY)
        const esp_partition_t* running = esp_ota_get_running_partition();
        if (running) {
            esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
            if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
                if (ota_state == ESP_OTA_IMG_NEW || ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                    isUnderProbation() = true;
                    Serial.printf("[BootSafetyGuard] ⏳ OTA PROBATION ACTIVE: Running on %s (0x%08X). Awaiting cloud verification (max 75s)...\n",
                                  running->label, running->address);
                }
            }
        }

        // 3. Лічильник краш-серій в NVS
        Preferences prefs;
        if (prefs.begin("boot_safety", false)) {
            getStreak() = prefs.getUInt("streak", 0) + 1;
            prefs.putUInt("streak", getStreak());
            prefs.end();
        }

        Serial.printf("[BootSafetyGuard] 🛡️ Boot sequence started. Streak count: %u\n", getStreak());

        // 4. Захист від зациклення перезавантажень (streak >= 3)
        if (getStreak() >= 3) {
            Serial.println("[BootSafetyGuard] ⚠️⚠️⚠️ CRITICAL: Crash loop detected (streak >= 3)!");

            if (Update.canRollBack()) {
                Serial.println("[BootSafetyGuard] 🔄 Executing autonomous hardware rollback to previous known-good firmware...");
                if (prefs.begin("boot_safety", false)) {
                    prefs.putUInt("streak", 0);
                    prefs.end();
                }
                delay(150);
                esp_err_t rb_err = esp_ota_mark_app_invalid_rollback_and_reboot();
                if (rb_err != ESP_OK) {
                    StandaloneOta::rollback();
                    ESP.restart();
                }
            } else {
                Serial.println("[BootSafetyGuard] ⚠️ Rollback partition unavailable. Entering FAILSAFE RESCUE MODE.");
                getFailsafe() = true;
            }
        }
    }

    static bool isFailsafeMode() {
        return getFailsafe();
    }

    static bool isForceApRecovery() {
        return getForceAp();
    }

    static void tick(bool is_cloud_online) {
        if (isConfirmed()) return;

        uint32_t uptime = millis() - getBootMs();

        // 🌟 СЦЕНАРІЙ А: Прошивка під випробувальним терміном після OTA
        if (isUnderProbation()) {
            if (is_cloud_online) {
                // Хмара відповіла! Прошивка 100% робоча
                Serial.println("[BootSafetyGuard] 🏆 OTA PROBATION PASSED: Cloud handshake confirmed!");
                markBootSuccessful();
            } else if (uptime > 75000) {
                // За 75 секунд нова прошивка так і не змогла зв'язатися з хмарою
                Serial.println("[BootSafetyGuard] ❌ OTA PROBATION FAILED: No cloud connection within 75s!");
                Serial.println("[BootSafetyGuard] 🔄 Auto-reverting to previous working firmware...");
                Preferences prefs;
                if (prefs.begin("boot_safety", false)) {
                    prefs.putUInt("streak", 0);
                    prefs.end();
                }
                delay(200);
                esp_err_t rb_err = esp_ota_mark_app_invalid_rollback_and_reboot();
                if (rb_err != ESP_OK) {
                    StandaloneOta::rollback();
                    ESP.restart();
                }
            }
            return;
        }

        // 🌟 СЦЕНАРІЙ Б: Стандартне завантаження вже підтвердженої прошивки
        // Знімаємо лічильник крашів після 25 секунд стабільної роботи або зв'язку з хмарою
        if (is_cloud_online || uptime > 25000) {
            markBootSuccessful();
        }
    }

    static void markBootSuccessful() {
        if (isConfirmed()) return;
        isConfirmed() = true;

        Preferences prefs;
        if (prefs.begin("boot_safety", false)) {
            prefs.putUInt("streak", 0);
            prefs.end();
        }

        // Знімаємо захист відкату в завантажувачі ESP-IDF
        StandaloneOta::confirmBootValid();
        Serial.printf("[BootSafetyGuard] ✅ Firmware confirmed 100%% stable (uptime=%ums). Rollback disarmed, streak cleared.\n",
                      millis() - getBootMs());
    }

    static uint32_t getBootStreak() {
        return getStreak();
    }
};
