#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "cloud/TgJsonUtils.h"
#include <WiFi.h>
#include <HTTPClient.h>

class OracleCloudService : public IService {
private:
    AppState&  state;
    WiFiClient net_client;
    HTTPClient http;
    String     sync_endpoint = "";

    void buildEndpointUrl() {
        String base = String(Config::ORACLE_SERVER_URL);
        base.trim();
        while (base.endsWith("/")) base.remove(base.length() - 1);
        if (base.length() < 10) {
            sync_endpoint = "";
            return;
        }
        if (base.indexOf("/api/") == -1) sync_endpoint = base + "/api/v1/box/sync";
        else sync_endpoint = base;
    }

    // Потокове завантаження 240x240 RGB565 (115200 байт) прямо у відеобуфер екрана (0 байт зайвої RAM)
    bool downloadRgb565ToCanvas(const String& url) {
        if (!state.shared_canvas_buf || !state.wifi_connected) return false;

        HTTPClient img_http;
        WiFiClient img_client;
        img_client.setNoDelay(true);
        img_http.setConnectTimeout(3000);
        img_http.setTimeout(5000);

        state.tls_in_flight = true;
        if (!img_http.begin(img_client, url)) {
            state.tls_in_flight = false;
            return false;
        }

        int code = img_http.GET();
        if (code != HTTP_CODE_OK || img_http.getSize() != 115200) {
            img_http.end();
            state.tls_in_flight = false;
            return false;
        }

        // Блокуємо перезапис canvas на 10 секунд, поки показується наліпка/фото
        state.custom_frame_until = millis() + 10000;
        WiFiClient* stream = img_http.getStreamPtr();
        size_t total_read = 0;
        uint32_t start_ms = millis();

        while (img_http.connected() && total_read < 115200 && (millis() - start_ms < 5000)) {
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
            state.triggerLedNotification(NOTIFY_CYAN_WAVE, false);
            state.registerActivity();
            return true;
        }
        state.custom_frame_until = 0;
        return false;
    }

    void processServerResponse(const String& json) {
        state.oracle_manages_bot = (json.indexOf("\"bot_managed\":true") != -1);

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

        String play_url = TgJsonUtils::extractString(json, "play_url");
        if (play_url.length() > 8) {
            state.setPendingAudioUrl(play_url.c_str());
        }

        String img_url = TgJsonUtils::extractString(json, "img_url");
        if (img_url.length() > 8) {
            state.setPendingImageUrl(img_url.c_str());
        }

        String ota_url = TgJsonUtils::extractString(json, "ota_url");
        if (ota_url.length() > 10) {
            state.setPendingOtaUrl(ota_url.c_str());
        }
    }

public:
    OracleCloudService(AppState& st) : state(st) {}

    const char* getName() const override { return "OracleCloud"; }

    bool isEnabled() const override {
        const char* url = Config::ORACLE_SERVER_URL;
        if (strlen(url) < 12) return false;
        if (strstr(url, "127.0.0.1") != nullptr || strstr(url, "localhost") != nullptr) return false;
        return true;
    }

    uint32_t getIntervalMs() const override {
        return Config::ORACLE_SYNC_MS;
    }

    void init() override {
        buildEndpointUrl();
        net_client.setNoDelay(true);
        http.setConnectTimeout(2500);
        http.setTimeout(2500);
        http.setReuse(true);
    }

    void update() override {
        if (!state.wifi_connected || state.ap_mode_active || state.ota_in_progress) return;
        if (sync_endpoint.length() == 0) buildEndpointUrl();

        // Якщо в черзі є нова наліпка/фото — завантажуємо її у відеобуфер
        char img_url_buf[160];
        if (state.popPendingImageUrl(img_url_buf, sizeof(img_url_buf))) {
            downloadRgb565ToCanvas(String(img_url_buf));
        }

        char status_json[460];
        CommandRouter::buildStatusJson(state, status_json, sizeof(status_json));

        state.tls_in_flight = true;
        if (http.begin(net_client, sync_endpoint)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-Device-Key", Config::ORACLE_API_KEY);

            int code = http.POST((uint8_t*)status_json, strlen(status_json));
            if (code == 200) {
                state.oracle_online = true;
                String resp = http.getString();
                http.end();
                state.tls_in_flight = false;
                processServerResponse(resp);
                return;
            }
            state.oracle_online      = false;
            state.oracle_manages_bot = false;
            http.end();
        } else {
            state.oracle_online      = false;
            state.oracle_manages_bot = false;
        }
        state.tls_in_flight = false;
    }
};