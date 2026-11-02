#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include "cloud/StandaloneOta.h"

/**
 * ============================================================================
 * BootSafetyGuard — Автономний захист від очорніння (Anti-Brick System)
 * 
 * 1. Відслідковує поспіль краші через NVS-лічильник (Boot Streak).
 * 2. Якщо плата крашиться >= 3 разів до стабілізації — миттєвий апаратний
 *    відкат (Rollback) на попередню 100% робочу прошивку OTA без кабелю.
 * 3. Якщо відкат недоступний — аварійний Failsafe-режим (тільки Wi-Fi + OTA,
 *    без важких сервісів UI та анімацій), щоб прийняти виправлення по хмарі.
 * 4. Підтвердження валідності прошивки (confirmBootValid) виконується ВИКЛЮЧНО
 *    після 25 секунд стабільної роботи або успішного виходу в хмару.
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

public:
    static void onEarlyBoot() {
        getBootMs() = millis();
        Preferences prefs;
        if (prefs.begin("boot_safety", false)) {
            getStreak() = prefs.getUInt("streak", 0) + 1;
            prefs.putUInt("streak", getStreak());
            prefs.end();
        }

        Serial.printf("[BootSafetyGuard] 🛡️ Boot sequence started. Streak count: %u\n", getStreak());

        // Якщо зафіксовано 3 або більше невдалих стартів поспіль:
        if (getStreak() >= 3) {
            Serial.println("[BootSafetyGuard] ⚠️⚠️⚠️ CRITICAL: Crash loop detected (streak >= 3)!");

            if (Update.canRollBack()) {
                Serial.println("[BootSafetyGuard] 🔄 Executing autonomous OTA rollback to previous known-good firmware...");
                if (prefs.begin("boot_safety", false)) {
                    prefs.putUInt("streak", 0);
                    prefs.end();
                }
                delay(150);
                StandaloneOta::rollback();
                delay(400);
                ESP.restart();
            } else {
                Serial.println("[BootSafetyGuard] ⚠️ Rollback partition unavailable. Entering FAILSAFE RESCUE MODE.");
                getFailsafe() = true;
            }
        }
    }

    static bool isFailsafeMode() {
        return getFailsafe();
    }

    static void tick(bool is_cloud_online) {
        if (isConfirmed()) return;

        uint32_t uptime = millis() - getBootMs();
        // Підтверджуємо здоров'я прошивки, якщо успішно з'єдналися з хмарою
        // або пропрацювали понад 25 секунд без крашу
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

        // Тільки зараз знімаємо захист відкату в завантажувачі ESP-IDF
        StandaloneOta::confirmBootValid();
        Serial.printf("[BootSafetyGuard] ✅ Firmware confirmed 100%% stable (uptime=%ums). Rollback disarmed, streak cleared.\n",
                      millis() - getBootMs());
    }

    static uint32_t getBootStreak() {
        return getStreak();
    }
};
