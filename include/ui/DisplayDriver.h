#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "Config.h"

class LGFX_GC9A01 : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
public:
    LGFX_GC9A01(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false; // Використовується виділений пін DC (4-wire SPI)
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = Config::PIN_TFT_SCLK;
            cfg.pin_mosi = Config::PIN_TFT_MOSI;
            cfg.pin_miso = -1;
            cfg.pin_dc   = Config::PIN_TFT_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = Config::PIN_TFT_CS;
            cfg.pin_rst          = Config::PIN_TFT_RST;
            cfg.pin_busy         = -1;
            cfg.memory_width     = 240;
            cfg.memory_height    = 240;
            cfg.panel_width      = 240;
            cfg.panel_height     = 240;
            cfg.invert           = true;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};