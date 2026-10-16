#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include <Audio.h>

class AudioService : public IService {
private:
    AppState& state;
    Audio     audio;
    uint8_t   applied_volume = 0;

public:
    AudioService(AppState& st) : state(st) {}

    const char* getName() const override { return "AudioI2S"; }

    void init() override {
        audio.setPinout(Config::PIN_I2S_BCLK, Config::PIN_I2S_LRC, Config::PIN_I2S_DOUT);
        applied_volume = state.audio_volume;
        audio.setVolume(applied_volume);
    }

    bool isPlaying() {
        return audio.isRunning();
    }

    void update() override {
        if (state.ota_in_progress) {
            if (audio.isRunning()) audio.stopSong();
            return;
        }

        if (applied_volume != state.audio_volume) {
            applied_volume = constrain(state.audio_volume, (uint8_t)0, (uint8_t)21);
            audio.setVolume(applied_volume);
        }

        char url_buf[160];
        if (state.popPendingAudioUrl(url_buf, sizeof(url_buf))) {
            if (state.wifi_connected) {
                audio.stopSong();
                audio.connecttohost(url_buf);
            }
        }

        if (audio.isRunning()) {
            audio.loop();
        }
    }
};