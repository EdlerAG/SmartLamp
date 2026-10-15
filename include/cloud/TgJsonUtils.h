#pragma once
#include <Arduino.h>

namespace TgJsonUtils {
    inline void decodeJsonString(const String& in, char* out, size_t max_len) {
        size_t j = 0;
        size_t len = in.length();
        for (size_t i = 0; i < len && j < max_len - 4; i++) {
            if (in[i] == '\\' && i + 1 < len) {
                char next = in[i + 1];
                if (next == 'u' && i + 5 < len) {
                    char hex[5] = { in[i+2], in[i+3], in[i+4], in[i+5], 0 };
                    uint16_t cp = (uint16_t)strtol(hex, nullptr, 16);
                    if (cp < 0x80) {
                        out[j++] = (char)cp;
                    } else if (cp < 0x800) {
                        out[j++] = (char)(0xC0 | (cp >> 6));
                        out[j++] = (char)(0x80 | (cp & 0x3F));
                    } else {
                        out[j++] = (char)(0xE0 | (cp >> 12));
                        out[j++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        out[j++] = (char)(0x80 | (cp & 0x3F));
                    }
                    i += 5;
                } else if (next == 'n' || next == 'r') {
                    out[j++] = ' ';
                    i++;
                } else {
                    out[j++] = next;
                    i++;
                }
            } else {
                out[j++] = in[i];
            }
        }
        out[j] = '\0';
    }

    inline String extractString(const String& json, const char* key, int start_from = 0) {
        String search = String("\"") + key + "\":\"";
        int idx = json.indexOf(search, start_from);
        if (idx == -1) return "";
        idx += search.length();
        int end = idx;
        while (end < (int)json.length()) {
            if (json[end] == '\"' && json[end - 1] != '\\') break;
            end++;
        }
        return json.substring(idx, end);
    }

    inline String extractIntStr(const String& json, const char* key, int start_from = 0) {
        String search = String("\"") + key + "\":";
        int idx = json.indexOf(search, start_from);
        if (idx == -1) return "";
        idx += search.length();
        while (idx < (int)json.length() && json[idx] == ' ') idx++;
        int end = idx;
        while (end < (int)json.length() && (isdigit(json[end]) || json[end] == '-')) end++;
        return json.substring(idx, end);
    }

    inline String urlEncode(const String& str) {
        String encoded = "";
        encoded.reserve(str.length() * 2);
        const char* hex = "0123456789ABCDEF";
        for (size_t i = 0; i < str.length(); i++) {
            uint8_t c = (uint8_t)str[i];
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += (char)c;
            } else {
                encoded += '%';
                encoded += hex[c >> 4];
                encoded += hex[c & 0x0F];
            }
        }
        return encoded;
    }
}