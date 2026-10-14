#pragma once
#include <WebServer.h>
#include <Update.h>
#include <WiFi.h>
#include "AppState.h"
#include "StorageService.h"
#include "WifiScanner.h"
#include "WebPortalHtml.h"

class PortalServer {
private:
    WebServer server{80};
    AppState& state;
    StorageService& storage;
    WifiScanner& scanner;
    uint32_t reboot_at_ms = 0;
    bool started = false;
    bool ota_header_checked = false;

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

    void begin() {
        if (started) return;

        server.enableDelay(false);

        server.on("/", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/setup", HTTP_GET, [this]() { sendPortalPage(); });

        // Apple iOS / macOS Captive Portal
        server.on("/hotspot-detect.html", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/library/test/success.html", HTTP_GET, [this]() { sendPortalPage(); });
        server.on("/success.txt", HTTP_GET, [this]() { sendPortalPage(); });

        // Android / Windows Captive Portal
        server.on("/generate_204", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/gen_204", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/ncsi.txt", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/connecttest.txt", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/redirect", HTTP_GET, [this]() { redirectCaptive(); });
        server.on("/canonical.html", HTTP_GET, [this]() { redirectCaptive(); });

        server.on("/api/status", HTTP_GET, [this]() {
            char buf[120];
            snprintf(buf, sizeof(buf), "{\"t\":%.1f,\"h\":%.0f,\"p\":%.0f,\"l\":%d,\"m\":%d}",
                     state.temperature, state.humidity, state.pressure, state.light_percent, state.light_mode);
            server.send(200, "application/json", buf);
        });

        server.on("/api/cmd", HTTP_POST, [this]() {
            String act = server.arg("do");
            if (act == "mode") state.sendEvent(EventType::BTN_TOP_SHORT);
            else if (act == "brt") state.sendEvent(EventType::BTN_RIGHT_SHORT);
            server.send(200, "text/plain", "OK");
        });

        server.on("/api/msg", HTTP_POST, [this]() {
            String s = server.arg("sender");
            String m = server.arg("msg");
            s.trim();
            m.trim();
            if (m.length() > 0) {
                state.setTelegramData(s.length() ? s.c_str() : "Web Hub", m.c_str());
                state.screen_mode = SCREEN_TELEGRAM;
                state.led_notify_trigger = true;
            }
            server.send(200, "text/plain", "OK");
        });

        server.on("/api/scan", HTTP_GET, [this]() {
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

        // Безпечне Web-OTA оновлення з перевіркою магічного байта 0xE9 та обробкою обриву
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
                    // Перший байт справжньої прошивки ESP32 завжди 0xE9
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

        server.onNotFound([this]() { sendPortalPage(); });

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