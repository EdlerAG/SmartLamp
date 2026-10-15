#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "cloud/CloudOtaService.h"
#include "cloud/TgJsonUtils.h"
#include "cloud/TgKeyboards.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class TelegramService : public IService {
private:
    AppState&        state;
    WiFiClientSecure client;
    HTTPClient       https;

    int64_t  last_update_id   = 0;
    uint32_t last_poll_ms     = 0;
    bool     cold_boot_synced = false;

    bool sendMessage(const char* token, const char* chat_id, const String& text, const char* reply_markup = nullptr) {
        if (!state.wifi_connected || strlen(token) < 10 || strlen(chat_id) == 0) return false;
        state.tls_in_flight = true;
        String url = String("https://api.telegram.org/bot") + token +
                     "/sendMessage?chat_id=" + chat_id +
                     "&text=" + TgJsonUtils::urlEncode(text);
        if (reply_markup != nullptr) url += "&reply_markup=" + TgJsonUtils::urlEncode(String(reply_markup));

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
        String url = String("https://api.telegram.org/bot") + token +
                     "/editMessageText?chat_id=" + chat_id +
                     "&message_id=" + msg_id +
                     "&text=" + TgJsonUtils::urlEncode(text);
        if (reply_markup != nullptr) url += "&reply_markup=" + TgJsonUtils::urlEncode(String(reply_markup));

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
        String url = String("https://api.telegram.org/bot") + token +
                     "/answerCallbackQuery?callback_query_id=" + cb_id +
                     "&text=" + TgJsonUtils::urlEncode(toast_text);
        if (https.begin(client, url)) {
            https.GET();
            https.end();
        }
        state.tls_in_flight = false;
    }

    String resolveTelegramFileUrl(const char* token, const String& file_id) {
        state.tls_in_flight = true;
        String api_url = String("https://api.telegram.org/bot") + token + "/getFile?file_id=" + file_id;
        String file_path = "";
        if (https.begin(client, api_url)) {
            if (https.GET() == 200) {
                file_path = TgJsonUtils::extractString(https.getString(), "file_path");
            }
            https.end();
        }
        state.tls_in_flight = false;
        if (file_path.length() == 0) return "";
        return String("https://api.telegram.org/file/bot") + token + "/" + file_path;
    }

    void handleCallbackQuery(const String& payload, int cb_pos, const char* token) {
        String cb_id = TgJsonUtils::extractString(payload, "id", cb_pos);
        String data  = TgJsonUtils::extractString(payload, "data", cb_pos);

        int msg_pos = payload.indexOf("\"message\":", cb_pos);
        String msg_id  = (msg_pos != -1) ? TgJsonUtils::extractIntStr(payload, "message_id", msg_pos) : "";
        int chat_pos   = (msg_pos != -1) ? payload.indexOf("\"chat\":", msg_pos) : -1;
        String chat_id = (chat_pos != -1) ? TgJsonUtils::extractIntStr(payload, "id", chat_pos) : "";

        if (data.startsWith("mode:")) {
            uint8_t m = (uint8_t)data.substring(5).toInt();
            CommandRouter::setLightMode(state, m);
            answerCallback(token, cb_id, String("Увімкнено: ") + CommandRouter::getModeNameUa(m));
            String header = "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ\nЗараз світиться: " +
                            String(CommandRouter::getModeNameUa(state.light_mode)) +
                            "\nЯскравість: " + String(state.getLedBrightnessPercent()) + "%";
            String kb = TgKeyboards::buildModesInline(state);
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        } else if (data.startsWith("brt:")) {
            String val = data.substring(4);
            if (val == "auto") {
                CommandRouter::setBrightnessPreset(state, true);
                answerCallback(token, cb_id, "🤖 Авто-яскравість увімкнено!");
            } else {
                uint8_t idx = (uint8_t)val.toInt();
                CommandRouter::setBrightnessPreset(state, false, idx);
                answerCallback(token, cb_id, CommandRouter::getBrightnessPresetName(false, idx));
            }
            String header = "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ\nРежим: " +
                            String(CommandRouter::getBrightnessPresetName(state.auto_brt, state.manual_brt_idx)) +
                            "\nПотужність LED: " + String(state.getLedBrightnessPercent()) + "%";
            String kb = TgKeyboards::buildBrightnessInline(state);
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        } else if (data.startsWith("slp:")) {
            uint16_t mins = (uint16_t)data.substring(4).toInt();
            CommandRouter::setSleepTimer(state, mins);
            answerCallback(token, cb_id, mins > 0 ? ("🌙 Таймер сну на " + String(mins) + " хв!") : "Таймер вимкнено");
            String header = (mins > 0) ? ("⏳ Таймер сну запущено на " + String(mins) + " хв!") : "⏳ Таймер сну вимкнено.";
            String kb = TgKeyboards::buildSleepInline();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), header, kb.c_str());
        } else if (data == "menu:brt") {
            answerCallback(token, cb_id, "Меню яскравості");
            String kb = TgKeyboards::buildBrightnessInline(state);
            editMessage(token, chat_id.c_str(), msg_id.c_str(), "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ", kb.c_str());
        } else if (data == "menu:sleep") {
            answerCallback(token, cb_id, "Таймер сну");
            String kb = TgKeyboards::buildSleepInline();
            editMessage(token, chat_id.c_str(), msg_id.c_str(), "⏳ ТАЙМЕР СНУ «ЗАХІД СОНЦЯ»", kb.c_str());
        } else if (data == "menu:modes") {
            answerCallback(token, cb_id, "Список режимів");
            String kb = TgKeyboards::buildModesInline(state);
            editMessage(token, chat_id.c_str(), msg_id.c_str(), "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ", kb.c_str());
        }
    }

    void handleIncomingPayload(const String& payload, const char* token, const char* admin_id, const char* girl_id) {
        int upd_idx = payload.indexOf("\"update_id\":");
        if (upd_idx == -1) return;

        int64_t upd_id = strtoll(payload.c_str() + upd_idx + 12, nullptr, 10);
        if (upd_id <= 0) return;

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
        String chat_id = (chat_idx != -1) ? TgJsonUtils::extractIntStr(payload, "id", chat_idx) : "";
        if (chat_id.length() == 0) return;

        bool is_admin = (strlen(admin_id) > 0 && chat_id == String(admin_id));
        bool is_girl  = (strlen(girl_id) > 0  && chat_id == String(girl_id));
        const char* active_kb = is_admin ? TgKeyboards::ADMIN_REPLY_KB : (is_girl ? TgKeyboards::GIRL_REPLY_KB : TgKeyboards::ADMIN_REPLY_KB);

        int doc_idx = payload.indexOf("\"document\":", msg_idx);
        if (doc_idx != -1) {
            if (!is_admin) {
                sendMessage(token, chat_id.c_str(), "⛔ Тільки адміністратор може оновлювати прошивку шкатулки.");
                return;
            }
            String file_name = TgJsonUtils::extractString(payload, "file_name", doc_idx);
            String file_id   = TgJsonUtils::extractString(payload, "file_id", doc_idx);
            if (!file_name.endsWith(".bin")) {
                sendMessage(token, chat_id.c_str(), "⚠️ Надішліть файл із розширенням *.bin (firmware.bin).");
                return;
            }
            sendMessage(token, chat_id.c_str(), "🚀 Прийнято `" + file_name + "`! Починаю турбо-оновлення...");
            String dl_url = resolveTelegramFileUrl(token, file_id);
            String err = "";
            if (dl_url.length() > 0 && CloudOtaService::flashFromUrl(state, dl_url, err)) {
                sendMessage(token, chat_id.c_str(), "🎉 Прошивку встановлено! Перезавантажую шкатулку...");
                vTaskDelay(pdMS_TO_TICKS(600));
                ESP.restart();
            } else {
                sendMessage(token, chat_id.c_str(), "❌ Помилка OTA: " + err);
            }
            return;
        }

        String raw_name = TgJsonUtils::extractString(payload, "first_name", msg_idx);
        if (raw_name.length() == 0) raw_name = "Telegram";
        String raw_text = TgJsonUtils::extractString(payload, "text", msg_idx);
        if (raw_text.length() == 0) return;

        char sender_utf8[28], text_utf8[96];
        TgJsonUtils::decodeJsonString(raw_name, sender_utf8, sizeof(sender_utf8));
        TgJsonUtils::decodeJsonString(raw_text, text_utf8, sizeof(text_utf8));
        String msg = String(text_utf8);
        msg.trim();

        if (msg.startsWith("/ota ") && is_admin) {
            String url = msg.substring(5); url.trim();
            sendMessage(token, chat_id.c_str(), "🌐 Завантажую прошивку за посиланням:\n" + url);
            String err = "";
            if (CloudOtaService::flashFromUrl(state, url, err)) {
                sendMessage(token, chat_id.c_str(), "🎉 Прошивку встановлено! Перезапуск...");
                vTaskDelay(pdMS_TO_TICKS(600));
                ESP.restart();
            } else {
                sendMessage(token, chat_id.c_str(), "❌ Помилка OTA: " + err);
            }
            return;
        }

        if (msg.equalsIgnoreCase("/rollback") && is_admin) {
            String err = "";
            if (CloudOtaService::rollbackToPrevious(err)) {
                sendMessage(token, chat_id.c_str(), "⏪ Відкат на попередню прошивку виконано! Перезапуск...");
                vTaskDelay(pdMS_TO_TICKS(600));
                ESP.restart();
            } else {
                sendMessage(token, chat_id.c_str(), "❌ Відкат неможливий: " + err);
            }
            return;
        }

        // ЗАХИЩЕНА ПРИВ'ЯЗКА РОЛЕЙ: спрацьовує лише якщо слот вільний або команду дає поточний власник
        if (msg.equalsIgnoreCase("/iam_admin")) {
            if (strlen(admin_id) == 0 || is_admin) {
                state.setTelegramConfig(nullptr, chat_id.c_str(), nullptr);
                state.tg_config_dirty = true;
                sendMessage(token, chat_id.c_str(), "👑 Акаунт Ярослава (Admin) підтверджено!", TgKeyboards::ADMIN_REPLY_KB);
            } else {
                sendMessage(token, chat_id.c_str(), "⛔ Адміністратор вже прив'язаний. Змінити ID можна лише через локальну Web-панель шкатулки.");
            }
            return;
        }

        if (msg.equalsIgnoreCase("/iam_nastia") || msg.equalsIgnoreCase("/iam_girl")) {
            if (strlen(girl_id) == 0 || is_girl || is_admin) {
                state.setTelegramConfig(nullptr, nullptr, chat_id.c_str());
                state.tg_config_dirty = true;
                sendMessage(token, chat_id.c_str(), "🌸 Акаунт власниці шкатулки прив'язано!", TgKeyboards::GIRL_REPLY_KB);
            } else {
                sendMessage(token, chat_id.c_str(), "⛔ Акаунт власниці вже захищено.");
            }
            return;
        }

        if (msg.startsWith("/start") || msg.startsWith("/help") || msg.startsWith("/menu")) {
            sendMessage(token, chat_id.c_str(), TgKeyboards::buildStatusReport(state), active_kb);
            return;
        }

        if (msg.startsWith("/modes") || msg.indexOf("Режими") != -1 || msg.indexOf("Свічка") != -1) {
            String inline_kb = TgKeyboards::buildModesInline(state);
            sendMessage(token, chat_id.c_str(), "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ", inline_kb.c_str());
            return;
        }

        if (msg.startsWith("/brt") || msg.indexOf("Яскравість") != -1) {
            String inline_kb = TgKeyboards::buildBrightnessInline(state);
            sendMessage(token, chat_id.c_str(), "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ", inline_kb.c_str());
            return;
        }

        if (msg.startsWith("/sleep") || msg.indexOf("Таймер") != -1) {
            String inline_kb = TgKeyboards::buildSleepInline();
            sendMessage(token, chat_id.c_str(), "⏳ ТАЙМЕР СНУ «ЗАХІД СОНЦЯ»", inline_kb.c_str());
            return;
        }

        if (msg.startsWith("/status") || msg.indexOf("Стан") != -1) {
            String inline_kb = TgKeyboards::buildModesInline(state);
            sendMessage(token, chat_id.c_str(), TgKeyboards::buildStatusReport(state), inline_kb.c_str());
            return;
        }

        if (msg.indexOf("Вогник") != -1 || msg.startsWith("/fire")) {
            CommandRouter::triggerReaction(state, NOTIFY_FIRE, sender_utf8, "FIRE VIBE! TSE PIDPAL!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "🔥 Настя кинула тобі ВОГНИК!", TgKeyboards::ADMIN_REPLY_KB);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "🔥 Ярослав запустив ВОГНИК на шкатулці!", TgKeyboards::GIRL_REPLY_KB);
            sendMessage(token, chat_id.c_str(), "🔥 Вогняний спалах запущено!", active_kb);
            return;
        }
        if (msg.indexOf("Блискавка") != -1 || msg.startsWith("/zap")) {
            CommandRouter::triggerReaction(state, NOTIFY_LIGHTNING, sender_utf8, "KUS-ZAP! BZZT!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "⚡ Настя бахнула тебе БЛИСКАВКОЮ!", TgKeyboards::ADMIN_REPLY_KB);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "⚡ Ярослав надіслав БЛИСКАВКУ на шкатулку!", TgKeyboards::GIRL_REPLY_KB);
            sendMessage(token, chat_id.c_str(), "⚡ Блискавка запущена!", active_kb);
            return;
        }
        if (msg.indexOf("Кіт") != -1 || msg.indexOf("Кота") != -1 || msg.startsWith("/cat")) {
            CommandRouter::triggerReaction(state, NOTIFY_CAT, sender_utf8, "MEOW! TYTS-KUS =^._.^=");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "🐱 Настя надіслала тобі МЕМНОГО КОТА!", TgKeyboards::ADMIN_REPLY_KB);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "🐱 Ярослав випустив МЕМНОГО КОТА на екран!", TgKeyboards::GIRL_REPLY_KB);
            sendMessage(token, chat_id.c_str(), "🐱 Мемний кіт підморгує на екрані!", active_kb);
            return;
        }
        if (msg.indexOf("Пороблено") != -1 || msg.startsWith("/ufo")) {
            CommandRouter::triggerReaction(state, NOTIFY_UFO, sender_utf8, "UFO BEAM! POROBLENO!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "👽 Настя запустила НЛО («Пороблено!»)!", TgKeyboards::ADMIN_REPLY_KB);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "👽 Ярослав запустив НЛО на шкатулці!", TgKeyboards::GIRL_REPLY_KB);
            sendMessage(token, chat_id.c_str(), "👽 Прибульці прилетіли на екран!", active_kb);
            return;
        }
        if (msg.indexOf("Корона") != -1 || msg.startsWith("/crown")) {
            CommandRouter::triggerReaction(state, NOTIFY_CROWN, sender_utf8, "QUEEN VIBE! KRASH!");
            if (is_girl && strlen(admin_id) > 0) sendMessage(token, admin_id, "👑 Настя активувала КОРОНУ!", TgKeyboards::ADMIN_REPLY_KB);
            else if (is_admin && strlen(girl_id) > 0) sendMessage(token, girl_id, "👑 Ярослав надіслав тобі КОРОНУ!", TgKeyboards::GIRL_REPLY_KB);
            sendMessage(token, chat_id.c_str(), "👑 Золота корона сяє на екрані!", active_kb);
            return;
        }

        if (is_girl) {
            if (strlen(admin_id) > 0) {
                sendMessage(token, admin_id, "💬 Повідомлення від Насті:\n«" + msg + "»", TgKeyboards::ADMIN_REPLY_KB);
                sendMessage(token, chat_id.c_str(), "📨 Відповідь доставлена Ярославу!", TgKeyboards::GIRL_REPLY_KB);
            } else {
                sendMessage(token, chat_id.c_str(), "⚠️ Ярослав ще не активував /iam_admin");
            }
            state.triggerLedNotification(NOTIFY_CAT, false);
        } else {
            CommandRouter::pushMessage(state, sender_utf8, text_utf8);
            sendMessage(token, chat_id.c_str(), "📩 Виведено на екран та збережено в Inbox!", active_kb);
            if (strlen(girl_id) > 0 && chat_id != String(girl_id)) {
                sendMessage(token, girl_id, "✨ Нове повідомлення на шкатулці від " + String(sender_utf8) + ":\n«" + msg + "»", TgKeyboards::GIRL_REPLY_KB);
            }
        }
    }

public:
    TelegramService(AppState& st) : state(st) {}

    const char* getName() const override { return "Telegram"; }

    void init() override {
        client.setInsecure();
        client.setTimeout(4);
        https.setConnectTimeout(3000);
        https.setTimeout(3000);
        https.setReuse(true);
    }

    void update() override {
        if (!state.wifi_connected || state.ap_mode_active || state.ota_in_progress) return;

        char ota_url_buf[160];
        if (state.popPendingOtaUrl(ota_url_buf, sizeof(ota_url_buf))) {
            String err = "";
            if (CloudOtaService::flashFromUrl(state, String(ota_url_buf), err)) {
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP.restart();
            }
        }

        char token[64], admin_id[20], girl_id[20];
        state.getTelegramConfig(token, sizeof(token), admin_id, sizeof(admin_id), girl_id, sizeof(girl_id));
        if (strlen(token) < 15) return;

        if (state.cmd_send_tg_ack) {
            if (strlen(admin_id) > 0) {
                if (sendMessage(token, admin_id, "✅ Настя торкнулася кришки шкатулки — повідомлення прочитано! 🔥")) {
                    state.cmd_send_tg_ack = false;
                }
            } else state.cmd_send_tg_ack = false;
        }

        if (state.cmd_send_meme_ping > 0) {
            uint8_t ping_type = state.cmd_send_meme_ping;
            if (strlen(admin_id) > 0) {
                const char* ping_txt = "🐱 Настя натиснула кнопку на шкатулці: МЕМНИЙ КУСЬ (=^._.^=)!";
                if (ping_type == NOTIFY_FIRE)      ping_txt = "🔥 Настя кинула тобі ВОГНИК зі шкатулки!";
                if (ping_type == NOTIFY_LIGHTNING) ping_txt = "⚡ Настя бахнула БЛИСКАВКОЮ зі шкатулки!";
                if (ping_type == NOTIFY_UFO)       ping_txt = "👽 Настя натиснула кнопку: «Пороблено! UFO Beam»!";
                if (ping_type == NOTIFY_CROWN)     ping_txt = "👑 Настя передає КОРОЛІВСЬКИЙ ПРИВІТ зі шкатулки!";
                if (sendMessage(token, admin_id, ping_txt)) state.cmd_send_meme_ping = 0;
            } else state.cmd_send_meme_ping = 0;
        }

        if (state.cmd_send_power_restore) {
            state.cmd_send_power_restore = false;
            if (strlen(admin_id) > 0) {
                sendMessage(token, admin_id, "⚡ Світло увімкнули! Wi-Fi відновлено (" + CloudOtaService::getPartitionInfo() + ").");
            }
        }

        uint32_t now = millis();
        if (now - last_poll_ms < Config::TG_POLL_MS) return;
        last_poll_ms = now;

        state.tls_in_flight = true;
        char url_buf[180];
        if (!cold_boot_synced) {
            snprintf(url_buf, sizeof(url_buf), "https://api.telegram.org/bot%s/getUpdates?limit=1&timeout=0&offset=-1", token);
        } else {
            snprintf(url_buf, sizeof(url_buf), "https://api.telegram.org/bot%s/getUpdates?limit=1&timeout=0&offset=%lld",
                     token, (long long)(last_update_id + 1));
        }

        if (https.begin(client, url_buf)) {
            int httpCode = https.GET();
            if (httpCode == 200) {
                String payload = https.getString();
                https.end();
                state.tls_in_flight = false;
                if (payload.indexOf("\"update_id\":") != -1) {
                    handleIncomingPayload(payload, token, admin_id, girl_id);
                } else {
                    cold_boot_synced = true;
                }
            } else {
                https.end();
                state.tls_in_flight = false;
            }
        } else {
            state.tls_in_flight = false;
        }
    }
};