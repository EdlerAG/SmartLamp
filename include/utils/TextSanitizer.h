#pragma once
#include <Arduino.h>

namespace TextSanitizer {
    /**
     * @brief Санітизує вхідний UTF-8 текст:
     * - Зберігає справжні українські літери (і, І, ї, Ї, є, Є, ґ, Ґ) та всю кирилицю
     * - Конвертує типографські лапки та апострофи (’ / ‘ / `) у стандартний '\''
     * - Конвертує парні лапки («, », “, ”, „) у стандартні '"'
     * - Конвертує довгі тире (–, —, ―, −) у стандартний дефіс '-'
     * - Конвертує трикрапку (…) у "..."
     * - Замінює нерозривний пробіл (NBSP) на звичайний ' '
     * - Замінює символи маркерів/крапок (•, ·) на чистий ASCII розділювач '|'
     * - Транслітерує популярні європейські літери з діакритикою (é->e, ö->o, ü->u, ñ->n тощо)
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
                if (c1 == '`') {
                    dst[d_idx++] = '\'';
                } else {
                    dst[d_idx++] = (char)c1;
                }
                s_idx++;
                continue;
            }

            // 2-байтові послідовності UTF-8
            if (c1 == 0xC2) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                if (c2 == 0xA0) { // non-breaking space
                    dst[d_idx++] = ' ';
                    s_idx += 2;
                    continue;
                } else if (c2 == 0xB7) { // middle dot '·' -> '|'
                    dst[d_idx++] = '|';
                    s_idx += 2;
                    continue;
                } else if (c2 == 0xAB || c2 == 0xBB) { // « or » -> '"'
                    dst[d_idx++] = '"';
                    s_idx += 2;
                    continue;
                }
            }

            // Латиниця з діакритикою (Latin-1 Supplement & Extended: C3, C4, C5)
            if (c1 == 0xC3) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                char mapped = 0;
                if ((c2 >= 0x80 && c2 <= 0x86)) mapped = 'A';
                else if ((c2 >= 0xA0 && c2 <= 0xA6)) mapped = 'a';
                else if (c2 == 0x87) mapped = 'C';
                else if (c2 == 0xA7) mapped = 'c';
                else if ((c2 >= 0x88 && c2 <= 0x8B)) mapped = 'E';
                else if ((c2 >= 0xA8 && c2 <= 0xAB)) mapped = 'e';
                else if ((c2 >= 0x8C && c2 <= 0x8F)) mapped = 'I';
                else if ((c2 >= 0xAC && c2 <= 0xAF)) mapped = 'i';
                else if (c2 == 0x91) mapped = 'N';
                else if (c2 == 0xB1) mapped = 'n';
                else if ((c2 >= 0x92 && c2 <= 0x96) || c2 == 0x98) mapped = 'O';
                else if ((c2 >= 0xB2 && c2 <= 0xB6) || c2 == 0xB8) mapped = 'o';
                else if ((c2 >= 0x99 && c2 <= 0x9C)) mapped = 'U';
                else if ((c2 >= 0xB9 && c2 <= 0xBC)) mapped = 'u';
                else if (c2 == 0x9D) mapped = 'Y';
                else if (c2 == 0xBD || c2 == 0xBF) mapped = 'y';
                else if (c2 == 0x9F) mapped = 's'; // ß -> s

                if (mapped != 0) {
                    dst[d_idx++] = mapped;
                    s_idx += 2;
                    continue;
                }
            }

            // Кирилиця (D0 / D1): зберігаємо всі літери повністю, включно з українськими і, І, ї, Ї, є, Є, ґ, Ґ
            if (c1 == 0xD0 || c1 == 0xD1) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                if (c2 != '\0') {
                    if (d_idx + 2 < max_len) {
                        dst[d_idx++] = (char)c1;
                        dst[d_idx++] = (char)c2;
                        s_idx += 2;
                        continue;
                    }
                }
            }

            // 3-байтові послідовності (E2)
            if (c1 == 0xE2) {
                uint8_t c2 = (uint8_t)src[s_idx + 1];
                uint8_t c3 = (uint8_t)src[s_idx + 2];
                if (c2 == 0x80) {
                    // Типографські тире (–, —, ―: U+2013, U+2014, U+2015) -> '-'
                    if (c3 >= 0x92 && c3 <= 0x95) {
                        dst[d_idx++] = '-';
                        s_idx += 3;
                        continue;
                    }
                    // Типографський апостроф ’ (U+2019) або ‘ (U+2018) -> '\''
                    if (c3 == 0x98 || c3 == 0x99) {
                        dst[d_idx++] = '\'';
                        s_idx += 3;
                        continue;
                    }
                    // Лапки “ (U+201C), ” (U+201D), „ (U+201E) -> '"'
                    if (c3 >= 0x9C && c3 <= 0x9E) {
                        dst[d_idx++] = '"';
                        s_idx += 3;
                        continue;
                    }
                    // Bullet '•' (U+2022) -> '|'
                    if (c3 == 0xA2) {
                        dst[d_idx++] = '|';
                        s_idx += 3;
                        continue;
                    }
                    // Трикрапка '…' (U+2026) -> "..."
                    if (c3 == 0xA6) {
                        if (d_idx + 3 < max_len) {
                            dst[d_idx++] = '.';
                            dst[d_idx++] = '.';
                            dst[d_idx++] = '.';
                        } else {
                            dst[d_idx++] = '.';
                        }
                        s_idx += 3;
                        continue;
                    }
                } else if (c2 == 0x88 && c3 == 0x92) {
                    // Математичний мінус '−' (U+2212) -> '-'
                    dst[d_idx++] = '-';
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
