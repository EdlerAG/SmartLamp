#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>

class SensorService : public IService {
private:
    AppState& state;
    Adafruit_BMP280 bmp280;

    uint8_t  sht_addr         = 0x44;
    bool     sht_waiting_data = false;
    uint32_t sht_cmd_ms       = 0;
    uint32_t last_climate_ms  = 0;
    uint32_t last_ldr_ms      = 0;
    uint32_t last_hist_ms     = 0;
    bool     first_hist_saved = false;
    float    led_heat_accumulator = 0.0f;

    bool probeSht31(uint8_t addr) {
        Wire.beginTransmission(addr);
        return (Wire.endTransmission() == 0);
    }

    static uint8_t crc8(const uint8_t* data, int len) {
        uint8_t crc = 0xFF;
        for (int j = len; j > 0; --j) {
            crc ^= *data++;
            for (int i = 8; i > 0; --i) {
                crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
            }
        }
        return crc;
    }

public:
    SensorService(AppState& st) : state(st) {}

    const char* getName() const override { return "Sensors"; }

    void init() override {
        analogReadResolution(12);
        analogSetPinAttenuation(Config::PIN_LDR, ADC_11db);

        Wire.begin(Config::PIN_I2C_SDA, Config::PIN_I2C_SCL, 400000);
        Wire.setTimeOut(25);

        if (bmp280.begin(0x76) || bmp280.begin(0x77)) {
            state.bmp_connected = true;
            bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL,
                               Adafruit_BMP280::SAMPLING_X1,
                               Adafruit_BMP280::SAMPLING_X8,
                               Adafruit_BMP280::FILTER_X8,
                               Adafruit_BMP280::STANDBY_MS_1000);
        }

        if (probeSht31(0x44)) {
            sht_addr = 0x44;
            state.sht_connected = true;
        } else if (probeSht31(0x45)) {
            sht_addr = 0x45;
            state.sht_connected = true;
        }
    }

    void update() override {
        uint32_t now = millis();

        // Адаптивне опитування: під час DND/тихих годин знижуємо частоту опитування для 0% саморозігріву
        bool is_quiet = state.dnd_mode || state.quiet_hours_active;
        uint32_t ldr_interval = is_quiet ? 1000 : (state.auto_brt ? 60 : 300);
        if (now - last_ldr_ms >= ldr_interval) {
            last_ldr_ms = now;
            int raw = analogRead(Config::PIN_LDR);
            state.raw_ldr = raw;
            state.smooth_ldr = (state.smooth_ldr * 0.85f) + (raw * 0.15f);
            int new_pct = constrain((int)((state.smooth_ldr / 4095.0f) * 100.0f), 0, 100);
            if (abs(new_pct - state.light_percent) >= 2) {
                state.light_percent = new_pct;
                state.ui_force_dirty = true;
            }
        }

        if (sht_waiting_data && (now - sht_cmd_ms >= 20)) {
            sht_waiting_data = false;
            if (Wire.requestFrom(sht_addr, (uint8_t)6) == 6) {
                uint8_t buf[6];
                for (int i = 0; i < 6; i++) buf[i] = Wire.read();

                if (crc8(&buf[0], 2) == buf[2] && crc8(&buf[3], 2) == buf[5]) {
                    uint16_t rawT = ((uint16_t)buf[0] << 8) | buf[1];
                    uint16_t rawH = ((uint16_t)buf[3] << 8) | buf[4];

                    float dynamic_offset = Config::TEMP_OFFSET_C - (led_heat_accumulator * 1.5f);
                    float t = (-45.0f + 175.0f * ((float)rawT / 65535.0f)) + dynamic_offset;
                    float h = 100.0f * ((float)rawH / 65535.0f);

                    if (t > -40.0f && t < 85.0f && fabs(t - state.temperature) >= 0.05f) {
                        state.temperature = t;
                        state.ui_force_dirty = true;
                    }
                    if (h >= 0.0f && h <= 100.0f && fabs(h - state.humidity) >= 0.5f) {
                        state.humidity = h;
                        state.ui_force_dirty = true;
                    }
                }
            }
        }

        uint32_t climate_interval = is_quiet ? 12000 : 2500;
        if (now - last_climate_ms >= climate_interval) {
            last_climate_ms = now;

            // Динамічна компенсація теплового розігріву корпусу від світлодіодів
            float target_heat = (state.light_mode != MODE_LIGHT_OFF)
                                ? ((float)state.current_led_brt / (float)Config::MAX_SAFE_BRT)
                                : 0.0f;
            led_heat_accumulator = (led_heat_accumulator * 0.985f) + (target_heat * 0.015f);

            if (state.sht_connected && !sht_waiting_data) {
                Wire.beginTransmission(sht_addr);
                Wire.write(0x24);
                Wire.write(0x00);
                if (Wire.endTransmission() == 0) {
                    sht_waiting_data = true;
                    sht_cmd_ms = now;
                }
            }

            if (state.bmp_connected) {
                float p = bmp280.readPressure();
                if (!isnan(p) && p > 80000.0f && p < 115000.0f) {
                    float mm = p * 0.00750062f;
                    if (fabs(mm - state.pressure) >= 0.2f) {
                        state.pressure = mm;
                        state.ui_force_dirty = true;
                    }
                    if (!first_hist_saved || (now - last_hist_ms >= 900000UL)) {
                        first_hist_saved = true;
                        last_hist_ms = now;
                        state.addPressureSample(mm);
                    }
                }
            }
        }
    }
};