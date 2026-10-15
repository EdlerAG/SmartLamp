#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include "Config.h"
#include "AppState.h"

class CloudOtaService {
private:
    // RAII-гард: гарантує скидання прапорців навіть при помилці чи обриві мережі
    struct OtaStateGuard {
        AppState& st;
        OtaStateGuard(AppState& s) : st(s) {
            st.tls_in_flight = true;
            WiFi.setSleep(false);
        }
        ~OtaStateGuard() {
            st.ota_in_progress = false;
            st.tls_in_flight   = false;
            WiFi.setSleep(WIFI_PS_MIN_MODEM);
            st.registerActivity();
        }
    };

public:
    static String getPartitionInfo() {
        const esp_partition_t* running = esp_ota_get_running_partition();
        char buf[96];
        snprintf(buf, sizeof(buf), "%s (%s)",
                 Config::FW_VERSION,
                 running ? running->label : "app");
        return String(buf);
    }

    static bool rollbackToPrevious(String& err_msg) {
        if (!Update.canRollBack()) {
            err_msg = "Попередня прошивка відсутня у другому слоті Flash.";
            return false;
        }
        if (Update.rollBack()) {
            return true;
        }
        err_msg = "Не вдалося перемкнути завантажувальний розділ.";
        return false;
    }

    static bool flashFromUrl(AppState& state, const String& url, String& err_msg) {
        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            err_msg = "URL має починатися з http:// або https://";
            return false;
        }

        OtaStateGuard guard(state);

        // Виділяємо 4 КБ буфер у PSRAM (або Heap), щоб не навантажувати стек потоку FreeRTOS
        constexpr size_t BUF_SIZE = 4096;
        uint8_t* buff = (uint8_t*)(psramFound() ? ps_malloc(BUF_SIZE) : malloc(BUF_SIZE));
        if (!buff) {
            err_msg = "Помилка виділення пам'яті буфера OTA.";
            return false;
        }

        WiFiClientSecure secure_client;
        WiFiClient       plain_client;
        HTTPClient       http;

        bool is_https = url.startsWith("https://");
        if (is_https) {
            secure_client.setInsecure();
            secure_client.setTimeout(15);
        }

        http.setConnectTimeout(10000);
        http.setTimeout(15000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        bool begun = is_https ? http.begin(secure_client, url) : http.begin(plain_client, url);
        if (!begun) {
            free(buff);
            err_msg = "Не вдалося ініціалізувати HTTP-клієнт.";
            return false;
        }

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            free(buff);
            err_msg = "HTTP помилка сервера: " + String(code);
            return false;
        }

        int total_len = http.getSize();
        if (total_len > 0 && total_len < 150000) {
            http.end();
            free(buff);
            err_msg = "Файл замалий для прошивки (" + String(total_len) + " Б).";
            return false;
        }

        if (!Update.begin(total_len > 0 ? total_len : UPDATE_SIZE_UNKNOWN)) {
            http.end();
            free(buff);
            err_msg = "Помилка Update.begin (недостатньо місця).";
            return false;
        }

        state.ota_in_progress = true;
        state.ota_progress    = 0;
        state.registerActivity();

        WiFiClient* stream = http.getStreamPtr();
        int      written        = 0;
        bool     header_checked = false;
        uint32_t last_data_ms   = millis();
        uint32_t last_yield_ms  = millis();

        while (http.connected() && (total_len > 0 ? (written < total_len) : true)) {
            size_t avail = stream->available();
            if (avail > 0) {
                last_data_ms = millis();
                size_t to_read = (avail > BUF_SIZE) ? BUF_SIZE : avail;
                int c = stream->readBytes(buff, to_read);
                if (c <= 0) break;

                if (!header_checked) {
                    header_checked = true;
                    if (buff[0] != 0xE9) {
                        Update.abort();
                        http.end();
                        free(buff);
                        err_msg = "Файл не є прошивкою ESP32 (немає заголовка 0xE9).";
                        return false;
                    }
                }

                if (Update.write(buff, c) != (size_t)c) {
                    Update.abort();
                    http.end();
                    free(buff);
                    err_msg = "Помилка запису сектора Flash.";
                    return false;
                }

                written += c;
                if (total_len > 0) {
                    uint8_t pct = (uint8_t)(((uint64_t)written * 100ULL) / (uint64_t)total_len);
                    if (pct != state.ota_progress) {
                        state.ota_progress = pct;
                        state.registerActivity();
                    }
                }

                // Віддаємо квант часу іншим потокам лише раз на 15 мс (максимальна швидкість скачування!)
                if (millis() - last_yield_ms >= 15) {
                    last_yield_ms = millis();
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            } else {
                if (millis() - last_data_ms > 12000) {
                    Update.abort();
                    http.end();
                    free(buff);
                    err_msg = "Обрив з'єднання під час завантаження (тайм-аут 12 с).";
                    return false;
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            }
        }

        http.end();
        free(buff);

        if (!Update.end(true) || Update.hasError()) {
            err_msg = "Контрольна сума файлу не збігається.";
            return false;
        }

        state.ota_progress = 100;
        return true;
    }
};