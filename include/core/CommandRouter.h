#pragma once
#include <Arduino.h>
#include "core/AppState.h"

class CommandRouter {
private:
    static void sanitizeJsonStr(const char* in, char* out, size_t max_len) {
        size_t j = 0;
        for (size_t i = 0; in[i] != '\0' && j < max_len - 1; i++) {
            char c = in[i];
            if (c == '\"' || c == '\\') c = '\'';
            else if ((uint8_t)c < 0x20) c = ' ';
            out[j++] = c;
        }
        out[j] = '\0';
    }

public:
    static const char* getModeNameShortUa(uint8_t mode) {
        static const char* const names[TOTAL_LIGHT_MODES] = {
            "Теплий затишок", "Жива свічка",     "Бурштин",
            "Камін з іскрами","Захід сонця",     "Сакура Неон",
            "Північне сяйво", "Океанський бриз", "Кібер-комета",
            "Веселка",        "Вимкнено",        "Spotify Ambi"
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
            "🌑 Вимкнено",
            "🎵 Spotify AmbiLight"
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

    static void setVolume(AppState& state, uint8_t vol) {
        state.setVolume(vol);
    }

    static void setMute(AppState& state, bool muted) {
        state.setMute(muted);
    }

    static void setTouchBeep(AppState& state, bool beep) {
        state.setTouchBeep(beep);
    }

    static void stopActiveMedia(AppState& state) {
        state.stopActiveMedia();
    }

    static void setMediaTimer(AppState& state, uint16_t minutes) {
        state.setMediaTimer(minutes);
    }

    static void setSleepTimer(AppState& state, uint16_t minutes) {
        if (minutes == 0) {
            state.sleep_timer_active = false;
        } else {
            uint32_t dur = (uint32_t)minutes * 60000UL;
            state.sleep_timer_total_ms = dur;
            state.sleep_timer_end_ms   = millis() + dur;
            state.sleep_timer_active   = true;
            if (state.light_mode == MODE_LIGHT_OFF) {
                state.light_mode = MODE_AMBER_EMBER;
            }
        }
        state.registerActivity();
    }

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
        state.screen_mode = SCREEN_SMART_HUB;
        state.triggerLedNotification(NOTIFY_CYAN_WAVE, false);
    }

    static void buildStatusJson(AppState& state, char* out, size_t max_len) {
        char t_str[10];
        state.getTimeString(t_str, sizeof(t_str));

        bool ack_flag = false;
        uint8_t ping_val = 0;
        int8_t sp_act = 0;

        portENTER_CRITICAL(&state.mux);
        if (state.oracle_online && state.oracle_manages_bot) {
            ack_flag = state.cmd_send_tg_ack;
            ping_val = state.cmd_send_meme_ping;
            state.cmd_send_tg_ack    = false;
            state.cmd_send_meme_ping = 0;
        }
        sp_act = state.cmd_spotify_action;
        state.cmd_spotify_action = 0;
        portEXIT_CRITICAL(&state.mux);

        snprintf(out, max_len,
                 "{\"fw\":\"%s\",\"t\":%.1f,\"h\":%.0f,\"p\":%.0f,\"p_diff\":%.1f,\"l\":%d,"
                 "\"m\":%d,\"m_name\":\"%s\","
                 "\"auto_b\":%s,\"b_idx\":%d,\"b_pct\":%d,"
                 "\"vol\":%u,\"muted\":%s,\"t_beep\":%s,\"dnd\":%s,"
                 "\"pomo\":%s,\"pomo_rem\":%u,\"pomo_brk\":%s,\"pomo_dur\":%u,"
                 "\"sleep_m\":%u,\"media_m\":%u,\"scr\":%d,\"rssi\":%d,\"time\":\"%s\","
                 "\"unread\":%s,\"inbox_cnt\":%u,\"oracle\":%s,"
                 "\"sp_act\":%d,\"sp_playing\":%s,"
                 "\"heap\":%lu,\"psram_free\":%lu,"
                 "\"ack\":%s,\"ping\":%u}",
                 Config::FW_VERSION,
                 state.temperature, state.humidity, state.pressure, state.pressure_diff, state.light_percent,
                 state.light_mode, getModeNameUa(state.light_mode),
                 state.auto_brt ? "true" : "false", state.manual_brt_idx, state.getLedBrightnessPercent(),
                 state.audio_volume, state.audio_muted ? "true" : "false", state.touch_beep_enabled ? "true" : "false",
                 state.dnd_mode ? "true" : "false",
                 state.pomo_active ? "true" : "false", state.getPomoRemainingSeconds(),
                 state.pomo_is_break ? "true" : "false", (unsigned int)state.pomo_duration_min,
                 state.getSleepTimerRemainingMin(), state.getMediaRemainingMin(), state.screen_mode, (int)state.wifi_rssi, t_str,
                 state.tg_unread ? "true" : "false", state.inbox_count,
                 state.oracle_online ? "true" : "false",
                 (int)sp_act, state.spotify.playing ? "true" : "false",
                 (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram(),
                 ack_flag ? "true" : "false", ping_val);
    }

    static void buildInboxJson(AppState& state, char* out, size_t max_len) {
        size_t pos = snprintf(out, max_len, "{\"unread\":%s,\"messages\":[", state.tg_unread ? "true" : "false");
        for (uint8_t i = 0; i < state.inbox_count; i++) {
            char raw_s[36], raw_m[128], tm[8];
            portENTER_CRITICAL(&state.mux);
            strncpy(raw_s, state.inbox[i].sender, sizeof(raw_s) - 1); raw_s[sizeof(raw_s) - 1] = '\0';
            strncpy(raw_m, state.inbox[i].text, sizeof(raw_m) - 1);   raw_m[sizeof(raw_m) - 1] = '\0';
            strncpy(tm, state.inbox[i].time_received, sizeof(tm) - 1); tm[sizeof(tm) - 1] = '\0';
            portEXIT_CRITICAL(&state.mux);

            char safe_s[36], safe_m[128];
            sanitizeJsonStr(raw_s, safe_s, sizeof(safe_s));
            sanitizeJsonStr(raw_m, safe_m, sizeof(safe_m));

            int w = snprintf(out + pos, max_len - pos - 4,
                             "%s{\"sender\":\"%s\",\"text\":\"%s\",\"time\":\"%s\"}",
                             (i == 0) ? "" : ",", safe_s, safe_m, tm);
            if (w > 0 && pos + w < max_len - 4) pos += w;
        }
        snprintf(out + pos, max_len - pos, "]}");
    }
};
