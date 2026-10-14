#pragma once
#include "IService.h"
#include "Config.h"
#include "AppState.h"
#include "StorageService.h"
#include "WifiScanner.h"
#include "PortalServer.h"
#include "OtaManager.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <time.h>

class NetworkService : public IService {
private:
    enum class NetMode : uint8_t { IDLE, CONNECTING, CONNECTED, AP_PORTAL, OFFLINE_ECO };
    NetMode net_mode = NetMode::IDLE;

    AppState&      state;
    StorageService storage;
    WifiScanner    scanner;
    PortalServer   portal;
    OtaManager     ota;
    DNSServer      dnsServer;

    String   saved_ssid = "";
    String   saved_pass = "";
    uint32_t connect_start_ms = 0;
    uint32_t ap_start_ms      = 0;
    uint32_t last_time_check  = 0;
    uint32_t last_reconnect   = 0;

    void enterApPortalMode() {
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

        ap_start_ms = millis();
        state.ap_mode_active = true;
        state.ap_clients     = 0;
        state.wifi_connected = false;
        state.wifi_sleeping  = false;
        state.setNetworkInfo(Config::AP_SSID, WiFi.softAPIP().toString().c_str());
        state.screen_mode = SCREEN_NETWORK;
        state.registerActivity();
        net_mode = NetMode::AP_PORTAL;
    }

    void enterOfflineEcoMode() {
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        state.ap_mode_active = false;
        state.ap_clients     = 0;
        state.wifi_connected = false;
        state.wifi_sleeping  = true;
        state.setNetworkInfo("ECO SLEEP (Hold L)", "Radio OFF");
        net_mode = NetMode::OFFLINE_ECO;
    }

public:
    NetworkService(AppState& st)
        : state(st), portal(st, storage, scanner), ota(st) {}

    void init() override {
        storage.loadUserSettings(state);
        storage.loadWifiCredentials(saved_ssid, saved_pass);

        if (saved_ssid.length() > 0) {
            WiFi.persistent(false);
            WiFi.mode(WIFI_STA);
            WiFi.setScanMethod(WIFI_FAST_SCAN);
            WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
            WiFi.setSleep(false);
            WiFi.setTxPower(WIFI_POWER_19_5dBm);
            WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

            portal.begin();
            state.setNetworkInfo(saved_ssid.c_str(), "Fast Connect...");
            connect_start_ms = millis();
            net_mode = NetMode::CONNECTING;
        } else {
            enterApPortalMode();
        }
    }

    void update() override {
        storage.checkDelayedSave(state);

        if (state.cmd_start_ap) {
            state.cmd_start_ap = false;
            enterApPortalMode();
        }

        if (net_mode != NetMode::OFFLINE_ECO) {
            portal.handle();
        }

        switch (net_mode) {
            case NetMode::AP_PORTAL: {
                dnsServer.processNextRequest();
                uint8_t clients = WiFi.softAPgetStationNum();

                if (clients != state.ap_clients) {
                    state.ap_clients = clients;
                    state.led_notify_trigger = true;
                    state.registerActivity();
                }

                if (clients > 0) {
                    ap_start_ms = millis();
                } else if (millis() - ap_start_ms >= 180000) {
                    enterOfflineEcoMode();
                }
                break;
            }

            case NetMode::CONNECTING:
                if (WiFi.status() == WL_CONNECTED) {
                    WiFi.setSleep(WIFI_PS_MIN_MODEM);
                    state.wifi_connected = true;
                    state.ap_mode_active = false;
                    state.wifi_sleeping  = false;
                    state.wifi_rssi      = WiFi.RSSI();
                    state.setNetworkInfo(saved_ssid.c_str(), WiFi.localIP().toString().c_str());
                    MDNS.begin(Config::DEVICE_NAME);
                    configTzTime(Config::TZ_INFO, Config::NTP_SERVER);
                    ota.begin();
                    scanner.triggerAsyncScan();
                    // Сповіщаємо інші сервіси + LED-нотифікація
                    state.led_notify_trigger = true;
                    state.sendEvent(EventType::WIFI_CONNECTED);
                    // Переключаємо на головний екран і примусово перемальовуємо UI
                    state.screen_mode = SCREEN_CLIMATE;
                    state.registerActivity();
                    net_mode = NetMode::CONNECTED;
                } else if (millis() - connect_start_ms >= 5500) {
                    enterApPortalMode();
                }
                break;

            case NetMode::CONNECTED:
                if (WiFi.status() == WL_CONNECTED) {
                    ota.handle();
                    scanner.checkAsyncScan();
                    uint32_t now = millis();
                    if (now - last_time_check >= 1000) {
                        last_time_check = now;
                        state.wifi_rssi = WiFi.RSSI();
                        struct tm timeinfo;
                        if (getLocalTime(&timeinfo, 2)) {
                            // Форматуємо поза критичною секцією!
                            char t_buf[10], d_buf[16];
                            strftime(t_buf, sizeof(t_buf), "%H:%M", &timeinfo);
                            strftime(d_buf, sizeof(d_buf), "%d.%m.%Y", &timeinfo);
                            state.setTimeStrings(t_buf, d_buf);
                        }
                    }
                } else {
                    // Щойно втратили з'єднання — одразу оновлюємо UI
                    if (state.wifi_connected) {
                        state.wifi_connected = false;
                        state.registerActivity();
                    }
                    if (millis() - last_reconnect >= 15000) {
                        last_reconnect = millis();
                        WiFi.reconnect();
                    }
                }
                break;

            case NetMode::OFFLINE_ECO:
                break;

            default:
                break;
        }
    }
};