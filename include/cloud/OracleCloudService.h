#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "core/StorageService.h"
#include "ui/DisplayService.h"
#include "cloud/CloudOtaService.h"
#include "cloud/TgJsonUtils.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class OracleCloudService : public IService {
private:
    AppState&        state;
    DisplayService*  display_srv = nullptr;
    StorageService*  storage_srv = nullptr;

    WiFiClient       net_client;
    WiFiClientSecure sec_client;
    HTTPClient       http;
    String           base_server_url = "";
    String           sync_endpoint   = "";
    String           api_key         = "";

    void refreshConfig() {
        char u_buf[128], k_buf[64];
        state.getOracleConfig(u_buf, sizeof(u_buf), k_buf, sizeof(k_buf));
        String base = String(u_buf);
        base.trim();
        while (base.endsWith("/")) base.remove(base.length() - 1);

        api_key = String(k_buf);
        api_key.trim();

        if (base.length() < 10 || base.indexOf("127.0.0.1") != -1 ||
            base.indexOf("localhost") != -1 || base.indexOf("YOUR_ORACLE_IP") != -1) {
            base_server_url = "";
            sync_endpoint   = "";
            return;
        }

        int api_idx = base.indexOf("/api/");
        if (api_idx != -1) {
            base_server_url = base.substring(0, api_idx);
            sync_endpoint   = base;
        } else {
            base_server_url = base;
            sync_endpoint   = base + "/api/v5/box/sync";
        }
    }

    String normalizeMediaUrl(const String& raw_url) const {
        if (raw_url.length() == 0) return "";
        if (raw_url.startsWith("/")) {
            return base_server_url + raw_url;
        }
        if (raw_url.indexOf("127.0.0.1") != -1 || raw_url.indexOf("localhost") != -1 || raw_url.indexOf("YOUR_ORACLE_IP") != -1) {
            int path_idx = raw_url.indexOf("/api/");
            if (path_idx != -1 && base_server_url.length() > 0) {
                return base_server_url + raw_url.substring(path_idx);
            }
        }
        return raw_url;
    }

    bool downloadRgb565ToCanvas(const String& raw_url) {
        if (!state.psram_anim_buf || !state.wifi_connected) return false;
        String url = normalizeMediaUrl(raw_url);

        HTTPClient img_http;
        WiFiClient img_client;
        WiFiClientSecure img_sec;

        img_http.setConnectTimeout(3500);
        img_http.setTimeout(6000);

        state.tls_in_flight = true;
        bool begun = false;
        if (url.startsWith("https://")) {
            img_sec.setInsecure();
            img_sec.setNoDelay(true);
            begun = img_http.begin(img_sec, url);
        } else {
            img_client.setNoDelay(true);
            begun = img_http.begin(img_client, url);
        }

        if (!begun) {
            state.tls_in_flight = false;
            return false;
        }

        int code = img_http.GET();
        int content_len = img_http.getSize();
        if (code != HTTP_CODE_OK || (content_len > 0 && content_len != 115200)) {
            img_http.end();
            state.tls_in_flight = false;
            return false;
        }

        WiFiClient* stream = img_http.getStreamPtr();
        size_t total_read = 0;
        uint32_t start_ms = millis();
        uint8_t* target_ptr = state.psram_anim_buf + sizeof(MbxHeader);

        while (img_http.connected() && total_read < 115200 && (millis() - start_ms < 6000)) {
            size_t avail = stream->available();
            if (avail > 0) {
                size_t to_read = min(avail, (size_t)(115200 - total_read));
                int r = stream->readBytes(target_ptr + total_read, to_read);
                if (r > 0) total_read += r;
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        img_http.end();
        state.tls_in_flight = false;

        if (total_read == 115200) {
            MbxHeader* hdr = (MbxHeader*)state.psram_anim_buf;
            memcpy(hdr->magic, "MBX5", 4);
            hdr->num_frames = 1;
            hdr->fps = 1;
            hdr->ambi_r = state.ambi_r;
            hdr->ambi_g = state.ambi_g;
            hdr->ambi_b = state.ambi_b;
            hdr->crc32 = 0;
            uint32_t dur = (state.media_expire_ms > millis()) ? (state.media_expire_ms - millis()) : 0;
            state.activateCustomAnimation(1, 1, state.ambi_r, state.ambi_g, state.ambi_b, dur);
            state.screen_mode = SCREEN_LIVING_GIF;
            state.ui_force_dirty = true;
            return true;
        }
        return false;
    }

    static uint32_t calculateCrc32(const uint8_t* data, size_t length) {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(int32_t)(crc & 1)));
            }
        }
        return ~crc;
    }

    bool downloadMbxAnimation(const String& raw_url, bool is_saver = false) {
        if (!state.psram_anim_buf || !state.wifi_connected) return false;
        String url = normalizeMediaUrl(raw_url);

        HTTPClient mbx_http;
        WiFiClient m_client;
        WiFiClientSecure m_sec;

        mbx_http.setConnectTimeout(4000);
        mbx_http.setTimeout(10000);

        state.tls_in_flight = true;
        bool begun = url.startsWith("https://")
                     ? (m_sec.setInsecure(), m_sec.setNoDelay(true), mbx_http.begin(m_sec, url))
                     : (m_client.setNoDelay(true), mbx_http.begin(m_client, url));

        if (!begun) {
            state.tls_in_flight = false;
            return false;
        }

        int code = mbx_http.GET();
        if (code != HTTP_CODE_OK) {
            mbx_http.end();
            state.tls_in_flight = false;
            return false;
        }

        int content_len = mbx_http.getSize();
        size_t max_buf = sizeof(MbxHeader) + Config::MBX_MAX_FRAMES * Config::FRAME_BYTES_RGB565;
        if (content_len > (int)max_buf) {
            mbx_http.end();
            state.tls_in_flight = false;
            return false;
        }

        WiFiClient* stream = mbx_http.getStreamPtr();
        size_t total_read = 0;
        uint32_t start_ms = millis();
        size_t target_len = (content_len > 0) ? (size_t)content_len : max_buf;

        while (mbx_http.connected() && total_read < target_len && (millis() - start_ms < 10000)) {
            size_t avail = stream->available();
            if (avail > 0) {
                size_t to_read = min(avail, target_len - total_read);
                int r = stream->readBytes(state.psram_anim_buf + total_read, to_read);
                if (r > 0) total_read += r;
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            if (content_len > 0 && total_read >= (size_t)content_len) break;
        }

        mbx_http.end();
        state.tls_in_flight = false;

        if (total_read >= sizeof(MbxHeader)) {
            MbxHeader* hdr = (MbxHeader*)state.psram_anim_buf;
            if (memcmp(hdr->magic, "MBX5", 4) == 0 && hdr->num_frames > 0) {
                // Захист від битих кадрів: перевірка контрольної суми CRC32
                if (hdr->crc32 != 0 && total_read > sizeof(MbxHeader)) {
                    uint32_t calc_crc = calculateCrc32(state.psram_anim_buf + sizeof(MbxHeader), total_read - sizeof(MbxHeader));
                    if (calc_crc != hdr->crc32) {
                        return false;
                    }
                }
                uint32_t dur = (state.media_expire_ms > millis()) ? (state.media_expire_ms - millis()) : 0;
                state.activateCustomAnimation(hdr->num_frames, hdr->fps, hdr->ambi_r, hdr->ambi_g, hdr->ambi_b, dur);
                state.screen_mode = SCREEN_LIVING_GIF;
                state.ui_force_dirty = true;

                if (is_saver && storage_srv && state.psram_saver_buf) {
                    // Зберігаємо постійну заставку у LittleFS
                    storage_srv->saveSaverFile(state.psram_anim_buf, total_read);
                    storage_srv->setSaverCrc(hdr->crc32);

                    size_t cpy_len = min(total_read, sizeof(MbxHeader) + (size_t)(18 * Config::FRAME_BYTES_RGB565));
                    memcpy(state.psram_saver_buf, state.psram_anim_buf, cpy_len);
                    state.saver_num_frames = hdr->num_frames;
                    state.saver_fps        = hdr->fps;
                    state.saver_crc        = hdr->crc32;
                    state.saver_buf_size   = cpy_len;
                    state.saver_loaded     = true;
                    state.screen_mode      = SCREEN_LIVING_GIF;
                    state.ui_force_dirty   = true;
                }
                return true;
            }
        }
        return false;
    }

    bool downloadSpotifyVinylArt() {
        if (!display_srv || !state.wifi_connected || base_server_url.length() == 0) return false;
        String url = base_server_url + "/api/v5/spotify/vinyl.rgb565";

        HTTPClient v_http;
        WiFiClient v_client;
        WiFiClientSecure v_sec;

        v_http.setConnectTimeout(3000);
        v_http.setTimeout(5000);

        state.tls_in_flight = true;
        bool begun = url.startsWith("https://")
                     ? (v_sec.setInsecure(), v_sec.setNoDelay(true), v_http.begin(v_sec, url))
                     : (v_client.setNoDelay(true), v_http.begin(v_client, url));

        if (!begun) {
            state.tls_in_flight = false;
            return false;
        }

        int code = v_http.GET();
        int content_len = v_http.getSize();
        if (code != HTTP_CODE_OK || (content_len > 0 && content_len != (int)Config::VINYL_BYTES_RGB565)) {
            v_http.end();
            state.tls_in_flight = false;
            return false;
        }

        uint8_t* vinyl_buf = (uint8_t*)display_srv->getVinylSprite().getBuffer();
        if (!vinyl_buf) {
            v_http.end();
            state.tls_in_flight = false;
            return false;
        }

        WiFiClient* stream = v_http.getStreamPtr();
        size_t total_read = 0;
        uint32_t start_ms = millis();

        while (v_http.connected() && total_read < Config::VINYL_BYTES_RGB565 && (millis() - start_ms < 5000)) {
            size_t avail = stream->available();
            if (avail > 0) {
                size_t to_read = min(avail, Config::VINYL_BYTES_RGB565 - total_read);
                int r = stream->readBytes(vinyl_buf + total_read, to_read);
                if (r > 0) total_read += r;
            } else {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        v_http.end();
        state.tls_in_flight = false;

        if (total_read == Config::VINYL_BYTES_RGB565) {
            state.spotify.vinyl_ready = true;
            state.ui_force_dirty = true;
            return true;
        }
        return false;
    }

    void processServerResponse(const String& json) {
        state.oracle_manages_bot  = (json.indexOf("\"bot_managed\":true") != -1);
        state.has_more_cloud_cmds = (json.indexOf("\"has_more\":true") != -1);

        // Статус підключення Spotify від сервера (для майстра Onboarding та плеєра)
        if (json.indexOf("\"sp_linked\":true") != -1) {
            state.sp_linked = true;
        } else if (json.indexOf("\"sp_linked\":false") != -1) {
            state.sp_linked = false;
        }

        // Обробка тихих годин та режиму DND для Насті (v5.0-v5.3)
        if (json.indexOf("\"quiet_hours\":true") != -1) {
            if (!state.quiet_hours_active) {
                state.setQuietHours(true);
            }
        } else if (json.indexOf("\"quiet_hours\":false") != -1) {
            if (state.quiet_hours_active) {
                state.setQuietHours(false);
            }
        }

        if (json.indexOf("\"dnd\":true") != -1) {
            state.dnd_mode = true;
            state.audio_muted = true;
        } else if (json.indexOf("\"dnd\":false") != -1) {
            state.dnd_mode = false;
            state.audio_muted = false;
        }

        // Обробка найближчої події Google Календаря Насті (Zero-Knowledge Smart Hub)
        int cal_idx = json.indexOf("\"cal_event\":{");
        if (cal_idx != -1) {
            int cal_end = json.indexOf("}", cal_idx);
            if (cal_end != -1) {
                String cal_block = json.substring(cal_idx, cal_end + 1);
                String summary = TgJsonUtils::extractString(cal_block, "summary");
                String time_str = TgJsonUtils::extractString(cal_block, "time");
                bool is_soon = (cal_block.indexOf("\"is_soon\":true") != -1);
                char s_dec[36];
                TgJsonUtils::decodeJsonString(summary, s_dec, sizeof(s_dec));
                state.setCalendarEvent(s_dec, time_str.c_str(), is_soon);
            }
        } else {
            state.clearCalendarEvent();
        }

        // Обробка Spotify стану від сервера (якщо передано)
        int sp_idx = json.indexOf("\"spotify\":{");
        if (sp_idx != -1) {
            int sp_end = json.indexOf("}", sp_idx);
            if (sp_end != -1) {
                String sp_block = json.substring(sp_idx, sp_end + 1);
                bool playing = (sp_block.indexOf("\"playing\":true") != -1);
                state.spotify.playing = playing;

                String tr_id = TgJsonUtils::extractString(sp_block, "track_id");
                bool track_changed = (tr_id.length() > 0 && strcmp(state.spotify.track_id, tr_id.c_str()) != 0);

                if (track_changed) {
                    strncpy(state.spotify.track_id, tr_id.c_str(), sizeof(state.spotify.track_id) - 1);
                    state.spotify.vinyl_ready = false;
                    state.spotify_vinyl_needed = true;
                }

                String title = TgJsonUtils::extractString(sp_block, "title");
                if (title.length() > 0) {
                    TgJsonUtils::decodeJsonString(title, state.spotify.title, sizeof(state.spotify.title));
                }

                String artist = TgJsonUtils::extractString(sp_block, "artist");
                if (artist.length() > 0) {
                    TgJsonUtils::decodeJsonString(artist, state.spotify.artist, sizeof(state.spotify.artist));
                }

                String prog_str = TgJsonUtils::extractIntStr(sp_block, "progress_ms");
                if (prog_str.length() > 0) state.spotify.progress_ms = (uint32_t)prog_str.toInt();

                String dur_str = TgJsonUtils::extractIntStr(sp_block, "duration_ms");
                if (dur_str.length() > 0) state.spotify.duration_ms = (uint32_t)dur_str.toInt();

                int ambi_idx = sp_block.indexOf("\"ambi_rgb\":[");
                if (ambi_idx != -1) {
                    int c1 = sp_block.indexOf(',', ambi_idx + 12);
                    int c2 = (c1 != -1) ? sp_block.indexOf(',', c1 + 1) : -1;
                    int end_brk = (c2 != -1) ? sp_block.indexOf(']', c2 + 1) : -1;
                    if (c1 != -1 && c2 != -1 && end_brk != -1) {
                        uint8_t r = (uint8_t)sp_block.substring(ambi_idx + 12, c1).toInt();
                        uint8_t g = (uint8_t)sp_block.substring(c1 + 1, c2).toInt();
                        uint8_t b = (uint8_t)sp_block.substring(c2 + 1, end_brk).toInt();
                        state.spotify.ambi_r = r;
                        state.spotify.ambi_g = g;
                        state.spotify.ambi_b = b;
                        state.ambi_r = r;
                        state.ambi_g = g;
                        state.ambi_b = b;
                    }
                }

                state.spotify.last_sync_ms = millis();
            }
        }

        if (json.indexOf("\"has_cmd\":true") == -1) return;
        state.registerNetActivity();

        // 1. Команди світла та яскравості
        String mode_str = TgJsonUtils::extractIntStr(json, "set_mode");
        if (mode_str.length() > 0) {
            CommandRouter::setLightMode(state, (uint8_t)mode_str.toInt());
        }

        String brt_str = TgJsonUtils::extractString(json, "set_brt");
        if (brt_str.length() > 0) {
            if (brt_str == "auto") CommandRouter::setBrightnessPreset(state, true);
            else CommandRouter::setBrightnessPreset(state, false, (uint8_t)brt_str.toInt());
        }

        // 2. Команди звуку: гучність, Mute, тактильні кліки
        String vol_str = TgJsonUtils::extractIntStr(json, "volume");
        if (vol_str.length() > 0) {
            uint8_t v = (uint8_t)constrain((int)vol_str.toInt(), 0, 21);
            CommandRouter::setVolume(state, v);
            if (storage_srv) storage_srv->saveAudioSettings(v, state.audio_muted, state.touch_beep_enabled);
        }

        if (json.indexOf("\"muted\":true") != -1) {
            CommandRouter::setMute(state, true);
            if (storage_srv) storage_srv->saveAudioSettings(state.audio_volume, true, state.touch_beep_enabled);
        } else if (json.indexOf("\"muted\":false") != -1) {
            CommandRouter::setMute(state, false);
            if (storage_srv) storage_srv->saveAudioSettings(state.audio_volume, false, state.touch_beep_enabled);
        }

        if (json.indexOf("\"touch_beep\":true") != -1) {
            CommandRouter::setTouchBeep(state, true);
            if (storage_srv) storage_srv->saveAudioSettings(state.audio_volume, state.audio_muted, true);
        } else if (json.indexOf("\"touch_beep\":false") != -1) {
            CommandRouter::setTouchBeep(state, false);
            if (storage_srv) storage_srv->saveAudioSettings(state.audio_volume, state.audio_muted, false);
        }

        // 3. Команди керування показом медіа: таймер та зупинка
        if (json.indexOf("\"stop_media\":true") != -1) {
            CommandRouter::stopActiveMedia(state);
        }

        // 4. Святкове привітання з днем народження (v5.1)
        if (json.indexOf("\"birthday_celebrate\":true") != -1) {
            state.triggerBirthdayCelebration(20000);
            state.trig_reaction_synth = NOTIFY_CROWN;
            state.light_mode = MODE_SUNSET_VIBE;
        }

        // 5. Керування режимом Pomodoro (v5.1 - v5.3)
        if (json.indexOf("\"pomo_action\":\"stop\"") != -1) {
            state.pomoStop();
        } else if (json.indexOf("\"pomo_action\":\"start\"") != -1) {
            String p_min = TgJsonUtils::extractIntStr(json, "pomo_min");
            if (p_min.length() > 0) {
                state.pomo_duration_min = (uint8_t)constrain((int)p_min.toInt(), 5, 90);
            }
            bool is_brk = (json.indexOf("\"pomo_break\":true") != -1);
            state.screen_mode = SCREEN_POMODORO;
            state.pomo_active = true;
            state.pomo_paused = false;
            state.pomo_is_break = is_brk;
            state.pomo_end_ms = millis() + (uint32_t)state.pomo_duration_min * 60000UL;
            state.light_mode = is_brk ? MODE_OCEAN_BREEZE : MODE_AMBER_EMBER;
            state.ui_force_dirty = true;
        }

        // 6. Системні команди обслуговування (v5.3)
        String sys_cmd = TgJsonUtils::extractString(json, "sys_cmd");
        if (sys_cmd.length() > 0) {
            if (sys_cmd == "reboot") {
                state.cmd_reboot = true;
            } else if (sys_cmd == "start_ap") {
                state.cmd_start_ap = true;
            } else if (sys_cmd == "clear_cache") {
                state.cmd_clear_cache = true;
                state.saver_loaded = false;
                state.saver_num_frames = 0;
                state.saver_crc = 0;
                if (storage_srv) storage_srv->clearScreensaver();
            }
        }

        String med_timer_str = TgJsonUtils::extractIntStr(json, "media_timer");
        if (med_timer_str.length() > 0) {
            CommandRouter::setMediaTimer(state, (uint16_t)med_timer_str.toInt());
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
                state.trig_reaction_synth = r;
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
            bool is_silent = (json.indexOf("\"silent\":true") >= 0 || json.indexOf("\"silent\": true") >= 0);
            if (!is_silent) {
                state.trig_notify_bell = true;
            }
        }

        String play_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "play_url"));
        if (play_url.length() > 8) {
            state.setPendingAudioUrl(play_url.c_str());
        }

        bool is_saver = (json.indexOf("\"set_saver\":true") != -1);
        String media_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "media_url"));
        String media_name = TgJsonUtils::extractString(json, "media_name");
        String media_crc = TgJsonUtils::extractString(json, "media_crc");

        if (media_url.length() > 8) {
            state.setPendingMedia(media_url.c_str(), media_name.c_str(), media_crc.c_str(), is_saver);
        } else {
            String img_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "img_url"));
            if (img_url.length() > 8) {
                state.setPendingImageUrl(img_url.c_str());
            }
        }

        String ota_url = normalizeMediaUrl(TgJsonUtils::extractString(json, "ota_url"));
        String ota_sha = TgJsonUtils::extractString(json, "ota_sha256");
        if (ota_url.length() > 10) {
            state.setPendingOtaUrl(ota_url.c_str(), ota_sha.c_str());
        }
    }

public:
    OracleCloudService(AppState& st) : state(st) {}

    const char* getName() const override { return "OracleCloud"; }

    void setDisplayService(DisplayService* ds) {
        display_srv = ds;
    }

    void setStorageService(StorageService* ss) {
        storage_srv = ss;
    }

    bool isEnabled() const override {
        char u_buf[128];
        state.getOracleConfig(u_buf, sizeof(u_buf), nullptr, 0);
        if (strlen(u_buf) < 10) return false;
        if (strstr(u_buf, "127.0.0.1") != nullptr ||
            strstr(u_buf, "localhost") != nullptr ||
            strstr(u_buf, "YOUR_ORACLE_IP") != nullptr) {
            return false;
        }
        return true;
    }

    uint32_t getIntervalMs() const override {
        if (state.has_more_cloud_cmds) return 300;
        return Config::ORACLE_SYNC_MS;
    }

    void init() override {
        refreshConfig();
        net_client.setNoDelay(true);
        sec_client.setInsecure();
        sec_client.setNoDelay(true);
        http.setConnectTimeout(2500);
        http.setTimeout(9000);
        http.setReuse(true);
    }

    void update() override {
        if (!state.wifi_connected || state.ap_mode_active || state.ota_in_progress) return;

        // Хмарне OTA-оновлення
        char ota_url_buf[160];
        char ota_sha_buf[65];
        if (state.popPendingOtaUrl(ota_url_buf, sizeof(ota_url_buf), ota_sha_buf, sizeof(ota_sha_buf))) {
            String err = "";
            if (CloudOtaService::flashFromUrl(state, String(ota_url_buf), err, ota_sha_buf)) {
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP.restart();
            }
        }

        refreshConfig();
        if (sync_endpoint.length() == 0) return;

        // Завантаження мультикадрового .mbx або статичного кадру
        char med_url[160], med_name[32], med_crc[16];
        bool is_sav = false;
        if (state.popPendingMedia(med_url, sizeof(med_url), med_name, sizeof(med_name), med_crc, sizeof(med_crc), is_sav)) {
            if (String(med_url).indexOf(".mbx") != -1) {
                downloadMbxAnimation(String(med_url), is_sav);
            } else {
                downloadRgb565ToCanvas(String(med_url));
            }
        }

        // Завантаження нової обкладинки вінілу для Spotify
        if (state.spotify_vinyl_needed && !state.audio_playing) {
            state.spotify_vinyl_needed = false;
            downloadSpotifyVinylArt();
        }

        // Захист від навантаження каналу під час I2S аудіо
        if (state.audio_playing && !state.cmd_send_tg_ack && state.cmd_send_meme_ping == 0 && !state.has_more_cloud_cmds) {
            return;
        }

        char status_json[576];
        CommandRouter::buildStatusJson(state, status_json, sizeof(status_json));

        String req_url = sync_endpoint;
        if (!state.has_more_cloud_cmds && !state.audio_playing) {
            float wait_sec = (state.cmd_spotify_action != 0 || state.btn_l_pressed || state.btn_r_pressed || state.btn_t_pressed) ? 1.0f : 6.0f;
            char wait_param[16];
            snprintf(wait_param, sizeof(wait_param), "?wait=%.1f", wait_sec);
            req_url += (req_url.indexOf('?') == -1 ? wait_param : String("&") + (wait_param + 1));
        }

        state.tls_in_flight = true;
        bool begun = req_url.startsWith("https://")
                     ? http.begin(sec_client, req_url)
                     : http.begin(net_client, req_url);

        if (begun) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-Device-Key", api_key);

            int code = http.POST((uint8_t*)status_json, strlen(status_json));
            if (code == 200) {
                state.oracle_online = true;
                state.last_cloud_sync_ms = millis();
                state.cloud_sync_ok_cnt++;
                String resp = http.getString();
                http.end();
                state.tls_in_flight = false;
                processServerResponse(resp);
                return;
            }
            state.oracle_online       = false;
            state.oracle_manages_bot  = false;
            state.has_more_cloud_cmds = false;
            state.cloud_sync_err_cnt++;
            http.end();
        } else {
            state.oracle_online       = false;
            state.oracle_manages_bot  = false;
            state.has_more_cloud_cmds = false;
            state.cloud_sync_err_cnt++;
        }
        state.tls_in_flight = false;
    }
};
