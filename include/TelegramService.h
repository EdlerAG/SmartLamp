#pragma once
#include "IService.h"
#include "Config.h"
#include "AppState.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class TelegramService : public IService {
private:
    AppState& state;
    WiFiClientSecure client;
    int32_t  last_update_id = 0;
    uint32_t last_poll_ms   = 0;

    // Зручні інтерактивні кнопки внизу Telegram для Тебе (Адмін/Відправник)
    const char* ADMIN_KEYBOARD = R"({"keyboard":[[{"text":"📊 Клімат та Стан"},{"text":"💖 Пульс сердечка"}],[{"text":"🎨 Змінити ефект"},{"text":"🌙 Нічник / Світло"}]],"resize_keyboard":true})";

    // Зручні кнопки внизу Telegram для Неї (Власниця шкатулки)
    const char* GIRL_KEYBOARD  = R"({"keyboard":[[{"text":"💌 Надіслати сердечко Ярославу"}],[{"text":"🎨 Змінити світло"},{"text":"🌙 Вимкнути / Увімкнути"}],[{"text":"🌡 Погода в кімнаті"}]],"resize_keyboard":true})";

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
        if (strlen(token) < 10 || strlen(chat_id) == 0) return false;

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
            int code = https.GET();
            ok = (code == 200);
            https.end();
        }
        return ok;
    }

    String buildClimateReport(bool detailed_admin) {
        const char* m_names[6] = {"Warm Cozy", "Rainbow", "Aurora", "Cyber", "Fire", "OFF"};
        char buf[280];
        if (detailed_admin) {
            uint32_t up_min = millis() / 60000;
            snprintf(buf, sizeof(buf),
                     "📊 Звіт шкатулки Насті:\n"
                     "🌡 Температура: %.1f °C\n"
                     "💧 Вологість: %.0f %%\n"
                     "🧭 Барометр: %.0f мм рт.ст.\n"
                     "☀️ Світло в кімнаті: %d %%\n"
                     "🎨 Режим лампи: %s (LED: %d)\n"
                     "📶 Wi-Fi RSSI: %d dBm | Uptime: %lu хв",
                     state.temperature, state.humidity, state.pressure,
                     state.light_percent, m_names[state.light_mode % 6],
                     state.current_led_brt, (int)state.wifi_rssi, (unsigned long)up_min);
        } else {
            const char* forecast = (state.pressure < 742.0f) ? "Можливий дощ 🌧" :
                                  ((state.pressure > 760.0f) ? "Ясно та сонячно ☀️" : "Комфортна погода ✨");
            snprintf(buf, sizeof(buf),
                     "🌸 Клімат у твоїй кімнаті:\n"
                     "🌡 Температура: %.1f °C\n"
                     "💧 Вологість: %.0f %%\n"
                     "🧭 Тиск: %.0f мм (%s)",
                     state.temperature, state.humidity, state.pressure, forecast);
        }
        return String(buf);
    }

    void handleIncomingPayload(const String& payload, const char* token, const char* admin_id, const char* girl_id) {
        int upd_idx = payload.indexOf("\"update_id\":");
        if (upd_idx == -1) return;

        int32_t upd_id = payload.substring(upd_idx + 12).toInt();
        if (upd_id <= 0) return;
        last_update_id = upd_id;

        int msg_idx = payload.indexOf("\"message\":", upd_idx);
        if (msg_idx == -1) return;

        // Витягуємо chat_id відправника
        String chat_id = "";
        int chat_idx = payload.indexOf("\"chat\":", msg_idx);
        if (chat_idx != -1) {
            int id_idx = payload.indexOf("\"id\":", chat_idx);
            if (id_idx != -1) {
                id_idx += 5;
                int id_end = id_idx;
                while (id_end < (int)payload.length() && (isdigit(payload[id_end]) || payload[id_end] == '-')) {
                    id_end++;
                }
                chat_id = payload.substring(id_idx, id_end);
            }
        }
        if (chat_id.length() == 0) return;

        String raw_name = extractJsonString(payload, "first_name", msg_idx);
        if (raw_name.length() == 0) raw_name = "Telegram";
        String raw_text = extractJsonString(payload, "text", msg_idx);
        if (raw_text.length() == 0) return;

        char sender_utf8[32], text_utf8[96];
        decodeJsonString(raw_name, sender_utf8, sizeof(sender_utf8));
        decodeJsonString(raw_text, text_utf8, sizeof(text_utf8));
        String msg = String(text_utf8);
        msg.trim();

        // --- 1. КОМАНДИ ШВИДКОЇ ПРИВ'ЯЗКИ РОЛЕЙ ---
        if (msg.equalsIgnoreCase("/iam_admin")) {
            state.setTelegramConfig(nullptr, chat_id.c_str(), nullptr);
            state.tg_config_dirty = true;
            sendMessage(token, chat_id.c_str(),
                        "👑 Твій акаунт прив'язано як АДМІНА (Ярослав)!\n\n"
                        "Тепер будь-який текст, який ти сюди напишеш, одразу з'явиться у Насті на шкатулці.",
                        ADMIN_KEYBOARD);
            return;
        }

        if (msg.equalsIgnoreCase("/iam_nastia") || msg.equalsIgnoreCase("/iam_girl")) {
            state.setTelegramConfig(nullptr, nullptr, chat_id.c_str());
            state.tg_config_dirty = true;
            sendMessage(token, chat_id.c_str(),
                        "🌸 Твій акаунт прив'язано як ВЛАСНИЦІ шкатулки!\n\n"
                        "Керуй світлом кнопками нижче, або напиши будь-який текст — і він полетить Ярославу!",
                        GIRL_KEYBOARD);
            return;
        }

        bool is_admin = (strlen(admin_id) > 0 && chat_id == String(admin_id));
        bool is_girl  = (strlen(girl_id) > 0  && chat_id == String(girl_id));

        if (msg.startsWith("/start") || msg.startsWith("/help")) {
            if (is_admin) {
                sendMessage(token, chat_id.c_str(), "👑 Панель Ярослава активна. Напиши повідомлення для шкатулки або натисни кнопку:", ADMIN_KEYBOARD);
            } else if (is_girl) {
                sendMessage(token, chat_id.c_str(), "✨ Твоя розумна шкатулка онлайн! Напиши повідомлення для Ярослава або керуй підсвіткою:", GIRL_KEYBOARD);
            } else {
                String info = "✨ MagicBox Smart Bot!\nВаш Telegram ID: " + chat_id + "\n\n"
                              "Для прив'язки ролі натисніть:\n"
                              "👉 /iam_admin — Ярослав (писати на шкатулку)\n"
                              "👉 /iam_nastia — Настя (власниця шкатулки)";
                sendMessage(token, chat_id.c_str(), info);
            }
            return;
        }

        // --- 2. ОБРОБКА КОМАНД ТА КНОПОК ---
        if (msg.startsWith("/status") || msg.indexOf("Клімат") != -1 || msg.indexOf("Погода") != -1) {
            sendMessage(token, chat_id.c_str(), buildClimateReport(is_admin), is_admin ? ADMIN_KEYBOARD : (is_girl ? GIRL_KEYBOARD : nullptr));
            return;
        }

        if (msg.startsWith("/mode") || msg.indexOf("Змінити ефект") != -1 || msg.indexOf("Змінити світло") != -1) {
            state.sendEvent(EventType::BTN_TOP_SHORT);
            sendMessage(token, chat_id.c_str(), "🎨 Ефект підсвітки перемкнено!");
            return;
        }

        if (msg.startsWith("/off") || msg.startsWith("/on") || msg.indexOf("Нічник") != -1 || msg.indexOf("Вимкнути") != -1) {
            state.light_mode = (state.light_mode == 5) ? 0 : 5;
            state.markSettingsDirty();
            sendMessage(token, chat_id.c_str(), (state.light_mode == 5) ? "🌙 Підсвітку вимкнено." : "☀️ Увімкнено тепле затишне світло.");
            return;
        }

        if (msg.indexOf("Пульс сердечка") != -1 || msg.startsWith("/pulse")) {
            state.triggerLedNotification(NOTIFY_LOVE_PULSE);
            sendMessage(token, chat_id.c_str(), "💖 На шкатулці запущено ніжний пульс сердечка!");
            return;
        }

        if (msg.indexOf("Надіслати сердечко") != -1 || msg.startsWith("/love")) {
            if (strlen(admin_id) > 0) {
                sendMessage(token, admin_id, "💖 Настя щойно надіслала тобі сердечко через бот шкатулки!");
            }
            state.triggerLedNotification(NOTIFY_LOVE_PULSE);
            sendMessage(token, chat_id.c_str(), "💌 Сердечко для Ярослава доставлено!");
            return;
        }

        // --- 3. ОБРОБКА ОСОБИСТИХ ТЕКСТОВИХ ПОВІДОМЛЕНЬ ЗА РОЛЯМИ ---
        if (is_girl) {
            // Якщо пише Настя -> пересилаємо повідомлення Ярославу в Telegram!
            if (strlen(admin_id) > 0) {
                String fwd = "🌸 Повідомлення від Насті:\n«" + msg + "»";
                sendMessage(token, admin_id, fwd, ADMIN_KEYBOARD);
                sendMessage(token, chat_id.c_str(), "💌 Твоя відповідь полетіла Ярославу!", GIRL_KEYBOARD);
            } else {
                sendMessage(token, chat_id.c_str(), "⚠️ Ярослав ще не активував команду /iam_admin");
            }
            state.triggerLedNotification(NOTIFY_LOVE_PULSE);
        } else {
            // Якщо пишеш Ти (Admin) або будь-хто інший -> виводимо на екран шкатулки Насті!
            state.setTelegramData(sender_utf8, text_utf8);
            state.screen_mode = SCREEN_TELEGRAM;
            state.triggerLedNotification(NOTIFY_CYAN_WAVE);

            sendMessage(token, chat_id.c_str(), "📩 Виведено на екран шкатулки! Як тільки вона торкнеться кришки — я напишу.", is_admin ? ADMIN_KEYBOARD : nullptr);

            // Якщо в Насті прив'язаний акаунт — дублюємо сповіщення їй у телефон
            if (strlen(girl_id) > 0 && chat_id != String(girl_id)) {
                String notify_girl = "✨ Нове повідомлення на твоїй шкатулці від " + String(sender_utf8) + ":\n«" + msg + "»";
                sendMessage(token, girl_id, notify_girl, GIRL_KEYBOARD);
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

        // 1. Вона торкнулася кришки шкатулки -> Сповіщаємо Тебе, що повідомлення прочитане!
        if (state.cmd_send_tg_ack) {
            if (strlen(admin_id) > 0) {
                if (sendMessage(token, admin_id, "💖 Вона щойно торкнулася кришки шкатулки і прочитала твоє повідомлення!")) {
                    state.cmd_send_tg_ack = false;
                }
            } else {
                state.cmd_send_tg_ack = false;
            }
        }

        // 2. Вона затиснула праву кнопку на шкатулці (1 сек) -> "Фізичне сердечко" летить Тобі!
        if (state.cmd_send_love_ping) {
            if (strlen(admin_id) > 0) {
                if (sendMessage(token, admin_id, "💝 Настя затиснула кнопку на шкатулці і передає тобі привіт/сердечко!")) {
                    state.cmd_send_love_ping = false;
                }
            } else {
                state.cmd_send_love_ping = false;
            }
        }

        // 3. Якщо з'явилося світло після блекауту -> Сповіщаємо Тебе
        if (state.cmd_send_power_restore) {
            state.cmd_send_power_restore = false;
            if (strlen(admin_id) > 0) {
                sendMessage(token, admin_id, "⚡ Світло та домашній Wi-Fi відновилися! Шкатулка знову онлайн.");
            }
        }

        // 4. Опитування нових повідомлень
        uint32_t now = millis();
        if (now - last_poll_ms < Config::TG_POLL_MS) return;
        last_poll_ms = now;

        HTTPClient https;
        https.setConnectTimeout(3000);
        https.setTimeout(3000);
        https.setReuse(true);

        String url = String("https://api.telegram.org/bot") + token +
                     "/getUpdates?limit=1&timeout=0&offset=" + String(last_update_id + 1);

        if (https.begin(client, url)) {
            int httpCode = https.GET();
            if (httpCode == 200) {
                String payload = https.getString();
                if (payload.indexOf("\"update_id\":") != -1) {
                    handleIncomingPayload(payload, token, admin_id, girl_id);
                }
            }
            https.end();
        }
    }
};

