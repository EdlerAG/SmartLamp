#pragma once
#include <Arduino.h>

namespace TgJsonUtils {
    // Zero-allocation JSON string decoder directly from const char*
    inline void decodeJsonString(const char* in, char* out, size_t max_len) {
        if (!in || !out || max_len == 0) return;
        size_t j = 0;
        size_t len = strlen(in);
        for (size_t i = 0; i < len && j < max_len - 4; i++) {
            uint8_t b0 = (uint8_t)in[i];
            if ((b0 & 0xF8) == 0xF0) {
                // Direct 4-byte UTF-8 sequence (emojis) - skip
                if (i + 3 < len) i += 3;
                continue;
            }

            if (in[i] == '\\' && i + 1 < len) {
                char next = in[i + 1];
                if (next == 'u' && i + 5 < len) {
                    char hex[5] = { in[i+2], in[i+3], in[i+4], in[i+5], 0 };
                    uint16_t cp = (uint16_t)strtol(hex, nullptr, 16);
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        i += 5;
                        if (i + 6 < len && in[i+1] == '\\' && in[i+2] == 'u') {
                            i += 6;
                        }
                        continue;
                    }

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

        // Protect from trailing broken UTF-8 byte
        while (j > 0 && ((uint8_t)out[j - 1] & 0xC0) == 0x80) {
            j--;
        }
        if (j > 0 && ((uint8_t)out[j - 1] & 0x80) != 0) {
            j--;
        }
        out[j] = '\0';
    }

    // Overload for Arduino String (zero heap allocation on decoding, uses c_str())
    inline void decodeJsonString(const String& in, char* out, size_t max_len) {
        decodeJsonString(in.c_str(), out, max_len);
    }

    inline String urlEncode(const char* str) {
        if (!str) return "";
        size_t len = strlen(str);
        String encoded = "";
        encoded.reserve(len * 2);
        const char* hex = "0123456789ABCDEF";
        for (size_t i = 0; i < len; i++) {
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

    inline String urlEncode(const String& str) {
        return urlEncode(str.c_str());
    }
}
