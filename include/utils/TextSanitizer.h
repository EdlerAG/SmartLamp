#pragma once
#include <Arduino.h>

namespace TextSanitizer {
    /**
     * @brief Санітизує вхідний UTF-8 текст:
     * - Зберігає справжні українські літери (і, І, ї, Ї, є, Є, ґ, Ґ)
     * - Конвертує типографські лапки та апострофи (’ / ‘ / `) у стандартний '\''
     * - Замінює символи маркерів/крапок (•, ·) на чистий ASCII розділювач '|'
     * - Видаляє непідтримувані емодзі (4-байтові та 3-байтові символи поза BMP)
     */
    inline void sanitize(const char* src, char* dst, size_t max_len) {
        if (!src || !dst || max_len == 0) return;
        size_t s_idx = 0;
        size_t d_idx = 0;

        while (src[s_idx] != '\0' && d_idx < max_len - 1) {
            uint8_t c1 = (uint8_t)src[s_idx];

            // 1-байтовий ASCII
            if (c1 < 0x80) {
                // Замінюємо зворотні лапки/гравіс на апостроф
                if (c1 == '`') {
                    dst[d_idx++] = '\'';
                } else {
                    dst[d_idx++] = (char)c1;
                }
                s_idx++;
                continue;
            }

            // 2-байтові послідовності UTF-8 (Кирилиця, середні крапки тощо)
            if (c1 == 0xC2) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                if (c2 == 0xB7) { // middle dot '·' -> '|'
                    dst[d_idx++] = '|';
                    s_idx += 2;
                    continue;
                }
            }

            if (c1 == 0xD0 || c1 == 0xD1) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                if (c2 != '\0') {
                    // Зберігаємо всю кирилицю, включно з українськими і, І, ї, Ї, є, Є, ґ, Ґ
                    if (d_idx + 2 < max_len) {
                        dst[d_idx++] = (char)c1;
                        dst[d_idx++] = (char)c2;
                        s_idx += 2;
                        continue;
                    }
                }
            }

            // 3-байтові послідовності
            if (c1 == 0xE2) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                uint8_t c3 = (uint8_t)src[s_idx + 2];
                // Типографський апостроф ’ (U+2019) або ‘ (U+2018) -> '\''
                if (c2 == 0x80 && (c3 == 0x98 || c3 == 0x99)) {
                    dst[d_idx++] = '\'';
                    s_idx += 3;
                    continue;
                }
                // Bullet '•' (U+2022: E2 80 A2) -> '|'
                if (c2 == 0x80 && c3 == 0xA2) {
                    dst[d_idx++] = '|';
                    s_idx += 3;
                    continue;
                }
                // Інші 3-байтові символи (наприклад, емодзі ✨ U+2728 тощо) пропускаємо
                s_idx += 3;
                continue;
            }

            // 4-байтові послідовності (емодзі: 0xF0..0xF4)
            if (c1 >= 0xF0 && c1 <= 0xF4) {
                s_idx += 4;
                continue;
            }

            // Якщо невідомий символ поза діапазоном, пропускаємо 1 байт
            s_idx++;
        }

        dst[d_idx] = '\0';
    }
}
