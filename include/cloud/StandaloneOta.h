#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

/**
 * ============================================================================
 * Standalone Remote Firmware Flasher (v3.8 Hallmark & Standalone OTA Edition)
 * Zero-dependency, single-header production module for ESP32-S3.
 * ============================================================================
 */

using OtaProgressCallback = void(*)(uint8_t percent, const char* status_msg);

class StandaloneOta {
public:
    static void confirmBootValid() {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            Serial.println("[StandaloneOTA] Boot partition confirmed valid (rollback cancelled).");
        } else {
            Serial.printf("[StandaloneOTA] confirmBootValid warning: 0x%X\n", err);
        }
    }

    static bool canRollback() {
        return Update.canRollBack();
    }

    static bool rollback(String* err_out = nullptr) {
        if (!Update.canRollBack()) {
            if (err_out) *err_out = "Попередній розділ Flash відсутній.";
            return false;
        }
        if (Update.rollBack()) {
            return true;
        }
        if (err_out) *err_out = "Помилка Update.rollBack().";
        return false;
    }

    static String getPartitionInfo() {
        const esp_partition_t* running = esp_ota_get_running_partition();
        char buf[96];
        snprintf(buf, sizeof(buf), "%s (0x%08X)",
                 running ? running->label : "app",
                 running ? running->address : 0);
        return String(buf);
    }

    static bool flashFromUrl(const char* url,
                            const char* expected_sha256 = nullptr,
                            OtaProgressCallback cb = nullptr,
                            String* err_out = nullptr,
                            bool auto_restart = true) {
        if (!url || strlen(url) < 10) {
            if (err_out) *err_out = "Порожній або занадто короткий URL прошивки.";
            return false;
        }

        String str_url(url);
        str_url.trim();
        bool is_https = str_url.startsWith("https://");
        if (!str_url.startsWith("http://") && !is_https) {
            if (err_out) *err_out = "URL має починатися з http:// або https://";
            return false;
        }

        if (WiFi.status() != WL_CONNECTED) {
            if (err_out) *err_out = "Wi-Fi не підключено.";
            return false;
        }

        if (cb) cb(0, "Підключення до сервера...");

        WiFiClientSecure sec_client;
        WiFiClient       net_client;
        HTTPClient       http;

        if (is_https) {
            sec_client.setInsecure();
            sec_client.setTimeout(20);
            sec_client.setNoDelay(true);
        } else {
            net_client.setNoDelay(true);
        }

        http.setConnectTimeout(10000);
        http.setTimeout(25000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        bool begun = is_https ? http.begin(sec_client, str_url) : http.begin(net_client, str_url);
        if (!begun) {
            if (err_out) *err_out = "Не вдалося ініціалізувати HTTP-з'єднання.";
            return false;
        }

        int http_code = http.GET();
        if (http_code != HTTP_CODE_OK) {
            http.end();
            if (err_out) *err_out = "Помилка HTTP сервера: " + String(http_code);
            return false;
        }

        int total_len = http.getSize();
        if (total_len > 0 && total_len < 150000) {
            http.end();
            if (err_out) *err_out = "Файл замалий для прошивки ESP32 (" + String(total_len) + " Б).";
            return false;
        }

        constexpr size_t CHUNK_SIZE = 4096;
        uint8_t* buff = (uint8_t*)(psramFound() ? ps_malloc(CHUNK_SIZE) : malloc(CHUNK_SIZE));
        if (!buff) {
            http.end();
            if (err_out) *err_out = "Недостатньо RAM/PSRAM для буфера OTA.";
            return false;
        }

        // Ініціалізація SHA256 контексту для контролю цілісності на льоту
        mbedtls_sha256_context sha_ctx;
        mbedtls_sha256_init(&sha_ctx);
        mbedtls_sha256_starts(&sha_ctx, 0); // 0 = SHA-256

        WiFiClient* stream = http.getStreamPtr();
        size_t written = 0;
        bool header_checked = false;
        uint32_t last_data_ms = millis();
        uint32_t last_yield_ms = millis();
        uint8_t last_pct = 0;

        if (cb) cb(1, "Перевірка заголовка...");

        while (http.connected() && (total_len > 0 ? (written < (size_t)total_len) : true)) {
            size_t avail = stream->available();
            if (avail > 0) {
                last_data_ms = millis();
                size_t to_read = (avail > CHUNK_SIZE) ? CHUNK_SIZE : avail;
                int c = stream->readBytes(buff, to_read);
                if (c <= 0) break;

                // Перевірка магічного байта 0xE9 заголовка ESP32 у першому блоці
                if (!header_checked) {
                    header_checked = true;
                    if (buff[0] != 0xE9) {
                        mbedtls_sha256_free(&sha_ctx);
                        http.end();
                        free(buff);
                        if (err_out) *err_out = "Файл не є прошивкою ESP32 (немає магічного байта 0xE9).";
                        return false;
                    }

                    if (!Update.begin(total_len > 0 ? total_len : UPDATE_SIZE_UNKNOWN)) {
                        mbedtls_sha256_free(&sha_ctx);
                        http.end();
                        free(buff);
                        if (err_out) *err_out = "Update.begin failed (недостатньо місця у розділі).";
                        return false;
                    }
                }

                mbedtls_sha256_update(&sha_ctx, buff, c);

                if (Update.write(buff, c) != (size_t)c) {
                    Update.abort();
                    mbedtls_sha256_free(&sha_ctx);
                    http.end();
                    free(buff);
                    if (err_out) *err_out = "Помилка запису у Flash-пам'ять.";
                    return false;
                }

                written += c;
                if (total_len > 0) {
                    uint8_t pct = (uint8_t)(((uint64_t)written * 100ULL) / (uint64_t)total_len);
                    if (pct != last_pct) {
                        last_pct = pct;
                        if (cb) cb(pct, "Запис у Flash...");
                    }
                }

                if (millis() - last_yield_ms >= 12) {
                    last_yield_ms = millis();
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            } else {
                if (millis() - last_data_ms > 15000) {
                    Update.abort();
                    mbedtls_sha256_free(&sha_ctx);
                    http.end();
                    free(buff);
                    if (err_out) *err_out = "Тайм-аут завантаження (15 с без даних).";
                    return false;
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            }
        }

        http.end();
        free(buff);

        // Фіналізація та порівняння SHA256
        uint8_t sha_bytes[32];
        mbedtls_sha256_finish(&sha_ctx, sha_bytes);
        mbedtls_sha256_free(&sha_ctx);

        char calc_sha[65];
        for (int i = 0; i < 32; i++) {
            snprintf(calc_sha + i * 2, 3, "%02x", sha_bytes[i]);
        }
        calc_sha[64] = '\0';

        if (expected_sha256 && strlen(expected_sha256) >= 32) {
            String exp_s(expected_sha256);
            exp_s.trim();
            exp_s.toLowerCase();
            String calc_s(calc_sha);
            calc_s.toLowerCase();

            if (!calc_s.startsWith(exp_s) && !exp_s.startsWith(calc_s)) {
                Update.abort();
                if (err_out) {
                    *err_out = "Незбіг SHA256! Розраховано: " + calc_s.substring(0, 12) +
                               "... Очікувалось: " + exp_s.substring(0, 12) + "...";
                }
                return false;
            }
        }

        if (!Update.end(true) || Update.hasError()) {
            if (err_out) *err_out = "Помилка фіналізації Flash (Update.end failed).";
            return false;
        }

        if (cb) cb(100, "Оновлено успішно! Перезавантаження...");
        Serial.printf("[StandaloneOTA] Successfully flashed %u bytes. SHA256: %s\n", written, calc_sha);

        if (auto_restart) {
            vTaskDelay(pdMS_TO_TICKS(600));
            ESP.restart();
        }
        return true;
    }

    static bool checkAndFlashFromTelegram(const char* bot_token,
                                         const char* admin_id,
                                         OtaProgressCallback cb = nullptr,
                                         String* err_out = nullptr) {
        if (!bot_token || strlen(bot_token) < 15) {
            if (err_out) *err_out = "Невірний TG_BOT_TOKEN.";
            return false;
        }
        if (WiFi.status() != WL_CONNECTED) {
            if (err_out) *err_out = "Wi-Fi не підключено.";
            return false;
        }

        WiFiClientSecure sec_client;
        sec_client.setInsecure();
        sec_client.setTimeout(12);
        HTTPClient http;

        String get_updates_url = "https://api.telegram.org/bot" + String(bot_token) + "/getUpdates?limit=5&offset=-1";
        if (!http.begin(sec_client, get_updates_url)) {
            if (err_out) *err_out = "Не вдалося з'єднатися з Telegram API.";
            return false;
        }

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            if (err_out) *err_out = "Telegram getUpdates HTTP " + String(code);
            return false;
        }

        String json = http.getString();
        http.end();

        // Пошук документа .bin або команди /ota від admin_id
        if (admin_id && strlen(admin_id) > 0 && json.indexOf(admin_id) == -1) {
            if (err_out) *err_out = "Немає повідомлень від адміністратора.";
            return false;
        }

        int doc_idx = json.lastIndexOf("\"document\"");
        if (doc_idx == -1) {
            if (err_out) *err_out = "У Telegram не знайдено прикріплених документів.";
            return false;
        }

        String doc_part = json.substring(doc_idx);
        int fn_idx = doc_part.indexOf("\"file_name\":\"");
        if (fn_idx == -1) {
            if (err_out) *err_out = "Не знайдено file_name.";
            return false;
        }

        int fn_end = doc_part.indexOf("\"", fn_idx + 13);
        String file_name = doc_part.substring(fn_idx + 13, fn_end);
        file_name.toLowerCase();

        if (!file_name.endsWith(".bin")) {
            if (err_out) *err_out = "Знайдений документ не має розширення .bin (" + file_name + ")";
            return false;
        }

        int fid_idx = doc_part.indexOf("\"file_id\":\"");
        if (fid_idx == -1) {
            if (err_out) *err_out = "Не знайдено file_id у документі.";
            return false;
        }

        int fid_end = doc_part.indexOf("\"", fid_idx + 11);
        String file_id = doc_part.substring(fid_idx + 11, fid_end);

        // Отримуємо file_path через getFile
        String get_file_url = "https://api.telegram.org/bot" + String(bot_token) + "/getFile?file_id=" + file_id;
        if (!http.begin(sec_client, get_file_url)) {
            if (err_out) *err_out = "Не вдалося викликати getFile.";
            return false;
        }

        int gf_code = http.GET();
        if (gf_code != HTTP_CODE_OK) {
            http.end();
            if (err_out) *err_out = "getFile HTTP " + String(gf_code);
            return false;
        }

        String gf_json = http.getString();
        http.end();

        int fp_idx = gf_json.indexOf("\"file_path\":\"");
        if (fp_idx == -1) {
            if (err_out) *err_out = "У відповіді getFile відсутній file_path.";
            return false;
        }

        int fp_end = gf_json.indexOf("\"", fp_idx + 13);
        String file_path = gf_json.substring(fp_idx + 13, fp_end);

        String download_url = "https://api.telegram.org/file/bot" + String(bot_token) + "/" + file_path;
        Serial.println("[StandaloneOTA] Downloading TG firmware: " + download_url);

        return flashFromUrl(download_url.c_str(), nullptr, cb, err_out, true);
    }

    static void tick() {
        // Допоміжний неблокуючий тік для інтеграції в цикл loop()
    }
};
