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

    void processServerResponse(const String& json) {
        if (json.indexOf("\"has_cmd\":true") == -1) return;

        String mode_str = TgJsonUtils::extractIntStr(json, "set_mode");
        if (mode_str.length() > 0) {
            CommandRouter::setLightMode(state, (uint8_t)mode_str.toInt());
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

        String ota_url = TgJsonUtils::extractString(json, "ota_url");
        if (ota_url.length() > 10) {
            state.setPendingOtaUrl(ota_url.c_str());
        }
    }

public:
    OracleCloudService(AppState& st) : state(st) {}

    const char* getName() const override { return "OracleCloud"; }

    // Вимкнений (0% CPU), якщо URL порожній або вказує на локальну заглушку 127.0.0.1
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
        http.setConnectTimeout(2500);
        http.setTimeout(2500);
        http.setReuse(true);
    }

    void update() override {
        if (!state.wifi_connected || state.ap_mode_active || state.ota_in_progress) return;

        char status_json[440];
        CommandRouter::buildStatusJson(state, status_json, sizeof(status_json));

        state.tls_in_flight = true;
        if (http.begin(net_client, Config::ORACLE_SERVER_URL)) {
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
            state.oracle_online = false;
            http.end();
        } else {
            state.oracle_online = false;
        }
        state.tls_in_flight = false;
    }
};