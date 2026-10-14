#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "AppState.h"

class StorageService {
private:
    Preferences prefs;

public:
    void loadUserSettings(AppState& state) {
        if (prefs.begin("box_state", false)) {
            state.light_mode     = prefs.getUChar("l_mode", 0);
            state.auto_brt       = prefs.getBool("a_brt", true);
            state.manual_brt_idx = prefs.getUChar("m_brt", 1);
            prefs.end();
        }
    }

    // Відкладений запис через 5 секунд тиші (захист Flash від зносу)
    void checkDelayedSave(AppState& state) {
        if (state.settings_dirty && (millis() - state.settings_change_ms >= 5000)) {
            state.settings_dirty = false;
            if (prefs.begin("box_state", false)) {
                prefs.putUChar("l_mode", state.light_mode);
                prefs.putBool("a_brt", state.auto_brt);
                prefs.putUChar("m_brt", state.manual_brt_idx);
                prefs.end();
            }
        }
    }

    void loadWifiCredentials(String& ssid, String& pass) {
        if (prefs.begin("wifi_cfg", false)) {
            ssid = prefs.getString("ssid", "");
            pass = prefs.getString("pass", "");
            prefs.end();
        }
    }

    void saveWifiCredentials(const String& ssid, const String& pass) {
        if (prefs.begin("wifi_cfg", false)) {
            prefs.putString("ssid", ssid);
            prefs.putString("pass", pass);
            prefs.end();
        }
    }
};