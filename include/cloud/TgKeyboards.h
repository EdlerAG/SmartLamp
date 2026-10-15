#pragma once
#include <Arduino.h>
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "cloud/CloudOtaService.h"

namespace TgKeyboards {
    constexpr const char* ADMIN_REPLY_KB = R"({"keyboard":[[{"text":"🎨 Режими та Свічка"},{"text":"🔆 Яскравість"},{"text":"⏳ Таймер сну"}],[{"text":"🔥 Вогник"},{"text":"⚡ Блискавка"},{"text":"🐱 Мем-Кіт"}],[{"text":"👽 Пороблено"},{"text":"👑 Корона"},{"text":"📊 Стан / Прошивка"}]],"resize_keyboard":true})";

    constexpr const char* GIRL_REPLY_KB  = R"({"keyboard":[[{"text":"🔥 Вогник"},{"text":"⚡ Блискавка"},{"text":"🐱 Кусь Кота"}],[{"text":"👽 Пороблено"},{"text":"👑 Корона"},{"text":"⏳ Таймер сну"}],[{"text":"🎨 Режими та Свічка"},{"text":"🔆 Яскравість"},{"text":"📊 Стан"}]],"resize_keyboard":true})";

    inline String buildModesInline(AppState& state) {
        String kb = "{\"inline_keyboard\":[";
        for (uint8_t i = 0; i < TOTAL_LIGHT_MODES; i += 2) {
            if (i > 0) kb += ",";
            kb += "[";
            String p1 = (state.light_mode == i) ? "✅ " : "";
            kb += "{\"text\":\"" + p1 + String(CommandRouter::getModeNameUa(i)) + "\",\"callback_data\":\"mode:" + String(i) + "\"}";
            if (i + 1 < TOTAL_LIGHT_MODES) {
                String p2 = (state.light_mode == (i + 1)) ? "✅ " : "";
                kb += ",{\"text\":\"" + p2 + String(CommandRouter::getModeNameUa(i + 1)) + "\",\"callback_data\":\"mode:" + String(i + 1) + "\"}";
            }
            kb += "]";
        }
        kb += ",[{\"text\":\"🔆 Яскравість\",\"callback_data\":\"menu:brt\"},{\"text\":\"⏳ Таймер сну\",\"callback_data\":\"menu:sleep\"}]]}";
        return kb;
    }

    inline String buildBrightnessInline(AppState& state) {
        String c_auto = state.auto_brt ? "✅ " : "";
        String c_0    = (!state.auto_brt && state.manual_brt_idx == 0) ? "✅ " : "";
        String c_1    = (!state.auto_brt && state.manual_brt_idx == 1) ? "✅ " : "";
        String c_2    = (!state.auto_brt && state.manual_brt_idx == 2) ? "✅ " : "";
        String c_3    = (!state.auto_brt && state.manual_brt_idx == 3) ? "✅ " : "";

        return "{\"inline_keyboard\":["
               "[{\"text\":\"" + c_auto + "🤖 Автоматична (Фоторезистор)\",\"callback_data\":\"brt:auto\"}],"
               "[{\"text\":\"" + c_0 + "🌙 15% Нічник\",\"callback_data\":\"brt:0\"},"
                "{\"text\":\"" + c_1 + "☕ 35% М'яко\",\"callback_data\":\"brt:1\"}],"
               "[{\"text\":\"" + c_2 + "✨ 65% Комфорт\",\"callback_data\":\"brt:2\"},"
                "{\"text\":\"" + c_3 + "☀️ 100% Макс\",\"callback_data\":\"brt:3\"}],"
               "[{\"text\":\"🎨 До списку режимів...\",\"callback_data\":\"menu:modes\"}]"
               "]}";
    }

    inline String buildSleepInline() {
        return R"({"inline_keyboard":[[{"text":"🕯 Сон 15 хв","callback_data":"slp:15"},{"text":"🕯 Сон 30 хв","callback_data":"slp:30"},{"text":"🕯 Сон 60 хв","callback_data":"slp:60"}],[{"text":"❌ Вимкнути таймер сну","callback_data":"slp:0"}],[{"text":"🎨 До списку режимів...","callback_data":"menu:modes"}]]})";
    }

    inline String buildStatusReport(AppState& state) {
        char buf[600];
        uint32_t up_min = millis() / 60000;
        uint16_t sl_min = state.getSleepTimerRemainingMin();
        char sleep_line[48] = "Вимкнено";
        if (sl_min > 0) snprintf(sleep_line, sizeof(sleep_line), "Активний (%u хв)", sl_min);

        const char* trend = (state.pressure_diff >= 0.8f) ? "Зростає ↑" :
                           ((state.pressure_diff <= -0.8f) ? "Падає ↓" : "Стабільний →");

        String part_info = CloudOtaService::getPartitionInfo();

        snprintf(buf, sizeof(buf),
                 "🪄 СТАН ШКАТУЛКИ MAGICBOX\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "💡 Як зараз світиться:\n"
                 "• Режим: %s\n"
                 "• Яскравість: %d%% (%s)\n"
                 "• Таймер сну: %s\n"
                 "• Світло в кімнаті: %d%%\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "🌡 Клімат та Барометр:\n"
                 "• Температура: %.1f °C\n"
                 "• Вологість: %.0f %%\n"
                 "• Тиск: %.0f мм (%s)\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "⚙️ %s | ☁️ Oracle: %s\n"
                 "📶 Wi-Fi: %d dBm | ⏱ %lu хв\n"
                 "📦 Для OTA просто надішли сюди файл .bin!",
                 CommandRouter::getModeNameUa(state.light_mode),
                 state.getLedBrightnessPercent(),
                 CommandRouter::getBrightnessPresetName(state.auto_brt, state.manual_brt_idx),
                 sleep_line,
                 state.light_percent,
                 state.temperature, state.humidity, state.pressure, trend,
                 part_info.c_str(), state.oracle_online ? "ON" : "OFF",
                 (int)state.wifi_rssi, (unsigned long)up_min);
        return String(buf);
    }
}