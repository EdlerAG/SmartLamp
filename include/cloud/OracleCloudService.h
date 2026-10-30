#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include "core/CommandRouter.h"
#include "core/StorageService.h"
#include "ui/DisplayService.h"
#include "cloud/CloudOtaService.h"
#include "cloud/TgJsonUtils.h"
#include "utils/Crc32.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#undef INADDR_NONE
#include <esp_websocket_client.h>

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

    esp_websocket_client_handle_t ws_client = nullptr;
    String           ws_url = "";
    bool             ws_connected = false;
    bool             rx_binary_in_progress = false;
    uint32_t         last_ws_send_ms = 0;

    static void ws_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
        OracleCloudService* self = (OracleCloudService*)handler_args;
        if (!self) return;
        esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
        if (event_id == WEBSOCKET_EVENT_CONNECTED) {
            self->state.oracle_online = true;
            self->ws_connected = true;
            self->rx_binary_in_progress = false;
            self->state.ws_reconnect_cnt++;
        } else if (event_id == WEBSOCKET_EVENT_DISCONNECTED || event_id == WEBSOCKET_EVENT_CLOSED) {
            self->ws_connected = false;
            self->rx_binary_in_progress = false;
        } else if (event_id == WEBSOCKET_EVENT_DATA) {
            if (data && data->data_len > 0) {
                self->state.oracle_online = true;
                self->state.last_cloud_sync_ms = millis();
                self->state.cloud_sync_ok_cnt++;
                if (data->op_code == 1 && data->data_ptr) {
                    self->processServerResponse(data->data_ptr, data->data_len);
                } else if (data->op_code == 2 || (data->op_code == 0 && self->rx_binary_in_progress)) {
                    if (self->display_srv) {
                        uint8_t* vinyl_buf = (uint8_t*)self->display_srv->getVinylSprite().getBuffer();
                        if (vinyl_buf && ((size_t)(data->payload_offset + data->data_len) <= Config::VINYL_BYTES_RGB565)) {
                            self->rx_binary_in_progress = true;
                            memcpy(vinyl_buf + data->payload_offset, data->data_ptr, data->data_len);
                            if ((size_t)(data->payload_offset + data->data_len) >= Config::VINYL_BYTES_RGB565) {
                                self->rx_binary_in_progress = false;
                                self->state.spotify.vinyl_ready = true;
                                self->state.spotify_vinyl_needed = false;
                                self->state.ui_force_dirty = true;
                            }
                        }
                    }
                }
            }
        }
    }

    void initWebSocketClient() {
        if (ws_client) {
            esp_websocket_client_stop(ws_client);
            esp_websocket_client_destroy(ws_client);
            ws_client = nullptr;
        }
        if (base_server_url.length() < 8) return;

        String proto = "ws://";
        String host_path = base_server_url;
        if (host_path.startsWith("https://")) {
            proto = "wss://";
            host_path = host_path.substring(8);
        } else if (host_path.startsWith("http://")) {
            host_path = host_path.substring(7);
        }
        while (host_path.endsWith("/")) host_path.remove(host_path.length() - 1);

        ws_url = proto + host_path + "/ws/box?key=" + api_key;

        esp_websocket_client_config_t ws_cfg = {};
        ws_cfg.uri = ws_url.c_str();
        ws_cfg.task_prio = 5;
        ws_cfg.task_stack = 4096;
        ws_cfg.buffer_size = 8192;

        ws_client = esp_websocket_client_init(&ws_cfg);
        if (ws_client) {
            esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, ws_event_handler, (void*)this);
            esp_websocket_client_start(ws_client);
        }
    }

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
                    uint32_t calc_crc = Crc32::calculate(state.psram_anim_buf + sizeof(MbxHeader), total_read - sizeof(MbxHeader));
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

    void flushOfflineTelemetry() {
        if (state.offline_buf_count == 0 || !state.wifi_connected) return;

        uint32_t now_ms = millis();
        uint8_t to_send = min((uint8_t)state.offline_buf_count, (uint8_t)24);
        String json = "{\"type\":\"batch_telemetry\",\"points\":[";
        for (int i = 0; i < to_send; i++) {
            uint32_t age_sec = (now_ms >= state.offline_buf[i].sample_millis)
                               ? (now_ms - state.offline_buf[i].sample_millis) / 1000UL
                               : 0;
            if (i > 0) json += ",";
            char pt[72];
            snprintf(pt, sizeof(pt), "{\"age_sec\":%u,\"t\":%.1f,\"h\":%.0f,\"p\":%.1f,\"l\":%u,\"m\":%u}",
                     (unsigned)age_sec, state.offline_buf[i].temp, state.offline_buf[i].hum,
                     state.offline_buf[i].pres, (unsigned)state.offline_buf[i].light, (unsigned)state.offline_buf[i].mode);
            json += pt;
        }
        json += "]}";

        // ⚡️ Пріоритет: миттєва передача через відкритий WebSocket без накладних витрат TLS
        if (ws_connected && ws_client && esp_websocket_client_is_connected(ws_client)) {
            int sent = esp_websocket_client_send_text(ws_client, json.c_str(), json.length(), pdMS_TO_TICKS(400));
            if (sent > 0) {
                if (to_send >= state.offline_buf_count) {
                    state.clearOfflineSamples();
                } else {
                    for (int i = to_send; i < state.offline_buf_count; i++) {
                        state.offline_buf[i - to_send] = state.offline_buf[i];
                    }
                    state.offline_buf_count -= to_send;
                }
                return;
            }
        }

        // Аварійний фолбек на HTTPClient, якщо WS тимчасово недоступний
        if (base_server_url.length() == 0) return;
        String b_url = base_server_url + "/api/v5/box/batch-telemetry";

        HTTPClient b_http;
        WiFiClient b_client;
        WiFiClientSecure b_sec;
        b_http.setConnectTimeout(3000);
        b_http.setTimeout(7000);

        bool begun = b_url.startsWith("https://")
                     ? (b_sec.setInsecure(), b_http.begin(b_sec, b_url))
                     : b_http.begin(b_client, b_url);
        if (!begun) return;

        b_http.addHeader("Content-Type", "application/json");
        b_http.addHeader("X-Device-Key", api_key);

        int code = b_http.POST((uint8_t*)json.c_str(), json.length());
        if (code == 200) {
            if (to_send >= state.offline_buf_count) {
                state.clearOfflineSamples();
            } else {
                for (int i = to_send; i < state.offline_buf_count; i++) {
                    state.offline_buf[i - to_send] = state.offline_buf[i];
                }
                state.offline_buf_count -= to_send;
            }
        }
        b_http.end();
    }
    void processServerResponse(const String& json_str) {
        processServerResponse(json_str.c_str(), json_str.length());
    }

    void processServerResponse(const char* json_buf, size_t buf_len) {
        if (!json_buf || buf_len < 2) return;
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, json_buf, buf_len);
        if (err) return;

        state.oracle_manages_bot  = doc["bot_managed"] | false;
        state.has_more_cloud_cmds = doc["has_more"] | false;

        // Статус підключення Spotify від сервера (для майстра Onboarding та плеєра)
        if (doc["sp_linked"].is<bool>()) {
            state.sp_linked = doc["sp_linked"].as<bool>();
        }

        // Обробка тихих годин та режиму DND для Насті
        if (doc["quiet_hours"].is<bool>()) {
            bool qh = doc["quiet_hours"].as<bool>();
            if (qh != state.quiet_hours_active) {
                state.setQuietHours(qh);
            }
        }

        if (doc["dnd"].is<bool>()) {
            bool dnd = doc["dnd"].as<bool>();
            state.dnd_mode = dnd;
            if (dnd) state.audio_muted = true;
        }

        // Обробка найближчої події Google Календаря Насті (Zero-Knowledge Smart Hub)
        if (doc["cal_event"].is<JsonObject>()) {
            JsonObject cal = doc["cal_event"];
            const char* summary = cal["summary"] | "";
            const char* time_str = cal["time"] | (cal["start"] | "");
            bool is_soon = cal["is_soon"] | false;
            char s_dec[36];
            TgJsonUtils::decodeJsonString(summary, s_dec, sizeof(s_dec));
            state.setCalendarEvent(s_dec, time_str, is_soon);
            if (is_soon && !state.dnd_mode && state.light_mode != MODE_LIGHT_OFF) {
                state.triggerLedNotification(NOTIFY_AMBILIGHT, false);
            }
        } else if (doc["cal_event"].is<nullptr_t>()) {
            state.clearCalendarEvent();
        }

        // Обробка Spotify стану від сервера (Zero Heap Allocation)
        if (doc["spotify"].is<JsonObject>()) {
            JsonObject sp = doc["spotify"];
            state.spotify.playing = sp["playing"] | false;

            const char* tr_id = sp["track_id"] | "";
            if (tr_id && tr_id[0] != '\0' && strcmp(state.spotify.track_id, tr_id) != 0) {
                strncpy(state.spotify.track_id, tr_id, sizeof(state.spotify.track_id) - 1);
                state.spotify.vinyl_ready = false;
                state.spotify_vinyl_needed = true;
            }

            const char* title = sp["title"] | "";
            if (title && title[0] != '\0') {
                TgJsonUtils::decodeJsonString(title, state.spotify.title, sizeof(state.spotify.title));
            }

            const char* artist = sp["artist"] | "";
            if (artist && artist[0] != '\0') {
                TgJsonUtils::decodeJsonString(artist, state.spotify.artist, sizeof(state.spotify.artist));
            }

            if (sp["progress_ms"].is<uint32_t>()) state.spotify.progress_ms = sp["progress_ms"].as<uint32_t>();
            if (sp["duration_ms"].is<uint32_t>()) state.spotify.duration_ms = sp["duration_ms"].as<uint32_t>();

            if (sp["ambi_rgb"].is<JsonArray>()) {
                JsonArray rgb = sp["ambi_rgb"];
                if (rgb.size() >= 3) {
                    uint8_t r = rgb[0].as<uint8_t>();
                    uint8_t g = rgb[1].as<uint8_t>();
                    uint8_t b = rgb[2].as<uint8_t>();
                    state.spotify.ambi_r = r; state.spotify.ambi_g = g; state.spotify.ambi_b = b;
                    state.ambi_r = r; state.ambi_g = g; state.ambi_b = b;
                }
            }
            state.spotify.last_sync_ms = millis();
        }

        if (!doc["has_cmd"].as<bool>()) return;
        state.registerNetActivity();

        // 1. Команди світла та яскравості
        if (doc["set_mode"].is<int>()) {
            CommandRouter::setLightMode(state, doc["set_mode"].as<uint8_t>());
        }

        if (!doc["set_brt"].isNull()) {
            if (doc["set_brt"].is<const char*>()) {
                const char* b_str = doc["set_brt"].as<const char*>();
                if (strcmp(b_str, "auto") == 0) {
                    CommandRouter::setBrightnessPreset(state, true);
                } else {
                    CommandRouter::setBrightnessPreset(state, false, (uint8_t)atoi(b_str));
                }
            } else if (doc["set_brt"].is<int>()) {
                CommandRouter::setBrightnessPreset(state, false, doc["set_brt"].as<uint8_t>());
            }
        }

        // 2. Команди звуку: гучність, Mute, тактильні кліки
        if (doc["volume"].is<int>()) {
            uint8_t v = (uint8_t)constrain(doc["volume"].as<int>(), 0, 21);
            CommandRouter::setVolume(state, v);
            if (storage_srv) storage_srv->saveAudioSettings(v, state.audio_muted, state.touch_beep_enabled);
        }

        if (doc["muted"].is<bool>()) {
            bool m = doc["muted"].as<bool>();
            CommandRouter::setMute(state, m);
            if (storage_srv) storage_srv->saveAudioSettings(state.audio_volume, m, state.touch_beep_enabled);
        }

        if (doc["touch_beep"].is<bool>()) {
            bool tb = doc["touch_beep"].as<bool>();
            CommandRouter::setTouchBeep(state, tb);
            if (storage_srv) storage_srv->saveAudioSettings(state.audio_volume, state.audio_muted, tb);
        }

        // 3. Команди керування показом медіа
        if (doc["stop_media"].as<bool>()) {
            CommandRouter::stopActiveMedia(state);
        }

        // 4. Святкове привітання з днем народження
        if (doc["birthday_celebrate"].as<bool>()) {
            state.triggerBirthdayCelebration(20000);
            state.trig_reaction_synth = NOTIFY_CROWN;
            state.light_mode = MODE_SUNSET_VIBE;
        }

        // 5. Керування Pomodoro
        if (doc["pomo_action"].is<const char*>()) {
            const char* act = doc["pomo_action"] | "";
            if (strcmp(act, "stop") == 0) {
                state.pomoStop();
            } else if (strcmp(act, "start") == 0) {
                bool is_brk = doc["pomo_break"] | false;
                int p_min = is_brk ? 5 : 25;
                if (doc["pomo_min"].is<int>()) {
                    p_min = doc["pomo_min"].as<int>();
                } else if (doc["pomo_min"].is<const char*>()) {
                    p_min = atoi(doc["pomo_min"].as<const char*>());
                }
                if (is_brk) {
                    state.pomo_break_duration_min = (uint8_t)constrain(p_min, 1, 30);
                    state.pomo_end_ms = millis() + (uint32_t)state.pomo_break_duration_min * 60000UL;
                } else {
                    state.pomo_duration_min = (uint8_t)constrain(p_min, 5, 90);
                    state.pomo_end_ms = millis() + (uint32_t)state.pomo_duration_min * 60000UL;
                }
                state.screen_mode = SCREEN_POMODORO;
                state.pomo_active = true;
                state.pomo_paused = false;
                state.pomo_is_break = is_brk;
                state.light_mode = is_brk ? MODE_OCEAN_BREEZE : MODE_AMBER_EMBER;
                state.ui_force_dirty = true;
            }
        }

        // 6. Системні команди
        if (doc["sys_cmd"].is<const char*>()) {
            const char* sys_cmd = doc["sys_cmd"] | "";
            if (strcmp(sys_cmd, "reboot") == 0) state.cmd_reboot = true;
            else if (strcmp(sys_cmd, "start_ap") == 0) state.cmd_start_ap = true;
            else if (strcmp(sys_cmd, "clear_cache") == 0) {
                state.cmd_clear_cache = true;
                state.saver_loaded = false;
                state.saver_num_frames = 0;
                state.saver_crc = 0;
                if (storage_srv) storage_srv->clearScreensaver();
            }
        }

        if (doc["media_timer"].is<uint32_t>()) {
            CommandRouter::setMediaTimer(state, doc["media_timer"].as<uint16_t>());
        }

        if (!doc["sleep"].isNull()) {
            uint16_t slp = 0;
            if (doc["sleep"].is<uint16_t>()) {
                slp = doc["sleep"].as<uint16_t>();
            } else if (doc["sleep"].is<const char*>()) {
                slp = (uint16_t)atoi(doc["sleep"].as<const char*>());
            } else if (doc["sleep"].is<int>()) {
                slp = (uint16_t)max(0, doc["sleep"].as<int>());
            }
            CommandRouter::setSleepTimer(state, slp);
        }

        bool has_reaction = false;
        if (!doc["react"].isNull()) {
            uint8_t r = 0;
            if (doc["react"].is<int>()) {
                r = doc["react"].as<uint8_t>();
            } else if (doc["react"].is<const char*>()) {
                r = (uint8_t)atoi(doc["react"].as<const char*>());
            }
            if (r >= NOTIFY_FIRE && r <= NOTIFY_CROWN) {
                const char* r_sender = doc["sender"] | "Telegram";
                const char* r_msg = doc["msg"] | "";
                CommandRouter::triggerReaction(state, (NotifyLedType)r, (r_msg && r_msg[0] != '\0') ? r_sender : nullptr, (r_msg && r_msg[0] != '\0') ? r_msg : nullptr);
                state.trig_reaction_synth = r;
                has_reaction = true;
            }
        }

        if (!has_reaction && doc["msg"].is<const char*>()) {
            const char* raw_msg = doc["msg"] | "";
            if (raw_msg && raw_msg[0] != '\0') {
                const char* raw_sender = doc["sender"] | "Oracle AI";
                char s_utf8[28], m_utf8[96];
                TgJsonUtils::decodeJsonString(raw_sender, s_utf8, sizeof(s_utf8));
                TgJsonUtils::decodeJsonString(raw_msg, m_utf8, sizeof(m_utf8));
                CommandRouter::pushMessage(state, s_utf8, m_utf8);
                if (!doc["silent"].as<bool>()) {
                    state.trig_notify_bell = true;
                }
            }
        }

        if (doc["play_url"].is<const char*>()) {
            String play_url = normalizeMediaUrl(doc["play_url"].as<const char*>());
            if (play_url.length() > 8) state.setPendingAudioUrl(play_url.c_str());
            if (play_url.indexOf("alarm.mp3") != -1 || doc["alarm_active"].as<bool>()) {
                state.alarm_active = true;
                state.alarm_dismissed = false;
                state.alarm_active_until_ms = millis() + 1200000UL;
            }
        }

        bool is_saver = doc["set_saver"] | false;
        if (doc["media_url"].is<const char*>()) {
            String media_url = normalizeMediaUrl(doc["media_url"].as<const char*>());
            const char* media_name = doc["media_name"] | "";
            const char* media_crc = doc["media_crc"] | "";
            if (media_url.length() > 8) {
                state.setPendingMedia(media_url.c_str(), media_name, media_crc, is_saver);
            }
        } else if (doc["img_url"].is<const char*>()) {
            String img_url = normalizeMediaUrl(doc["img_url"].as<const char*>());
            if (img_url.length() > 8) state.setPendingImageUrl(img_url.c_str());
        }

        if (doc["ota_url"].is<const char*>()) {
            String ota_url = normalizeMediaUrl(doc["ota_url"].as<const char*>());
            const char* ota_sha = doc["ota_sha256"] | "";
            if (ota_url.length() > 10) state.setPendingOtaUrl(ota_url.c_str(), ota_sha);
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
        if (state.cmd_spotify_action != 0 || state.cmd_send_tg_ack || state.cmd_send_meme_ping != 0 || state.alarm_dismissed ||
            state.spotify_vinyl_needed || state.hasPendingMedia() || state.hasPendingOta()) {
            return 20;
        }
        if (state.has_more_cloud_cmds) return 150;
        if (ws_connected) return 5000;
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
            if (ws_connected && ws_client) {
                esp_websocket_client_send_text(ws_client, "{\"type\":\"get_vinyl\"}", 20, pdMS_TO_TICKS(50));
                state.spotify_vinyl_needed = false;
            } else {
                state.spotify_vinyl_needed = false;
                downloadSpotifyVinylArt();
            }
        }

        // Захист від навантаження каналу під час I2S аудіо
        if (state.audio_playing && !state.cmd_send_tg_ack && state.cmd_send_meme_ping == 0 && !state.has_more_cloud_cmds) {
            return;
        }

        // ⚡️ Захист від зайвої важкої телеметрії: якщо щойно виконали швидкий pull (вініл/медіа/OTA)
        // і немає реальних дій користувача (кнопки, алярм) — не надсилаємо статус раніше ніж за 4500 мс
        bool has_urgent_events = (state.cmd_spotify_action != 0 || state.cmd_send_tg_ack || state.cmd_send_meme_ping != 0 || state.alarm_dismissed || state.has_more_cloud_cmds);
        if (ws_connected && !has_urgent_events && (millis() - last_ws_send_ms < 4500)) {
            return;
        }

        char status_json[576];
        CommandRouter::buildStatusJson(state, status_json, sizeof(status_json));

        // ⚡️ Пріоритетний швидкісний WebSocket-канал (15 мс latency)
        if (!ws_client && base_server_url.length() > 8) {
            initWebSocketClient();
        }

        if (ws_client && esp_websocket_client_is_connected(ws_client)) {
            ws_connected = true;
            state.oracle_online = true;
            int sent = esp_websocket_client_send_text(ws_client, status_json, strlen(status_json), pdMS_TO_TICKS(350));
            if (sent > 0) {
                last_ws_send_ms = millis();
                if (state.offline_buf_count > 0 && !state.audio_playing) {
                    flushOfflineTelemetry();
                }
                return;
            }
        }

        String req_url = sync_endpoint;
        if (!state.has_more_cloud_cmds && !state.audio_playing) {
            float wait_sec = (state.cmd_spotify_action != 0 || state.btn_l_pressed || state.btn_r_pressed || state.btn_t_pressed) ? 0.2f : 6.0f;
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
                if (state.offline_buf_count > 0 && !state.audio_playing) {
                    flushOfflineTelemetry();
                }
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
