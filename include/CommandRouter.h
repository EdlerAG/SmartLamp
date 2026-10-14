#pragma once
#include <Arduino.h>
#include "AppState.h"

class CommandRouter {
public:
    static const char* getModeNameAscii(uint8_t mode) {
        static const char* const names[TOTAL_LIGHT_MODES] = {
            "WARM COZY",   "CANDLE LIGHT", "AMBER EMBER",
            "FIREPLACE",   "SUNSET VIBE",  "SAKURA NEON",
            "AURORA WAVE", "OCEAN BREEZE", "CYBER RUNNER",
            "RAINBOW FLOW","LIGHT OFF"
        };
        return names[mode % TOTAL_LIGHT_MODES];
    }

    static const char* getModeNameUa(uint8_t mode) {
        static const char* const names_ua[TOTAL_LIGHT_MODES] = {
            "☀️ Теплий затишок",
            "🕯 Жива свічка",
            "🍯 Бурштинова свічка",
            "🔥 Камін з іскрами",
            "🌅 Захід сонця",
            "🌸 Сакура Неон",
            "🌌 Північне сяйво",
            "🌊 Океанський бриз",
            "⚡ Кібер-комета",
            "🌈 Веселка",
            "🌑 Вимкнено"
        };
        return names_ua[mode % TOTAL_LIGHT_MODES];
    }

    static const char* getBrightnessPresetName(bool auto_brt, uint8_t idx) {
        if (auto_brt) return "🤖 Авто (Фоторезистор)";
        static const char* const b_names[4] = {
            "🌙 15% (Нічник)",
            "☕ 35% (М'яко)",
            "✨ 65% (Комфорт)",
            "☀️ 100% (Максимум)"
        };
        return b_names[idx % 4];
    }

    static void setLightMode(AppState& state, uint8_t mode) {
        state.light_mode = mode % TOTAL_LIGHT_MODES;
        if (state.light_mode == MODE_LIGHT_OFF) {
            state.sleep_timer_active = false;
        }
        state.markSettingsDirty();
    }

    static void nextLightMode(AppState& state) {
        state.light_mode = (state.light_mode + 1) % TOTAL_LIGHT_MODES;
        state.markSettingsDirty();
    }

    static void togglePower(AppState& state) {
        if (state.light_mode == MODE_LIGHT_OFF) {
            state.light_mode = MODE_WARM_COZY;
        } else {
            state.light_mode = MODE_LIGHT_OFF;
            state.sleep_timer_active = false;
        }
        state.markSettingsDirty();
    }

    static void setBrightnessPreset(AppState& state, bool auto_mode, uint8_t preset_idx = 2) {
        state.auto_brt = auto_mode;
        if (!auto_mode) {
            state.manual_brt_idx = preset_idx % 4;
        }
        state.markSettingsDirty();
    }

    static void cycleBrightness(AppState& state) {
        if (state.auto_brt) {
            state.auto_brt = false;
            state.manual_brt_idx = 0;
        } else {
            state.manual_brt_idx++;
            if (state.manual_brt_idx >= 4) {
                state.auto_brt = true;
                state.manual_brt_idx = 2;
            }
        }
        state.markSettingsDirty();
    }

    // Запуск або скасування таймера сну (minutes = 0 вимикає таймер)
    static void setSleepTimer(AppState& state, uint16_t minutes) {
        if (minutes == 0) {
            state.sleep_timer_active = false;
        } else {
            uint32_t dur = (uint32_t)minutes * 60000UL;
            state.sleep_timer_total_ms = dur;
            state.sleep_timer_end_ms   = millis() + dur;
            state.sleep_timer_active   = true;
            // Якщо світло було вимкнене — вмикаємо ніжну бурштинову свічку
            if (state.light_mode == MODE_LIGHT_OFF) {
                state.light_mode = MODE_AMBER_EMBER;
            }
        }
        state.registerActivity();
    }

    // Гортання збережених повідомлень в Inbox
    static void nextInboxMessage(AppState& state) {
        if (state.inbox_count > 1) {
            state.inbox_view_idx = (state.inbox_view_idx + 1) % state.inbox_count;
            state.registerActivity();
        }
    }

    static void triggerReaction(AppState& state, NotifyLedType type, const char* sender = nullptr, const char* label = nullptr) {
        if (sender && label) {
            state.setTelegramData(sender, label);
        }
        state.triggerLedNotification(type, true);
    }

    static void pushMessage(AppState& state, const char* sender, const char* text) {
        state.setTelegramData(sender, text);
        state.screen_mode = SCREEN_TELEGRAM;
        state.triggerLedNotification(NOTIFY_CYAN_WAVE, false);
    }

    // Повний JSON статус для Web та майбутнього мобільного застосунку
    static void buildStatusJson(AppState& state, char* out, size_t max_len) {
        char t_str[10];
        state.getTimeString(t_str, sizeof(t_str));

        snprintf(out, max_len,
                 "{\"t\":%.1f,\"h\":%.0f,\"p\":%.0f,\"p_diff\":%.1f,\"l\":%d,"
                 "\"m\":%d,\"m_name\":\"%s\","
                 "\"auto_b\":%s,\"b_idx\":%d,\"b_pct\":%d,"
                 "\"sleep_m\":%u,\"scr\":%d,\"rssi\":%d,\"time\":\"%s\","
                 "\"unread\":%s,\"inbox_cnt\":%u,\"heap\":%lu}",
                 state.temperature, state.humidity, state.pressure, state.pressure_diff, state.light_percent,
                 state.light_mode, getModeNameUa(state.light_mode),
                 state.auto_brt ? "true" : "false", state.manual_brt_idx, state.getLedBrightnessPercent(),
                 state.getSleepTimerRemainingMin(), state.screen_mode, (int)state.wifi_rssi, t_str,
                 state.tg_unread ? "true" : "false", state.inbox_count, (unsigned long)ESP.getFreeHeap());
    }

    // JSON-історія останніх 5 повідомлень для мобільного застосунку та Web
    static void buildInboxJson(AppState& state, char* out, size_t max_len) {
        size_t pos = snprintf(out, max_len, "{\"unread\":%s,\"messages\":[", state.tg_unread ? "true" : "false");
        for (uint8_t i = 0; i < state.inbox_count; i++) {
            char s[28], m[64], tm[8];
            portENTER_CRITICAL(&state.mux);
            strncpy(s, state.inbox[i].sender, sizeof(s) - 1); s[sizeof(s) - 1] = '\0';
            strncpy(m, state.inbox[i].text, sizeof(m) - 1);   m[sizeof(m) - 1] = '\0';
            strncpy(tm, state.inbox[i].time_received, sizeof(tm) - 1); tm[sizeof(tm) - 1] = '\0';
            portEXIT_CRITICAL(&state.mux);

            int w = snprintf(out + pos, max_len - pos - 4,
                             "%s{\"sender\":\"%s\",\"text\":\"%s\",\"time\":\"%s\"}",
                             (i == 0) ? "" : ",", s, m, tm);
            if (w > 0 && pos + w < max_len - 4) pos += w;
        }
        snprintf(out + pos, max_len - pos, "]}");
    }
};