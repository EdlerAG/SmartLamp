#pragma once
#include <Arduino.h>

/**
 * Standalone I2S DMA & Loop Latency Diagnostic Helper.
 * Kept intentionally decoupled from production services so normal firmware builds
 * stay lean (0 bytes RAM / Flash overhead).
 *
 * Quick attach when profiling clicks or DMA jitter:
 *   1. #include "hardware/I2sDiagnostics.h"
 *   2. Before i2s_write(): uint32_t t0 = I2sDiagnostics::beginClick(millis());
 *   3. After  i2s_write(): I2sDiagnostics::endClick(t0, bytes_written, res, getCpuFrequencyMhz(), xPortGetCoreID());
 *   4. In loop() tail:     I2sDiagnostics::recordLoop(loop_start_us);
 *   5. Print or append:    I2sDiagnostics::dumpReport(); / I2sDiagnostics::appendJsonFields(buf, sizeof(buf));
 */
struct I2sDiagSnapshot {
    uint32_t clk_cnt     = 0;
    uint32_t clk_us      = 0;
    uint32_t clk_max_us  = 0;
    size_t   clk_bytes   = 0;
    int32_t  clk_res     = 0;
    uint32_t clk_gap_ms  = 0;
    uint32_t clk_last_ms = 0;
    uint32_t loop_max_ms = 0;
    uint32_t clk_cpu     = 0;
    uint8_t  clk_core    = 0;
};

class I2sDiagnostics {
public:
    static I2sDiagSnapshot& state() {
        static I2sDiagSnapshot s_snap;
        return s_snap;
    }

    static uint32_t beginClick(uint32_t now_ms) {
        I2sDiagSnapshot& s = state();
        s.clk_gap_ms = (s.clk_last_ms > 0) ? (now_ms - s.clk_last_ms) : 0;
        s.clk_last_ms = now_ms;
        s.clk_cnt++;
        return micros();
    }

    static void endClick(uint32_t start_us, size_t bytes_written, esp_err_t res, uint32_t cpu_mhz, BaseType_t core_id) {
        I2sDiagSnapshot& s = state();
        uint32_t dt_us = micros() - start_us;
        s.clk_us = dt_us;
        if (dt_us > s.clk_max_us) {
            s.clk_max_us = dt_us;
        }
        s.clk_bytes = bytes_written;
        s.clk_res = (int32_t)res;
        s.clk_cpu = cpu_mhz;
        s.clk_core = (uint8_t)core_id;
    }

    static void recordLoop(uint32_t loop_start_us) {
        I2sDiagSnapshot& s = state();
        uint32_t dt_ms = (micros() - loop_start_us) / 1000UL;
        if (dt_ms > s.loop_max_ms) {
            s.loop_max_ms = dt_ms;
        }
    }

    static void reset() {
        state() = I2sDiagSnapshot{};
    }

    static void dumpReport() {
        const I2sDiagSnapshot& s = state();
        Serial.printf(
            "[I2sDiag] cnt=%lu last=%lu us max=%lu us bytes=%u res=%ld gap=%lu ms cpu=%lu MHz core=%u loop_max=%lu ms\n",
            (unsigned long)s.clk_cnt,
            (unsigned long)s.clk_us,
            (unsigned long)s.clk_max_us,
            (unsigned int)s.clk_bytes,
            (long)s.clk_res,
            (unsigned long)s.clk_gap_ms,
            (unsigned long)s.clk_cpu,
            (unsigned int)s.clk_core,
            (unsigned long)s.loop_max_ms
        );
    }

    static size_t appendJsonFields(char* out, size_t max_len) {
        if (!out || max_len == 0) return 0;
        const I2sDiagSnapshot& s = state();
        int n = snprintf(
            out, max_len,
            "\"clk_cnt\":%lu,\"clk_us\":%lu,\"clk_max_us\":%lu,\"clk_b\":%u,\"clk_res\":%ld,\"clk_gap\":%lu,\"loop_max_ms\":%lu,\"clk_cpu\":%lu",
            (unsigned long)s.clk_cnt,
            (unsigned long)s.clk_us,
            (unsigned long)s.clk_max_us,
            (unsigned int)s.clk_bytes,
            (long)s.clk_res,
            (unsigned long)s.clk_gap_ms,
            (unsigned long)s.loop_max_ms,
            (unsigned long)s.clk_cpu
        );
        return (n > 0) ? (size_t)n : 0;
    }
};
