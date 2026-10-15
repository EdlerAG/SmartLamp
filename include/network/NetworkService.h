#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "core/StorageService.h"
#include "network/WifiScanner.h"
#include "network/PortalServer.h"
#include "network/OtaManager.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <time.h>

class NetworkService : public IService {
private:
    enum class NetMode : uint8_t {
        IDLE,
        CONNECTING,
        CONNECTED,
        AP_PORTAL,
        BLACKOUT_ECO,
        BLACKOUT_PROBE
    };
    NetMode net_mode = NetMode::IDLE;

    AppState&      state;
    StorageService storage;
    WifiScanner    scanner;
    PortalServer   portal;
    OtaManager     ota;
    DNSServer      dnsServer;

    String   saved_ssid = "";
    String   saved_pass = "";
    uint32_t state_timer_ms = 0;
    uint32_t last_rtc_tick  = 0;
    uint32_t drop_start_ms  = 0;

    void waitForTlsIdle() {
        uint32_t start = millis();
        while (state.tls_in_flight && (millis() - start < 500)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    void startStaConnection() {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.setScanMethod(WIFI_FAST_SCAN);
        WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
        WiFi.setSleep(false);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

        portal.begin();
        state.wifi_sleeping  = false;
        state.ap_mode_active = false;
        state.setNetworkInfo(saved_ssid.c_str(), "Connecting...");
        state_timer_ms = millis();
        net_mode = NetMode::CONNECTING;
    }

    void enterApPortalMode() {
        state.wifi_connected = false;
        waitForTlsIdle();

        WiFi.persistent(false);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);

        if (!scanner.hasCachedResults()) {
            scanner.performFastPreScan();
        }

        WiFi.mode(WIFI_AP_STA);
        WiFi.setSleep(false);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        WiFi.softAP(Config::AP_SSID, NULL, 6, 0, 4);
        vTaskDelay(pdMS_TO_TICKS(25));

        dnsServer.setTTL(0);
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(53, "*", WiFi.softAPIP());
        portal.begin();

        state_timer_ms       = millis();
        state.ap_mode_active = true;
        state.ap_clients     = 0;
        state.wifi_sleeping  = false;
        state.setNetworkInfo(Config::AP_SSID, WiFi.softAPIP().toString().c_str());
        state.screen_mode = SCREEN_NETWORK;
        state.registerActivity();
        net_mode = NetMode::AP_PORTAL;
    }

    void onWifiConnected() {
        WiFi.setSleep(WIFI_PS_MIN_MODEM);
        state.wifi_connected = true;
        state.ap_mode_active = false;
        state.wifi_sleeping  = false;
        state.wifi_rssi      = WiFi.RSSI();
        state.setNetworkInfo(saved_ssid.c_str(), WiFi.localIP().toString().c_str());
        MDNS.begin(Config::DEVICE_NAME);
        ota.begin();
        portal.begin();

        if (state.had_blackout_drop) {
            state.had_blackout_drop = false;
            state.cmd_send_power_restore = true;
        }

        state.screen_mode = SCREEN_CLIMATE;
        state.registerActivity();
        drop_start_ms = 0;
        net_mode = NetMode::CONNECTED;
    }

    void enterBlackoutEcoMode() {
        state.wifi_connected = false;
        waitForTlsIdle();

        dnsServer.stop();
        portal.stop();
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        state.ap_mode_active = false;
        state.ap_clients     = 0;
        state.wifi_sleeping  = true;
        state.setNetworkInfo(saved_ssid.length() ? saved_ssid.c_str() : "Offline Mode", "Blackout Eco");
        state_timer_ms = millis();
        net_mode = NetMode::BLACKOUT_ECO;
    }

    void updateInternalRtcClock() {
        uint32_t now = millis();
        if (now - last_rtc_tick < 1000) return;
        last_rtc_tick = now;

        if (state.wifi_connected) {
            state.wifi_rssi = WiFi.RSSI();
        }

        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 0)) {
            if (timeinfo.tm_year > 120) {
                char t_buf[10], d_buf[16];
                strftime(t_buf, sizeof(t_buf), "%H:%M", &timeinfo);
                strftime(d_buf, sizeof(d_buf), "%d.%m.%Y", &timeinfo);
                state.setTimeStrings(t_buf, d_buf);
            }
        }
    }

public:
    NetworkService(AppState& st)
        : state(st), portal(st, storage, scanner), ota(st) {}

    const char* getName() const override { return "Network"; }

    void init() override {
        storage.loadUserSettings(state);
        storage.loadWifiCredentials(saved_ssid, saved_pass);
        configTzTime(Config::TZ_INFO, Config::NTP_SERVER);

        if (saved_ssid.length() > 0) {
            startStaConnection();
        } else {
            enterApPortalMode();
        }
    }

    void update() override {
        storage.checkDelayedSave(state);
        updateInternalRtcClock();

        if (state.cmd_start_ap) {
            state.cmd_start_ap = false;
            enterApPortalMode();
        }

        if (net_mode != NetMode::BLACKOUT_ECO && net_mode != NetMode::BLACKOUT_PROBE) {
            portal.handle();
        }

        switch (net_mode) {
            case NetMode::AP_PORTAL: {
                dnsServer.processNextRequest();
                uint8_t clients = WiFi.softAPgetStationNum();
                if (clients != state.ap_clients) {
                    state.ap_clients = clients;
                    state.triggerLedNotification(NOTIFY_CYAN_WAVE, false);
                }
                if (clients > 0) state_timer_ms = millis();
                else if (millis() - state_timer_ms >= Config::AP_PORTAL_TIMEOUT_MS) enterBlackoutEcoMode();
                break;
            }
            case NetMode::CONNECTING:
                if (WiFi.status() == WL_CONNECTED) {
                    scanner.triggerAsyncScan();
                    onWifiConnected();
                } else if (millis() - state_timer_ms >= Config::WIFI_BOOT_TIMEOUT_MS) {
                    state.had_blackout_drop = true;
                    enterBlackoutEcoMode();
                }
                break;
            case NetMode::CONNECTED:
                if (WiFi.status() == WL_CONNECTED) {
                    ota.handle();
                    scanner.checkAsyncScan();
                } else {
                    if (state.wifi_connected) {
                        state.wifi_connected = false;
                        state.registerActivity();
                    }
                    if (drop_start_ms == 0) {
                        drop_start_ms = millis();
                        WiFi.reconnect();
                    } else if (millis() - drop_start_ms >= 20000) {
                        state.had_blackout_drop = true;
                        enterBlackoutEcoMode();
                    }
                }
                break;
            case NetMode::BLACKOUT_ECO:
                if (saved_ssid.length() > 0 && (millis() - state_timer_ms >= Config::BLACKOUT_PROBE_EVERY_MS)) {
                    WiFi.persistent(false);
                    WiFi.mode(WIFI_STA);
                    WiFi.setScanMethod(WIFI_FAST_SCAN);
                    WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());
                    state_timer_ms = millis();
                    net_mode = NetMode::BLACKOUT_PROBE;
                }
                break;
            case NetMode::BLACKOUT_PROBE:
                if (WiFi.status() == WL_CONNECTED) onWifiConnected();
                else if (millis() - state_timer_ms >= 4500) enterBlackoutEcoMode();
                break;
            default:
                break;
        }
    }
};