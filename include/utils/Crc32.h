#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * Стандартний IEEE 802.3 CRC32 розрахунок для валідації цілісності файлів MBX5.
 */
class Crc32 {
public:
    static uint32_t calculate(const uint8_t* data, size_t length) {
        if (!data || length == 0) return 0;
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(int32_t)(crc & 1)));
            }
        }
        return ~crc;
    }
};
