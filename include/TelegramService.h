#pragma once
#include "IService.h"
#include "Config.h"
#include "AppState.h"
#include "CommandRouter.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class TelegramService : public IService {
private:
    AppState& state;
    WiFiClientSecure client;
    int64_t  last_update_id   = 0;
    uint32_t last_poll_ms     = 0;
    bool     cold_boot_synced = false;

    const char* ADMIN_KEYBOARD = R"({"keyboard":[[{"text":"🎨 Режими та Свічка"},{"text":"🔆 Яскравість"},{"text":"⏳ Таймер сну"}],[{"text":"🔥 Вогник"},{"text":"⚡ Блискавка"},{"text":"🐱 Мем-Кіт"}],[{"text":"👽 Пороблено"},{"text":"👑 Корона"},{"text":"📊 Стан / Історія"}]],"resize_keyboard":true})";

    const char* GIRL_KEYBOARD  = R"({"keyboard":[[{"text":"🔥 Вогник"},{"text":"⚡ Блискавка"},{"text":"🐱 Кусь Кота"}],[{"text":"👽 Пороблено"},{"text":"👑 Корона"},{"text":"⏳ Таймер сну"}],[{"text":"🎨 Режими та Свічка"},{"text":"🔆 Яскравість"},{"text":"📊 Стан"}]],"resize_keyboard":true})";

    static void decodeJsonString(const String& in, char* out, size_t max_len) {
        size_t j = 0;
        size_t len = in.length();
        for (size_t i = 0; i < len && j < max_len - 4; i++) {
            if (in[i] == '\\' && i + 1 < len) {
                char next = in[i + 1];
                if (next == 'u' && i + 5 < len) {
                    char hex[5] = { in[i+2], in[i+3], in[i+4], in[i+5], 0 };
                    uint16_t cp = (uint16_t)strtol(hex, nullptr, 16);
                    if (cp < 0x80) {
                        out[j++] = (char)cp;
                    } else if (cp < 0x800) {
                        out[j++] = (char)(0xC0 | (cp >> 6));
                        out[j++] = (char)(0x80 | (cp & 0x3F));
                    } else {
                        out[j++] = (char)(0xE0 | (cp >> 12));
                        out[j++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        out[j++] = (char)(0x80 | (cp & 0x3F));
                    }
                    i += 5;
                } else if (next == 'n' || next == 'r') {
                    out[j++] = ' ';
                    i++;
                } else {
                    out[j++] = next;
                    i++;
                }
            } else {
                out[j++] = in[i];
            }
        }
        out[j] = '\0';
    }

    static String extractJsonString(const String& json, const char* key, int start_from = 0) {
        String search = String("\"") + key + "\":\"";
        int idx = json.indexOf(search, start_from);
        if (idx == -1) return "";
        idx += search.length();
        int end = idx;
        while (end < (int)json.length()) {
            if (json[end] == '\"' && json[end - 1] != '\\') break;
            end++;
        }
        return json.substring(idx, end);
    }

    static String extractJsonIntStr(const String& json, const char* key, int start_from = 0) {
        String search = String("\"") + key + "\":";
        int idx = json.indexOf(search, start_from);
        if (idx == -1) return "";
        idx += search.length();
        while (idx < (int)json.length() && json[idx] == ' ') idx++;
        int end = idx;
        while (end < (int)json.length() && (isdigit(json[end]) || json[end] == '-')) end++;
        return json.substring(idx, end);
    }

    static String urlEncode(const String& str) {
        String encoded = "";
        encoded.reserve(str.length() * 2);
        const char* hex = "0123456789ABCDEF";
        for (size_t i = 0; i < str.length(); i++) {
            uint8_t c = (uint8_t)str[i];
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += (char)c;
            } else {
                encoded += '%';
                encoded += hex[c >> 4];
                encoded += hex[c & 0x0F];
            }
        }
        return encoded;
    }

    bool sendMessage(const char* token, const char* chat_id, const String& text, const char* reply_markup = nullptr) {
        if (!state.wifi_connected || strlen(token) < 10 || strlen(chat_id) == 0) return false;

        state.tls_in_flight = true;
        HTTPClient https;
        https.setConnectTimeout(3000);
        https.setTimeout(3000);
        https.setReuse(true);

        String url = String("https://api.telegram.org/bot") + token +
                     "/sendMessage?chat_id=" + chat_id +
                     "&text=" + urlEncode(text);
        if (reply_markup != nullptr) {
            url += "&reply_markup=" + urlEncode(String(reply_markup));
        }

        bool ok = false;
        if (https.begin(client, url)) {
            ok = (https.GET() == 200);
            https.end();
        }
        state.tls_in_flight = false;
        return ok;
    }

    bool editMessage(const char* token, const char* chat_id, const char* msg_id, const String& text, const char* reply_markup = nullptr) {
        if (!state.wifi_connected || strlen(token) < 10 || strlen(chat_id) == 0 || strlen(msg_id) == 0) return false;

        state.tls_in_flight = true;
        HTTPClient https;
        https.setConnectTimeout(3000);
        https.setTimeout(3000);
        https.setReuse(true);

        String url = String("https://api.telegram.org/bot") + token +
                     "/editMessageText?chat_id=" + chat_id +
                     "&message_id=" + msg_id +
                     "&text=" + urlEncode(text);
        if (reply_markup != nullptr) {
            url += "&reply_markup=" + urlEncode(String(reply_markup));
        }

        bool ok = false;
        if (https.begin(client, url)) {
            ok = (https.GET() == 200);
            https.end();
        }
        state.tls_in_flight = false;
        return ok;
    }

    void answerCallback(const char* token, const String& cb_id, const String& toast_text) {
        if (!state.wifi_connected || cb_id.length() == 0) return;
        state.tls_in_flight = true;
        HTTPClient https;
        https.setConnectTimeout(2500);
        https.setTimeout(2500);
        https.setReuse(true);

        String url = String("https://api.telegram.org/bot") + token +
                     "/answerCallbackQuery?callback_query_id=" + cb_id +
                     "&text=" + urlEncode(toast_text);
        if (https.begin(client, url)) {
            https.GET();
            https.end();
        }
        state.tls_in_flight = false;
    }

    String buildFullStatusText() {
        char buf[520];
        uint32_t up_min = millis() / 60000;
        uint16_t sl_min = state.getSleepTimerRemainingMin();
        char sleep_line[48] = "Вимкнено";
        if (sl_min > 0) snprintf(sleep_line, sizeof(sleep_line), "Активний (залишилось %u хв)", sl_min);

        const char* trend = (state.pressure_diff >= 0.8f) ? "Зростає ↑" :
                           ((state.pressure_diff <= -0.8f) ? "Падає ↓" : "Стабільний →");

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
                 "📶 Wi-Fi: %d dBm | ⏱ %lu хв",
                 CommandRouter::getModeNameUa(state.light_mode),
                 state.getLedBrightnessPercent(),
                 CommandRouter::getBrightnessPresetName(state.auto_brt, state.manual_brt_idx),
                 sleep_line,
                 state.light_percent,
                 state.temperature, state.humidity, state.pressure, trend,
                 (int)state.wifi_rssi, (unsigned long)up_min);
        return String(buf);
    }

    String buildModesInlineKeyboard() {
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

    String buildBrightnessInlineKeyboard() {
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

    String buildSleepInlineKeyboard() {
        return R"({"inline_keyboard":[[{"text":"🕯 Сон 15 хв","callback_data":"slp:15"},{"text":"🕯 Сон 30 хв","callback_data":"slp:30"},{"text":"🕯 Сон 60 хв","callback_data":"slp:60"}],[{"text":"❌ Вимкнути таймер сну","callback_data":"slp:0"}],[{"text":"🎨 До списку режимів...","callback_data":"menu:modes"}]]})";
    }

    void handleCallbackQuery(const String& payload, int cb_pos, const char* token) {
        String cb_id = extractJsonString(payload, "id", cb_pos);
        String data  = extractJsonString(payload, "data", cb_pos);

        int msg_pos = payload.indexOf("\"message\":", cb_pos);
        String msg_id  = (msg_pos != -1) ? extractJsonIntStr(payload, "message_id", msg_pos) : "";
        int chat_pos   = (msg_pos != -1) ? payload.indexOf("\"chat\":", msg_pos) : -1;
        String chat_id = (chat_pos != -1) ? extractJsonIntStr(payload, "id", chat_pos) : "";

        if (data.startsWith("mode:")) {
            uint8_t m = (uint8_t)data.substring(5).toInt();
            CommandRouter::setLightMode(state, m);
            answerCallback(token, cb_id, String("Увімкнено: ") + CommandRouter::getModeNameUa(m));

            String header = "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ\n"
                            "Зараз світиться: " + String(CommandRouter::getModeNameUa(state.light_mode)) +
                            "\nЯскравість: " + String(state.getLedBrightnessPercent()) + "%";
            String kb = buildModesInlineKeyboard();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        }
        else if (data.startsWith("brt:")) {
            String val = data.substring(4);
            if (val == "auto") {
                CommandRouter::setBrightnessPreset(state, true);
                answerCallback(token, cb_id, "🤖 Авто-яскравість увімкнено!");
            } else {
                uint8_t idx = (uint8_t)val.toInt();
                CommandRouter::setBrightnessPreset(state, false, idx);
                answerCallback(token, cb_id, CommandRouter::getBrightnessPresetName(false, idx));
            }
            String header = "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ\n"
                            "Режим: " + String(CommandRouter::getBrightnessPresetName(state.auto_brt, state.manual_brt_idx)) +
                            "\nПотужність LED: " + String(state.getLedBrightnessPercent()) + "% | Світло в кімнаті: " + String(state.light_percent) + "%";
            String kb = buildBrightnessInlineKeyboard();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        }
        else if (data.startsWith("slp:")) {
            uint16_t mins = (uint16_t)data.substring(4).toInt();
            CommandRouter::setSleepTimer(state, mins);
            answerCallback(token, cb_id, mins > 0 ? ("🌙 Таймер сну на " + String(mins) + " хв!") : "Таймер вимкнено");
            String header = (mins > 0)
                            ? ("⏳ Таймер сну запущено на " + String(mins) + " хв!\nЛампа плавно приглушуватиме свічку до повного вимкнення.")
                            : "⏳ Таймер сну вимкнено.";
            String kb = buildSleepInlineKeyboard();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        }
        else if (data == "menu:brt") {
            answerCallback(token, cb_id, "Меню яскравості");
            String header = "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ\nПотужність: " + String(state.getLedBrightnessPercent()) + "%";
            String kb = buildBrightnessInlineKeyboard();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        }
        else if (data == "menu:sleep") {
            answerCallback(token, cb_id, "Таймер сну");
            String header = "⏳ ТАЙМЕР СНУ «ЗАХІД СОНЦЯ»\nОберіть час до плавного згасання:";
            String kb = buildSleepInlineKeyboard();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        }
        else if (data == "menu:modes") {
            answerCallback(token, cb_id, "Список режимів");
            String header = "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ\nЗараз: " + String(CommandRouter::getModeNameUa(state.light_mode));
            String kb = buildModesInlineKeyboard();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        }
    }

    void handleIncomingPayload(const String& payload, const char* token, const char* admin_id, const char* girl_id) {
        int upd_idx = payload.indexOf("\"update_id\":");
        if (upd_idx == -1) return;

        int64_t upd_id = strtoll(payload.c_str() + upd_idx + 12, nullptr, 10);
        if (upd_id <= 0) return;

        // Якщо це найперший запит після увімкнення живлення — просто запам'ятовуємо останній update_id без виконання старих команд
        if (!cold_boot_synced) {
            cold_boot_synced = true;
            last_update_id = upd_id;
            return;
        }
        last_update_id = upd_id;

        int cb_idx = payload.indexOf("\"callback_query\":", upd_idx);
        if (cb_idx != -1) {
            handleCallbackQuery(payload, cb_idx, token);
            return;
        }

        int msg_idx = payload.indexOf("\"message\":", upd_idx);
        if (msg_idx == -1) return;

        int chat_idx = payload.indexOf("\"chat\":", msg_idx);
        String chat_id = (chat_idx != -1) ? extractJsonIntStr(payload, "id", chat_idx) : "";
        if (chat_id.length() == 0) return;

        String raw_name = extractJsonString(payload, "first_name", msg_idx);
        if (raw_name.length() == 0) raw_name = "Telegram";
        String raw_text = extractJsonString(payload, "text", msg_idx);
        if (raw_text.length() == 0) return;

        char sender_utf8[28], text_utf8[96];
        decodeJsonString(raw_name, sender_utf8, sizeof(sender_utf8));
        decodeJsonString(raw_text, text_utf8, sizeof(text_utf8));
        String msg = String(text_utf8);
        msg.trim();

        if (msg.equalsIgnoreCase("/iam_admin")) {
            state.setTelegramConfig(nullptr, chat_id.c_str(), nullptr);
            state.tg_config_dirty = true;
            sendMessage(token, chat_id.c_str(), "👑 Акаунт Ярослава (Admin) прив'язано!", ADMIN_KEYBOARD);
            return;
        }

        if (msg.equalsIgnoreCase("/iam_nastia") || msg.equalsIgnoreCase("/iam_girl")) {
            state.setTelegramConfig(nullptr, nullptr, chat_id.c_str());
            state.tg_config_dirty = true;
            sendMessage(token, chat_id.c_str(), "🌸 Акаунт власниці шкатулки прив'язано!", GIRL_KEYBOARD);
            return;
        }

        bool is_admin = (strlen(admin_id) > 0 && chat_id == String(admin_id));
        bool is_girl  = (strlen(girl_id) > 0  && chat_id == String(girl_id));
        const char* active_kb = is_admin ? ADMIN_KEYBOARD : (is_girl ? GIRL_KEYBOARD : ADMIN_KEYBOARD);

        if (msg.startsWith("/start") || msg.startsWith("/help") || msg.startsWith("/menu")) {
            sendMessage(token, chat_id.c_str(), buildFullStatusText(), active_kb);
            return;
        }

        if (msg.startsWith("/modes") || msg.indexOf("Режими") != -1 || msg.indexOf("Свічка") != -1) {
            String header = "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ\n"
                            "Зараз світиться: " + String(CommandRouter::getModeNameUa(state.light_mode)) +
                            "\nЯскравість: " + String(state.getLedBrightnessPercent()) + "%";
            String inline_kb = buildModesInlineKeyboard();
            sendMessage(token, chat_id.c_str(), header, inline_kb.c_str());
            return;
        }

        if (msg.startsWith("/brt") || msg.indexOf("Яскравість") != -1) {
            String header = "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ\n"
                            "Режим: " + String(CommandRouter::getBrightnessPresetName(state.auto_brt, state.manual_brt_idx)) +
                            "\nПотужність LED: " + String(state.getLedBrightnessPercent()) + "%";
            String inline_kb = buildBrightnessInlineKeyboard();
            sendMessage(token, chat_id.c_str(), header, inline_kb.c_str());
            return;
        }

        if (msg.startsWith("/sleep") || msg.indexOf("Таймер") != -1) {
            String header = "⏳ ТАЙМЕР СНУ «ЗАХІД СОНЦЯ»\nЛампа плавно приглушуватиме світло до вимкнення:";
            String inline_kb = buildSleepInlineKeyboard();
            sendMessage(token, chat_id.c_str(), header, inline_kb.c_str());
            return;
        }

        if (msg.startsWith("/status") || msg.indexOf("Стан") != -1) {
            String inline_kb = buildModesInlineKeyboard();
            sendMessage(token, chat_id.c_str(), buildFullStatusText(), inline_kb.c_str());
            return;
        }

        // --- 5 МЕМНИХ РЕАКЦІЙ ---
        if (msg.indexOf("Вогник") != -1 || msg.startsWith("/fire")) {
            CommandRouter::triggerReaction(state, NOTIFY_FIRE, sender_utf8, "FIRE VIBE! TSE PIDPAL!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "🔥 Настя кинула тобі ВОГНИК («Це підпал!»)!", ADMIN_KEYBOARD);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "🔥 Ярослав запустив ВОГНИК на твоїй шкатулці!", GIRL_KEYBOARD);
            sendMessage(token, chat_id.c_str(), "🔥 Вогняний спалах запущено на шкатулці!", active_kb);
            return;
        }

        if (msg.indexOf("Блискавка") != -1 || msg.startsWith("/zap")) {
            CommandRouter::triggerReaction(state, NOTIFY_LIGHTNING, sender_utf8, "KUS-ZAP! BZZT!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "⚡ Настя бахнула тебе БЛИСКАВКОЮ («Кусь-розряд!»)!", ADMIN_KEYBOARD);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "⚡ Ярослав надіслав БЛИСКАВКУ на шкатулку!", GIRL_KEYBOARD);
            sendMessage(token, chat_id.c_str(), "⚡ Блискавка запущена на екрані та стрічці!", active_kb);
            return;
        }

        if (msg.indexOf("Кіт") != -1 || msg.indexOf("Кота") != -1 || msg.startsWith("/cat")) {
            CommandRouter::triggerReaction(state, NOTIFY_CAT, sender_utf8, "MEOW! TYTS-KUS =^._.^=");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "🐱 Настя надіслала тобі МЕМНОГО КОТА («Тиць-Кусь»)!", ADMIN_KEYBOARD);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "🐱 Ярослав випустив МЕМНОГО КОТА на екран шкатулки!", GIRL_KEYBOARD);
            sendMessage(token, chat_id.c_str(), "🐱 Мемний кіт підморгує на екрані шкатулки!", active_kb);
            return;
        }

        if (msg.indexOf("Пороблено") != -1 || msg.startsWith("/ufo")) {
            CommandRouter::triggerReaction(state, NOTIFY_UFO, sender_utf8, "UFO BEAM! POROBLENO!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "👽 Настя запустила НЛО («Пороблено!»)!", ADMIN_KEYBOARD);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "👽 Ярослав запустив НЛО на твоїй шкатулці!", GIRL_KEYBOARD);
            sendMessage(token, chat_id.c_str(), "👽 Прибульці прилетіли на екран шкатулки!", active_kb);
            return;
        }

        if (msg.indexOf("Корона") != -1 || msg.startsWith("/crown")) {
            CommandRouter::triggerReaction(state, NOTIFY_CROWN, sender_utf8, "QUEEN VIBE! KRASH!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "👑 Настя активувала КОРОНУ на шкатулці!", ADMIN_KEYBOARD);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "👑 Ярослав надіслав тобі КОРОЛІВСЬКУ КОРОНУ!", GIRL_KEYBOARD);
            sendMessage(token, chat_id.c_str(), "👑 Золота корона сяє на екрані!", active_kb);
            return;
        }

        // --- ОСОБИСТІ ПОВІДОМЛЕННЯ ---
        if (is_girl) {
            if (strlen(admin_id) > 0) {
                sendMessage(token, admin_id, "💬 Повідомлення від Насті:\n«" + msg + "»", ADMIN_KEYBOARD);
                sendMessage(token, chat_id.c_str(), "📨 Відповідь доставлена Ярославу!", GIRL_KEYBOARD);
            } else {
                sendMessage(token, chat_id.c_str(), "⚠️ Ярослав ще не активував /iam_admin");
            }
            state.triggerLedNotification(NOTIFY_CAT, false);
        } else {
            CommandRouter::pushMessage(state, sender_utf8, text_utf8);
            sendMessage(token, chat_id.c_str(), "📩 Виведено на екран та збережено в Inbox шкатулки!", active_kb);
            if (strlen(girl_id) > 0 && chat_id != String(girl_id)) {
                sendMessage(token, girl_id, "✨ Нове повідомлення на шкатулці від " + String(sender_utf8) + ":\n«" + msg + "»", GIRL_KEYBOARD);
            }
        }
    }

public:
    TelegramService(AppState& st) : state(st) {}

    void init() override {
        client.setInsecure();
        client.setTimeout(4);
    }

    void update() override {
        if (!state.wifi_connected || state.ap_mode_active || state.ota_in_progress) {
            return;
        }

        char token[64], admin_id[20], girl_id[20];
        state.getTelegramConfig(token, sizeof(token), admin_id, sizeof(admin_id), girl_id, sizeof(girl_id));
        if (strlen(token) < 15) return;

        if (state.cmd_send_tg_ack) {
            if (strlen(admin_id) > 0) {
                if (sendMessage(token, admin_id, "✅ Настя торкнулася кришки шкатулки — твоє повідомлення прочитано! 🔥")) {
                    state.cmd_send_tg_ack = false;
                }
            } else {
                state.cmd_send_tg_ack = false;
            }
        }

        if (state.cmd_send_meme_ping > 0) {
            uint8_t ping_type = state.cmd_send_meme_ping;
            if (strlen(admin_id) > 0) {
                const char* ping_txt = "🐱 Настя натиснула кнопку на шкатулці і передає тобі МЕМНИЙ КУСЬ (=^._.^=)!";
                if (ping_type == NOTIFY_FIRE)      ping_txt = "🔥 Настя натиснула кнопку на шкатулці і кинула тобі ВОГНИК!";
                if (ping_type == NOTIFY_LIGHTNING) ping_txt = "⚡ Настя натиснула кнопку на шкатулці і бахнула БЛИСКАВКОЮ!";
                if (ping_type == NOTIFY_UFO)       ping_txt = "👽 Настя натиснула кнопку на шкатулці: «Пороблено! UFO Beam»!";
                if (ping_type == NOTIFY_CROWN)     ping_txt = "👑 Настя передає тобі КОРОЛІВСЬКИЙ ПРИВІТ зі шкатулки!";
                if (sendMessage(token, admin_id, ping_txt)) {
                    state.cmd_send_meme_ping = 0;
                }
            } else {
                state.cmd_send_meme_ping = 0;
            }
        }

        if (state.cmd_send_power_restore) {
            state.cmd_send_power_restore = false;
            if (strlen(admin_id) > 0) {
                sendMessage(token, admin_id, "⚡ Світло увімкнули! Домашній Wi-Fi відновлено, шкатулка онлайн.");
            }
        }

        uint32_t now = millis();
        if (now - last_poll_ms < Config::TG_POLL_MS) return;
        last_poll_ms = now;

        state.tls_in_flight = true;
        HTTPClient https;
        https.setConnectTimeout(3000);
        https.setTimeout(3000);
        https.setReuse(true);

        char url_buf[180];
        if (!cold_boot_synced) {
            // При першому запуску беремо тільки найостаннішу подію (offset=-1), щоб пропустити стару чергу
            snprintf(url_buf, sizeof(url_buf), "https://api.telegram.org/bot%s/getUpdates?limit=1&timeout=0&offset=-1", token);
        } else {
            snprintf(url_buf, sizeof(url_buf), "https://api.telegram.org/bot%s/getUpdates?limit=1&timeout=0&offset=%lld",
                     token, (long long)(last_update_id + 1));
        }

        if (https.begin(client, url_buf)) {
            int httpCode = https.GET();
            if (httpCode == 200) {
                String payload = https.getString();
                state.tls_in_flight = false;
                if (payload.indexOf("\"update_id\":") != -1) {
                    handleIncomingPayload(payload, token, admin_id, girl_id);
                } else {
                    cold_boot_synced = true; // Черга порожня, синхронізацію завершено
                }
            } else {
                state.tls_in_flight = false;
            }
            https.end();
        } else {
            state.tls_in_flight = false;
        }
    }
};