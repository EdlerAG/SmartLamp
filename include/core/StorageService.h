#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "core/AppState.h"
#include "Config.h"

class StorageService {
private:
    Preferences prefs;
    bool fs_mounted = false;

public:
    bool initFs() {
        if (LittleFS.begin(true)) {
            fs_mounted = true;
            return true;
        }
        Serial.println("[FS] LittleFS Mount Failed");
        fs_mounted = false;
        return false;
    }

    bool isFsMounted() const {
        return fs_mounted;
    }

    bool isOnboarded() {
        if (prefs.begin("box_state", true)) {
            bool val = prefs.getBool("onboarded", false);
            prefs.end();
            return val;
        }
        return false;
    }

    void setOnboarded(bool val) {
        if (prefs.begin("box_state", false)) {
            prefs.putBool("onboarded", val);
            prefs.end();
        }
    }

    uint32_t getSaverCrc() {
        if (prefs.begin("box_state", true)) {
            uint32_t crc = prefs.getUInt("saver_crc", 0);
            prefs.end();
            return crc;
        }
        return 0;
    }

    void setSaverCrc(uint32_t crc) {
        if (prefs.begin("box_state", false)) {
            prefs.putUInt("saver_crc", crc);
            prefs.end();
        }
    }

    bool getSaverShowClock() {
        if (prefs.begin("box_state", true)) {
            bool clk = prefs.getBool("saver_clk", true);
            prefs.end();
            return clk;
        }
        return true;
    }

    void setSaverShowClock(bool val) {
        if (prefs.begin("box_state", false)) {
            prefs.putBool("saver_clk", val);
            prefs.end();
        }
    }

    bool saveSaverFile(const uint8_t* data, size_t len) {
        if (!fs_mounted && !initFs()) return false;
        File f = LittleFS.open(Config::SAVER_FILE_PATH, "w");
        if (!f) return false;
        size_t written = f.write(data, len);
        f.flush();
        f.close();
        return written == len;
    }

    size_t loadSaverFile(uint8_t* dest, size_t max_len) {
        if (!dest || max_len == 0) return 0;
        if (!fs_mounted && !initFs()) return 0;
        if (!LittleFS.exists(Config::SAVER_FILE_PATH)) return 0;

        File f = LittleFS.open(Config::SAVER_FILE_PATH, "r");
        if (!f) return 0;

        size_t sz = f.size();
        if (sz == 0 || sz > max_len) {
            f.close();
            return 0;
        }

        size_t bytes_read = f.read(dest, sz);
        f.close();
        return bytes_read;
    }

    void loadUserSettings(AppState& state) {
        state.onboarded        = isOnboarded();
        state.saver_crc        = getSaverCrc();
        state.saver_show_clock = getSaverShowClock();

        if (prefs.begin("box_state", false)) {
            if (prefs.isKey("l_mode")) state.light_mode     = prefs.getUChar("l_mode", 0) % TOTAL_LIGHT_MODES;
            if (prefs.isKey("a_brt"))  state.auto_brt       = prefs.getBool("a_brt", true);
            if (prefs.isKey("m_brt"))  state.manual_brt_idx = prefs.getUChar("m_brt", 2) % 4;

            String tok = prefs.isKey("tg_tok") ? prefs.getString("tg_tok") : String(Config::DEFAULT_TG_TOKEN);
            String adm = prefs.isKey("tg_adm") ? prefs.getString("tg_adm") : String(Config::DEFAULT_ADMIN_ID);
            String grl = prefs.isKey("tg_grl") ? prefs.getString("tg_grl") : String(Config::DEFAULT_GIRL_ID);
            state.setTelegramConfig(tok.c_str(), adm.c_str(), grl.c_str());

            String o_url = prefs.isKey("or_url") ? prefs.getString("or_url") : String(Config::ORACLE_SERVER_URL);
            String o_key = prefs.isKey("or_key") ? prefs.getString("or_key") : String(Config::ORACLE_API_KEY);
            state.setOracleConfig(o_url.c_str(), o_key.c_str());

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

    void saveOracleConfig(AppState& state, const String& url, const String& key) {
        char cur_url[128], cur_key[64];
        state.getOracleConfig(cur_url, sizeof(cur_url), cur_key, sizeof(cur_key));

        const char* final_url = (url.length() > 8) ? url.c_str() : cur_url;
        const char* final_key = (key.length() > 0) ? key.c_str() : cur_key;

        state.setOracleConfig(final_url, final_key);

        if (prefs.begin("box_state", false)) {
            prefs.putString("or_url", final_url);
            prefs.putString("or_key", final_key);
            prefs.end();
        }
    }

    void checkDelayedSave(AppState& state) {
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

        if (state.cloud_config_dirty) {
            state.cloud_config_dirty = false;
            char url[128], key[64];
            state.getOracleConfig(url, sizeof(url), key, sizeof(key));
            if (prefs.begin("box_state", false)) {
                prefs.putString("or_url", url);
                prefs.putString("or_key", key);
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
            ssid = prefs.isKey("ssid") ? prefs.getString("ssid") : "";
            pass = prefs.isKey("pass") ? prefs.getString("pass") : "";
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
