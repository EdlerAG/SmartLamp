#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "AppState.h"
#include "Config.h"

class StorageService {
private:
    Preferences prefs;

public:
    void loadUserSettings(AppState& state) {
        if (prefs.begin("box_state", false)) {
            state.light_mode     = prefs.getUChar("l_mode", 0);
            state.auto_brt       = prefs.getBool("a_brt", true);
            state.manual_brt_idx = prefs.getUChar("m_brt", 1);

            String tok = prefs.getString("tg_tok", Config::DEFAULT_TG_TOKEN);
            String adm = prefs.getString("tg_adm", Config::DEFAULT_ADMIN_ID);
            String grl = prefs.getString("tg_grl", Config::DEFAULT_GIRL_ID);

            state.setTelegramConfig(tok.c_str(), adm.c_str(), grl.c_str());
            prefs.end();
        }
    }

    void saveTelegramConfig(AppState& state, const String& tok, const String& adm, const String& grl) {
        char cur_tok[64], cur_adm[20], cur_grl[20];
        state.getTelegramConfig(cur_tok, sizeof(cur_tok), cur_adm, sizeof(cur_adm), cur_grl, sizeof(cur_grl));

        const char* final_tok = (tok.length() > 10) ? tok.c_str() : cur_tok;
        const char* final_adm = (adm.length() > 0)  ? adm.c_str() : cur_adm;
        const char* final_grl = (grl.length() > 0)  ? grl.c_str() : cur_grl;

        state.setTelegramConfig(final_tok, final_adm, final_grl);

        if (prefs.begin("box_state", false)) {
            prefs.putString("tg_tok", final_tok);
            prefs.putString("tg_adm", final_adm);
            prefs.putString("tg_grl", final_grl);
            prefs.end();
        }
    }

    void checkDelayedSave(AppState& state) {
        // Якщо змінився ID через команду /iam_admin або /iam_nastia у Telegram
        if (state.tg_config_dirty) {
            state.tg_config_dirty = false;
            char tok[64], adm[20], grl[20];
            state.getTelegramConfig(tok, sizeof(tok), adm, sizeof(adm), grl, sizeof(grl));
            if (prefs.begin("box_state", false)) {
                prefs.putString("tg_tok", tok);
                prefs.putString("tg_adm", adm);
                prefs.putString("tg_grl", grl);
                prefs.end();
            }
        }

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