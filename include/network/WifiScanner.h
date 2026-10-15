#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WifiScanner {
private:
    char json_cache[1536];
    bool has_cache = false;

    // Очищення SSID від небезпечних символів, що ламають JSON
    static void sanitizeSsid(const String& raw, char* out, size_t max_len) {
        size_t j = 0;
        for (size_t i = 0; i < raw.length() && j < max_len - 1; i++) {
            char c = raw[i];
            if (c == '\"' || c == '\\' || (uint8_t)c < 0x20) continue;
            out[j++] = c;
        }
        out[j] = '\0';
    }

public:
    WifiScanner() {
        strcpy(json_cache, "{\"scanning\":false,\"networks\":[]}");
    }

    bool hasCachedResults() const { return has_cache; }
    const char* getCachedJson() const { return json_cache; }
    bool isScanning() const { return WiFi.scanComplete() == WIFI_SCAN_RUNNING; }

    void performFastPreScan() {
        int n = WiFi.scanNetworks(false, false, false, 45);
        buildJsonFromResults(n);
    }

    void triggerAsyncScan() {
        if (!isScanning()) {
            WiFi.scanNetworks(true, false, false, 50);
        }
    }

    bool checkAsyncScan() {
        int status = WiFi.scanComplete();
        if (status >= 0) {
            buildJsonFromResults(status);
            return true;
        }
        return false;
    }

private:
    void buildJsonFromResults(int n) {
        if (n <= 0) {
            strcpy(json_cache, "{\"scanning\":false,\"networks\":[]}");
            WiFi.scanDelete();
            return;
        }

        int count = min(n, 24);
        int idx[24];
        for (int i = 0; i < count; i++) idx[i] = i;

        // Insertion sort по RSSI (менше swap-операцій ніж bubble sort для малих n)
        for (int i = 1; i < count; i++) {
            int key = idx[i];
            int j = i - 1;
            while (j >= 0 && WiFi.RSSI(idx[j]) < WiFi.RSSI(key)) {
                idx[j + 1] = idx[j];
                j--;
            }
            idx[j + 1] = key;
        }

        size_t pos = snprintf(json_cache, sizeof(json_cache), "{\"scanning\":false,\"networks\":[");
        bool first = true;

        for (int i = 0; i < count; i++) {
            char safe_ssid[36];
            sanitizeSsid(WiFi.SSID(idx[i]), safe_ssid, sizeof(safe_ssid));
            if (safe_ssid[0] == '\0') continue;

            // Дедублікація через char* strcmp — без String heap-алокацій
            bool dup = false;
            for (int k = 0; k < i; k++) {
                char prev[36];
                sanitizeSsid(WiFi.SSID(idx[k]), prev, sizeof(prev));
                if (strcmp(prev, safe_ssid) == 0) { dup = true; break; }
            }
            if (dup) continue;

            int written = snprintf(
                json_cache + pos, sizeof(json_cache) - pos - 4,
                "%s{\"ssid\":\"%s\",\"rssi\":%d,\"enc\":%s}",
                first ? "" : ",",
                safe_ssid,
                (int)WiFi.RSSI(idx[i]),
                (WiFi.encryptionType(idx[i]) != WIFI_AUTH_OPEN) ? "true" : "false"
            );

            if (written > 0 && (pos + written < sizeof(json_cache) - 4)) {
                pos += written;
                first = false;
            } else {
                break;
            }
        }

        snprintf(json_cache + pos, sizeof(json_cache) - pos, "]}");
        has_cache = true;
        WiFi.scanDelete();
    }
};