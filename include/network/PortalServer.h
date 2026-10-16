#pragma once
#include <WebServer.h>
#include <Update.h>
#include <WiFi.h>
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "cloud/CloudOtaService.h"
#include "core/StorageService.h"
#include "network/WifiScanner.h"
#include "network/WebPortalHtml.h"

class PortalServer {
private:
    WebServer server{80};
    AppState& state;
    StorageService& storage;
    WifiScanner& scanner;
    uint32_t reboot_at_ms = 0;
    bool started = false;
    bool ota_header_checked = false;

    void addCorsHeaders() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }

    void sendPortalPage() {
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.send_P(200, "text/html", PORTAL_HTML);
    }

    void redirectCaptive() {
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    }

public:
    PortalServer(AppState& st, StorageService& stor, WifiScanner& scan)
        : state(st), storage(stor), scanner(scan) {}

    void stop() {
        if (started) {
            server.stop();
            started = false;
        }
    }

    void begin() {
        if (started) return;
        server.enableDelay(false);

        server.on("/", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/setup", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/hotspot-detect.html", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/library/test/success.html", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/success.txt", HTTP_GET, [this]() { sendPortalPage(); });

        server.on("/generate_204", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/gen_204", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/ncsi.txt", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/connecttest.txt", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/redirect", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/canonical.html", HTTP_GET, [this]() { redirectCaptive(); });

        server.on("/api/status", HTTP_GET, [this]() {
            addCorsHeaders();
            char buf[440];
            CommandRouter::buildStatusJson(state, buf, sizeof(buf));
            server.send(200, "application/json", buf);
        });

        server.on("/api/inbox", HTTP_GET, [this]() {
            addCorsHeaders();
            char buf[768];
            CommandRouter::buildInboxJson(state, buf, sizeof(buf));
            server.send(200, "application/json", buf);
        });

        server.on("/api/cmd", HTTP_POST, [this]() {
            addCorsHeaders();
            if (server.hasArg("set_mode")) {
                CommandRouter::setLightMode(state, (uint8_t)server.arg("set_mode").toInt());
            } else if (server.hasArg("set_brt")) {
                String b = server.arg("set_brt");
                if (b == "auto") CommandRouter::setBrightnessPreset(state, true);
                else CommandRouter::setBrightnessPreset(state, false, (uint8_t)b.toInt());
            } else if (server.hasArg("sleep")) {
                CommandRouter::setSleepTimer(state, (uint16_t)server.arg("sleep").toInt());
            } else if (server.hasArg("react")) {
                String r = server.arg("react");
                if (r == "fire")       CommandRouter::triggerReaction(state, NOTIFY_FIRE, "App Hub", "FIRE VIBE!");
                else if (r == "zap")   CommandRouter::triggerReaction(state, NOTIFY_LIGHTNING, "App Hub", "KUS-ZAP!");
                else if (r == "cat")   CommandRouter::triggerReaction(state, NOTIFY_CAT, "App Hub", "TYTS-KUS!");
                else if (r == "ufo")   CommandRouter::triggerReaction(state, NOTIFY_UFO, "App Hub", "UFO BEAM!");
                else if (r == "crown") CommandRouter::triggerReaction(state, NOTIFY_CROWN, "App Hub", "QUEEN VIBE!");
            } else {
                String act = server.arg("do");
                if (act == "mode") {
                    if (server.hasArg("id")) CommandRouter::setLightMode(state, (uint8_t)server.arg("id").toInt());
                    else CommandRouter::nextLightMode(state);
                } else if (act == "brt") {
                    CommandRouter::cycleBrightness(state);
                } else if (act == "sleep") {
                    CommandRouter::setSleepTimer(state, (uint16_t)server.arg("min").toInt());
                } else if (act == "reaction") {
                    uint8_t rid = (uint8_t)server.arg("id").toInt();
                    NotifyLedType rtype = (NotifyLedType)constrain(rid + 1, (int)NOTIFY_FIRE, (int)NOTIFY_CROWN);
                    CommandRouter::triggerReaction(state, rtype, "API", "REACTION!");
                } else if (act == "power") {
                    CommandRouter::togglePower(state);
                } else if (act == "next_msg") {
                    CommandRouter::nextInboxMessage(state);
                }
            }
            server.send(200, "application/json", "{\"ok\":true}");
        });

        server.on("/api/msg", HTTP_POST, [this]() {
            addCorsHeaders();
            String s = server.arg("sender");
            String m = server.arg("msg");
            s.trim(); m.trim();
            if (m.length() > 0) {
                CommandRouter::pushMessage(state, s.length() ? s.c_str() : "Smart App", m.c_str());
            }
            server.send(200, "application/json", "{\"ok\":true}");
        });

        server.on("/api/tg", HTTP_POST, [this]() {
            addCorsHeaders();
            String tok = server.arg("token");
            String adm = server.arg("admin_id");
            String grl = server.arg("girl_id");
            tok.trim(); adm.trim(); grl.trim();
            storage.saveTelegramConfig(state, tok, adm, grl);
            server.send(200, "application/json", "{\"ok\":true}");
        });

        server.on("/api/cloud", HTTP_POST, [this]() {
            addCorsHeaders();
            String url = server.arg("oracle_url");
            String key = server.arg("oracle_key");
            url.trim(); key.trim();
            storage.saveOracleConfig(state, url, key);
            server.send(200, "application/json", "{\"ok\":true}");
        });

        server.on("/api/rollback", HTTP_POST, [this]() {
            addCorsHeaders();
            String err = "";
            if (CloudOtaService::rollbackToPrevious(err)) {
                server.send(200, "application/json", "{\"ok\":true}");
                reboot_at_ms = millis() + 500;
            } else {
                server.send(400, "application/json", "{\"ok\":false,\"err\":\"" + err + "\"}");
            }
        });

        server.on("/api/scan", HTTP_GET, [this]() {
            addCorsHeaders();
            scanner.checkAsyncScan();
            if (server.hasArg("refresh")) {
                scanner.triggerAsyncScan();
                server.send(200, "application/json", "{\"scanning\":true}");
                return;
            }
            if (scanner.isScanning()) {
                server.send(200, "application/json", "{\"scanning\":true}");
                return;
            }
            server.send(200, "application/json", scanner.getCachedJson());
        });

        server.on("/save", HTTP_POST, [this]() {
            String new_ssid = server.arg("ssid");
            String new_pass = server.arg("pass");
            new_ssid.trim();
            if (new_ssid.length() > 0) {
                storage.saveWifiCredentials(new_ssid, new_pass);
                server.send(200, "text/plain", "Мережу збережено!");
                reboot_at_ms = millis() + 500;
            } else {
                server.send(400, "text/plain", "Помилка SSID");
            }
        });

        server.on("/update", HTTP_POST, [this]() {
            server.sendHeader("Connection", "close");
            bool ok = !Update.hasError();
            server.send(ok ? 200 : 400, "text/plain", ok ? "OK" : "FAIL");
            if (ok) reboot_at_ms = millis() + 700;
        }, [this]() {
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                WiFi.setSleep(false);
                ota_header_checked = false;
                state.ota_in_progress = true;
                state.ota_progress = 0;
                state.registerActivity();
                Update.begin(UPDATE_SIZE_UNKNOWN);
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (!ota_header_checked && upload.currentSize > 0) {
                    ota_header_checked = true;
                    if (upload.buf[0] != 0xE9) {
                        Update.abort();
                        state.ota_in_progress = false;
                        return;
                    }
                }
                if (state.ota_in_progress) {
                    Update.write(upload.buf, upload.currentSize);
                    if (Update.size() > 0) {
                        state.ota_progress = (Update.progress() * 100) / Update.size();
                        state.registerActivity();
                    }
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                Update.end(true);
                state.ota_in_progress = false;
            } else if (upload.status == UPLOAD_FILE_ABORTED) {
                Update.abort();
                state.ota_in_progress = false;
                state.registerActivity();
            }
        });

        server.onNotFound([this]() {
            if (server.method() == HTTP_OPTIONS) {
                addCorsHeaders();
                server.send(204);
                return;
            }
            sendPortalPage();
        });

        server.begin();
        started = true;
    }

    void handle() {
        if (started) server.handleClient();
        if (reboot_at_ms > 0 && millis() >= reboot_at_ms) {
            delay(50);
            ESP.restart();
        }
    }
};