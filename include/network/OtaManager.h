#pragma once
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "Config.h"
#include "core/AppState.h"

class OtaManager {
private:
    AppState& state;
    bool initialized = false;

public:
    OtaManager(AppState& st) : state(st) {}

    void begin() {
        if (initialized) return;
        ArduinoOTA.setHostname(Config::DEVICE_NAME);
        ArduinoOTA.onStart([this]() {
            WiFi.setSleep(false);
            state.ota_in_progress = true;
            state.ota_progress = 0;
            state.registerActivity();
        });
        ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
            if (total > 0) state.ota_progress = (progress * 100) / total;
            state.registerActivity();
        });
        ArduinoOTA.onEnd([this]() { state.ota_in_progress = false; });
        ArduinoOTA.onError([this](ota_error_t) { state.ota_in_progress = false; });
        ArduinoOTA.begin();
        initialized = true;
    }

    void handle() {
        if (initialized) ArduinoOTA.handle();
    }
};