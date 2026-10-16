#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "cloud/CloudOtaService.h"
#include "cloud/TgJsonUtils.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class OracleCloudService : public IService {
private:
    AppState&        state;
    WiFiClient       net_client;
    WiFiClientSecure sec_client;
    HTTPClient       http;
    String           base_server_url = "";
    String           sync_endpoint   = "";
    String           api_key         = "";

    void refreshConfig() {
        char u_buf[128], k_buf[64];
        state.getOracleConfig(u_buf, sizeof(u_buf), k_buf, sizeof(k_buf));
        String base = String(u_buf);
        base.trim();
        while (base.endsWith("/")) base.remove(base.length() - 1);

        api_key = String(k_buf);
        api_key.trim();

        if (base.length() < 10 || base.indexOf("127.0.0.1") != -1 ||
            base.indexOf("localhost") != -1 || base.indexOf("YOUR_ORACLE_IP") != -1) {
            base_server_url = "";
            sync_endpoint   = "";
            return;
        }

        int api_idx = base.indexOf("/api/");
        if (api_idx != -1) {
            base_server_url = base.substring(0, api_idx);
            sync_endpoint   = base;
        } else {
            base_server_url = base;
            sync_endpoint   = base + "/api/v1/box/sync";
        }
    }

    String normalizeMediaUrl(const String& raw_url) const {
        if (raw_url.length() == 0) return "";
        if (raw_url.startsWith("/")) {
            return base_server_url + raw_url;
        }
        if (raw_url.indexOf("127.0.0.1") != -1 || raw_url.indexOf("localhost") != -1 || raw_url.indexOf("YOUR_ORACLE_IP") != -1) {
            int path_idx = raw_url.indexOf("/api/");
            if (path_idx != -1 && base_server_url.length() > 0) {
                return base_server_url + raw_url.substring(path_idx);
            }
        }
        return raw_url;
    }

    bool downloadRgb565ToCanvas(const String& raw_url) {
        if (!state.shared_canvas_buf || !state.wifi_connected) return false;
        String url = normalizeMediaUrl(raw_url);

        HTTPClient img_http;
        WiFiClient img_client;
        WiFiClientSecure img_sec;

        img_http.setConnectTimeout(3500);
        img_http.setTimeout(6000);

        state.tls_in_flight = true;
        bool begun = false;
        if (url.startsWith("https://")) {
            img_sec.setInsecure();
            img_sec.setNoDelay(true);
            begun = img_http.begin(img_sec, url);
        } else {
            img_client.setNoDelay(true);
            begun = img_http.begin(img_client, url);
        }

        if (!begun) {
            state.tls_in_flight = false;
            return false;
        }

        int code = img_http.GET();
        int content_len = img_http.getSize();
        if (code != HTTP_CODE_OK || (content_len > 0 && content_len != 115200)) {
            img_http.end();
            state.tls_in_flight = false;
            return false;
        }

        // Забороняємо Core 1 малювати у буфер на час прийому кадру
        state.canvas_loading = true;
        WiFiClient* stream = img_http.getStreamPtr();
        size_t total_read = 0;
        uint32_t start_ms = millis();

        while (img_http.connected() && total_read < 115200 && (millis() - start_ms < 6000)) {
            size_t avail = stream->available();
            if (avail > 0) {
                size_t to_read = min(avail, (size_t)(115200 - total_read));
                int r = stream->readBytes(state.shared_canvas_buf + total_read, to_read);
                if (r > 0) total_read += r;
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        img_http.end();
        state.tls_in_flight = false;

        if (total_read == 115200) {
            state.custom_frame_until = millis() + 10000;
            state.canvas_loading     = false;
            state.triggerLedNotification(NOTIFY_CYAN_WAVE, false);
            state.registerActivity();
            return true;
        }

        state.custom_frame_until = 0;
        state.canvas_loading     = false;
        state.registerActivity();
        return false;
    }

    void processServerResponse(const String& json) {
        state.oracle_manages_bot  = (json.indexOf("\"bot_managed\":true") != -1);
        state.has_more_cloud_cmds = (json.indexOf("\"has_more\":true") != -1);

        if (json.indexOf("\"has_cmd\":true") == -1) return;
        state.registerNetActivity();

        String mode_str = TgJsonUtils::extractIntStr(json, "set_mode");
        if (mode_str.length() > 0) {
            CommandRouter::setLightMode(state, (uint8_t)mode_str.toInt());
        }

        String brt_str = TgJsonUtils::extractString(json, "set_brt");
        if (brt_str.length() > 0) {
            if (brt_str == "auto") CommandRouter::setBrightnessPreset(state, true);
            else CommandRouter::setBrightnessPreset(state, false, (uint8_t)brt_str.toInt());
        }

        String sleep_str = TgJsonUtils::extractIntStr(json, "sleep");
        if (sleep_str.length() > 0) {
            CommandRouter::setSleepTimer(state, (uint16_t)sleep_str.toInt());
        }

        String react_str = TgJsonUtils::extractIntStr(json, "react");
        if (react_str.length() > 0) {
            uint8_t r = (uint8_t)react_str.toInt();
            if (r >= NOTIFY_FIRE && r <= NOTIFY_CROWN) {
                CommandRouter::triggerReaction(state, (NotifyLedType)r, "Oracle Cloud", "CLOUD VIBE!");
            }
        }

        String msg = TgJsonUtils::extractString(json, "msg");
        if (msg.length() > 0) {
            String sender = TgJsonUtils::extractString(json, "sender");
            if (sender.length() == 0) sender = "Oracle AI";
            char s_utf8[28], m_utf8[96];
            TgJsonUtils::decodeJsonString(sender, s_utf8, sizeof(s_utf8));
            TgJsonUtils::decodeJsonString(msg, m_utf8, sizeof(m_utf8));
            CommandRouter::pushMessage(state, s_utf8, m_utf8);
        }

        String play_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "play_url"));
        if (play_url.length() > 8) {
            state.setPendingAudioUrl(play_url.c_str());
        }

        String img_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "img_url"));
        if (img_url.length() > 8) {
            state.setPendingImageUrl(img_url.c_str());
        }

        String ota_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "ota_url"));
        if (ota_url.length() > 10) {
            state.setPendingOtaUrl(ota_url.c_str());
        }
    }

public:
    OracleCloudService(AppState& st) : state(st) {}

    const char* getName() const override { return "OracleCloud"; }

    bool isEnabled() const override {
        char u_buf[128];
        state.getOracleConfig(u_buf, sizeof(u_buf), nullptr, 0);
        if (strlen(u_buf) < 10) return false;
        if (strstr(u_buf, "127.0.0.1") != nullptr ||
            strstr(u_buf, "localhost") != nullptr ||
            strstr(u_buf, "YOUR_ORACLE_IP") != nullptr) {
            return false;
        }
        return true;
    }

    uint32_t getIntervalMs() const override {
        if (state.has_more_cloud_cmds) return 300;
        return Config::ORACLE_SYNC_MS;
    }

    void init() override {
        refreshConfig();
        net_client.setNoDelay(true);
        sec_client.setInsecure();
        sec_client.setNoDelay(true);
        http.setConnectTimeout(2500);
        http.setTimeout(2500);
        http.setReuse(true);
    }

    void update() override {
        if (!state.wifi_connected || state.ap_mode_active || state.ota_in_progress) return;

        // Виконуємо відкладене хмарне OTA-оновлення незалежно від TelegramService
        char ota_url_buf[160];
        if (state.popPendingOtaUrl(ota_url_buf, sizeof(ota_url_buf))) {
            String err = "";
            if (CloudOtaService::flashFromUrl(state, String(ota_url_buf), err)) {
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP.restart();
            }
        }

        refreshConfig();
        if (sync_endpoint.length() == 0) return;

        char img_url_buf[160];
        if (state.popPendingImageUrl(img_url_buf, sizeof(img_url_buf))) {
            downloadRgb565ToCanvas(String(img_url_buf));
        }

        // Якщо зараз грає аудіо по I2S і немає термінового пінга з кнопок — не навантажуємо канал
        if (state.audio_playing && !state.cmd_send_tg_ack && state.cmd_send_meme_ping == 0 && !state.has_more_cloud_cmds) {
            return;
        }

        char status_json[460];
        CommandRouter::buildStatusJson(state, status_json, sizeof(status_json));

        state.tls_in_flight = true;
        bool begun = sync_endpoint.startsWith("https://")
                     ? http.begin(sec_client, sync_endpoint)
                     : http.begin(net_client, sync_endpoint);

        if (begun) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-Device-Key", api_key);

            int code = http.POST((uint8_t*)status_json, strlen(status_json));
            if (code == 200) {
                state.oracle_online = true;
                String resp = http.getString();
                http.end();
                state.tls_in_flight = false;
                processServerResponse(resp);
                return;
            }
            state.oracle_online       = false;
            state.oracle_manages_bot  = false;
            state.has_more_cloud_cmds = false;
            http.end();
        } else {
            state.oracle_online       = false;
            state.oracle_manages_bot  = false;
            state.has_more_cloud_cmds = false;
        }
        state.tls_in_flight = false;
    }
};