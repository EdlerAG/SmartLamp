#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include <WiFi.h>
#include <Audio.h>
#include <driver/i2s.h>
#include <math.h>

class AudioService : public IService {
private:
    AppState& state;
    Audio     audio;
    uint8_t   applied_volume = 0;
    bool      applied_muted  = false;

    // Стан процедурного синтезатора звуків
    enum class SynthType : uint8_t {
        NONE = 0,
        TOUCH_CLICK,
        BOOT_CHORD,
        NOTIFY_BELL,
        REACTION_FIRE,
        REACTION_ZAP,
        REACTION_CAT,
        REACTION_UFO,
        REACTION_CROWN
    };

    SynthType cur_synth        = SynthType::NONE;
    uint32_t  synth_sample_idx = 0;
    uint32_t  synth_total_samples = 0;
    float     synth_phase1     = 0.0f;
    float     synth_phase2     = 0.0f;
    float     synth_phase3     = 0.0f;

    static constexpr uint32_t SAMPLE_RATE = 44100;
    static constexpr size_t CHUNK_SAMPLES = 128; // 128 стерео-семплів (512 байт) на тик

    void startSynth(SynthType type) {
        // Якщо увімкнено режим тиші або гучність на нулі — жодних звуків (100% тиша)
        if (state.audio_muted || state.audio_volume == 0) {
            cur_synth = SynthType::NONE;
            return;
        }

        // Якщо кліки вимкнені у налаштуваннях
        if (type == SynthType::TOUCH_CLICK && !state.touch_beep_enabled) {
            return;
        }

        if (audio.isRunning()) return; // Якщо грає стрім MP3 — не перебиваємо його
        i2s_start(I2S_NUM_0);
        cur_synth = type;
        synth_sample_idx = 0;
        synth_phase1 = 0.0f;
        synth_phase2 = 0.0f;
        synth_phase3 = 0.0f;

        switch (type) {
            case SynthType::TOUCH_CLICK:
                synth_total_samples = (SAMPLE_RATE * 6) / 1000;    // 6 мс (~265 семплів)
                break;
            case SynthType::BOOT_CHORD:
                synth_total_samples = (SAMPLE_RATE * 750) / 1000;  // 750 мс
                break;
            case SynthType::NOTIFY_BELL:
                synth_total_samples = (SAMPLE_RATE * 360) / 1000;  // 360 мс
                break;
            case SynthType::REACTION_FIRE:
                synth_total_samples = (SAMPLE_RATE * 280) / 1000;  // 280 мс
                break;
            case SynthType::REACTION_ZAP:
                synth_total_samples = (SAMPLE_RATE * 180) / 1000;  // 180 мс
                break;
            case SynthType::REACTION_CAT:
                synth_total_samples = (SAMPLE_RATE * 420) / 1000;  // 420 мс
                break;
            case SynthType::REACTION_UFO:
                synth_total_samples = (SAMPLE_RATE * 500) / 1000;  // 500 мс
                break;
            case SynthType::REACTION_CROWN:
                synth_total_samples = (SAMPLE_RATE * 480) / 1000;  // 480 мс
                break;
            default:
                synth_total_samples = 0;
                break;
        }
    }

    void tickProceduralSynth() {
        if (cur_synth == SynthType::NONE || synth_total_samples == 0) return;
        if (state.audio_muted || state.audio_volume == 0) {
            cur_synth = SynthType::NONE;
            return;
        }

        int16_t pcm_buf[CHUNK_SAMPLES * 2]; // Лівий та правий канали
        size_t samples_to_gen = min((size_t)(synth_total_samples - synth_sample_idx), CHUNK_SAMPLES);
        if (samples_to_gen == 0) {
            cur_synth = SynthType::NONE;
            return;
        }

        // Квадратичне масштабування гучності для природного людського слуху
        float norm_vol = (float)constrain((int)state.audio_volume, 0, 21) / 21.0f;
        float vol_gain = norm_vol * norm_vol; // Наприклад, на vol=4 це 0.036 (дуже тихий шепіт), на vol=21 це 1.0 (гучно)
        constexpr float TWO_PI_F = 6.283185307f;

        for (size_t i = 0; i < samples_to_gen; i++) {
            uint32_t n = synth_sample_idx + i;
            float t = (float)n / (float)SAMPLE_RATE;
            float sample_val = 0.0f;

            switch (cur_synth) {
                case SynthType::TOUCH_CLICK: {
                    // Делікатний 6 мс тактильний клік (експоненційний спад частоти 180 -> 90 Гц)
                    float progress = (float)n / (float)synth_total_samples;
                    float freq = 180.0f - (90.0f * progress);
                    synth_phase1 += (freq * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    float env = expf(-progress * 6.5f);
                    sample_val = sinf(synth_phase1) * env * 0.40f;
                    break;
                }

                case SynthType::BOOT_CHORD: {
                    // Теплий мажорний септакорд C-E-G-B (261.6Hz, 329.6Hz, 392.0Hz, 493.9Hz)
                    float progress = (float)n / (float)synth_total_samples;
                    synth_phase1 += (261.63f * TWO_PI_F) / (float)SAMPLE_RATE;
                    synth_phase2 += (329.63f * TWO_PI_F) / (float)SAMPLE_RATE;
                    synth_phase3 += (392.00f * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    if (synth_phase2 > TWO_PI_F) synth_phase2 -= TWO_PI_F;
                    if (synth_phase3 > TWO_PI_F) synth_phase3 -= TWO_PI_F;

                    float env = (progress < 0.08f) ? (progress / 0.08f) : expf(-(progress - 0.08f) * 3.8f);
                    sample_val = (sinf(synth_phase1) * 0.45f +
                                  sinf(synth_phase2) * 0.35f +
                                  sinf(synth_phase3) * 0.30f) * env * 0.85f;
                    break;
                }

                case SynthType::NOTIFY_BELL: {
                    // Елегантний хорологічний дзвіночок A5 (880 Гц) + обертон E6 (1318 Гц)
                    float progress = (float)n / (float)synth_total_samples;
                    synth_phase1 += (880.0f * TWO_PI_F) / (float)SAMPLE_RATE;
                    synth_phase2 += (1318.5f * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    if (synth_phase2 > TWO_PI_F) synth_phase2 -= TWO_PI_F;

                    float env = (progress < 0.03f) ? (progress / 0.03f) : expf(-(progress - 0.03f) * 7.5f);
                    sample_val = (sinf(synth_phase1) * 0.75f + sinf(synth_phase2) * 0.35f) * env * 0.85f;
                    break;
                }

                case SynthType::REACTION_FIRE: {
                    // Теплий ламповий синтез розпалювання вогню (широкий шум + суб-бас 90 Гц)
                    float progress = (float)n / (float)synth_total_samples;
                    float freq = 90.0f + 60.0f * sinf(progress * 18.0f);
                    synth_phase1 += (freq * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    float noise = ((float)(rand() % 1000) / 500.0f) - 1.0f;
                    float env = expf(-progress * 4.0f);
                    sample_val = (sinf(synth_phase1) * 0.6f + noise * 0.4f) * env * 0.70f;
                    break;
                }

                case SynthType::REACTION_ZAP: {
                    // Енергетичний лазерний клік/спалах (2400 Гц -> 380 Гц)
                    float progress = (float)n / (float)synth_total_samples;
                    float freq = 2400.0f * expf(-progress * 6.0f) + 380.0f;
                    synth_phase1 += (freq * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    float env = expf(-progress * 5.0f);
                    sample_val = sinf(synth_phase1) * env * 0.85f;
                    break;
                }

                case SynthType::REACTION_CAT: {
                    // М'яукання котика (підйом 460 -> 780 Гц, потім плавний спад до 560 Гц)
                    float progress = (float)n / (float)synth_total_samples;
                    float freq = (progress < 0.5f) ? (460.0f + progress * 640.0f) : (780.0f - (progress - 0.5f) * 440.0f);
                    synth_phase1 += (freq * TWO_PI_F) / (float)SAMPLE_RATE;
                    synth_phase2 += (freq * 2.0f * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    if (synth_phase2 > TWO_PI_F) synth_phase2 -= TWO_PI_F;
                    float env = (progress < 0.1f) ? (progress / 0.1f) : expf(-(progress - 0.1f) * 2.8f);
                    sample_val = (sinf(synth_phase1) * 0.7f + sinf(synth_phase2) * 0.3f) * env;
                    break;
                }

                case SynthType::REACTION_UFO: {
                    // Космічне вібрато (540 Гц модульоване 11 Гц тремоло)
                    float progress = (float)n / (float)synth_total_samples;
                    float lfo = sinf(t * 11.0f * TWO_PI_F) * 65.0f;
                    synth_phase1 += ((540.0f + lfo) * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    float env = (progress < 0.1f) ? (progress / 0.1f) : expf(-(progress - 0.1f) * 2.2f);
                    sample_val = sinf(synth_phase1) * env * 0.80f;
                    break;
                }

                case SynthType::REACTION_CROWN: {
                    // Королівський фанфарний арпеджіо C5 (523Hz) -> E5 (659Hz) -> G5 (784Hz)
                    float progress = (float)n / (float)synth_total_samples;
                    float freq = (progress < 0.33f) ? 523.25f : ((progress < 0.66f) ? 659.25f : 783.99f);
                    float sub_p = fmodf(progress * 3.0f, 1.0f);
                    synth_phase1 += (freq * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;
                    float env = (sub_p < 0.08f) ? (sub_p / 0.08f) : expf(-(sub_p - 0.08f) * 3.5f);
                    sample_val = sinf(synth_phase1) * env * 0.85f;
                    break;
                }

                default:
                    break;
            }

            int16_t s16 = (int16_t)constrain((int)(sample_val * vol_gain * 26000.0f), -32767, 32767);
            pcm_buf[i * 2]     = s16; // Лівий канал
            pcm_buf[i * 2 + 1] = s16; // Правий канал
        }

        size_t bytes_written = 0;
        i2s_write(I2S_NUM_0, pcm_buf, samples_to_gen * sizeof(int16_t) * 2, &bytes_written, 0);
        synth_sample_idx += samples_to_gen;
        if (synth_sample_idx >= synth_total_samples) {
            cur_synth = SynthType::NONE;
            i2s_zero_dma_buffer(I2S_NUM_0);
            i2s_stop(I2S_NUM_0);
        }
    }

public:
    AudioService(AppState& st) : state(st) {}

    const char* getName() const override { return "AudioI2S"; }

    void init() override {
        audio.setPinout(Config::PIN_I2S_BCLK, Config::PIN_I2S_LRC, Config::PIN_I2S_DOUT);
        applied_volume = state.audio_volume;
        applied_muted  = state.audio_muted;
        audio.setVolume(applied_muted ? 0 : applied_volume);
        i2s_zero_dma_buffer(I2S_NUM_0);
        i2s_stop(I2S_NUM_0);
    }

    bool isPlaying() {
        return audio.isRunning() || (cur_synth != SynthType::NONE);
    }

    void playTouchClick() {
        startSynth(SynthType::TOUCH_CLICK);
    }

    void playBootChord() {
        startSynth(SynthType::BOOT_CHORD);
    }

    void playNotificationBell() {
        startSynth(SynthType::NOTIFY_BELL);
    }

    void playReactionSynth(uint8_t reaction_id) {
        switch (reaction_id) {
            case NOTIFY_FIRE:      startSynth(SynthType::REACTION_FIRE); break;
            case NOTIFY_LIGHTNING: startSynth(SynthType::REACTION_ZAP);  break;
            case NOTIFY_CAT:       startSynth(SynthType::REACTION_CAT);  break;
            case NOTIFY_UFO:       startSynth(SynthType::REACTION_UFO);  break;
            case NOTIFY_CROWN:     startSynth(SynthType::REACTION_CROWN);break;
            default:               startSynth(SynthType::NOTIFY_BELL);   break;
        }
    }

    void update() override {
        if (state.ota_in_progress) {
            if (audio.isRunning()) audio.stopSong();
            cur_synth = SynthType::NONE;
            state.audio_playing = false;
            return;
        }

        // Обробка зміни гучності та режиму MUTE
        bool mute_state = state.audio_muted || (state.audio_volume == 0);
        if (applied_muted != mute_state || applied_volume != state.audio_volume) {
            applied_muted  = mute_state;
            applied_volume = constrain(state.audio_volume, (uint8_t)0, (uint8_t)21);
            audio.setVolume(applied_muted ? 0 : applied_volume);
            if (applied_muted && audio.isRunning()) {
                audio.stopSong();
            }
        }

        // Обробка тригерів із AppState
        if (state.trig_touch_click) {
            state.trig_touch_click = false;
            playTouchClick();
        }
        if (state.trig_boot_chord) {
            state.trig_boot_chord = false;
            playBootChord();
        }
        if (state.trig_notify_bell) {
            state.trig_notify_bell = false;
            playNotificationBell();
        }
        if (state.trig_reaction_synth > 0) {
            uint8_t r = state.trig_reaction_synth;
            state.trig_reaction_synth = 0;
            playReactionSynth(r);
        }

        char url_buf[160];
        if (state.popPendingAudioUrl(url_buf, sizeof(url_buf))) {
            if (state.wifi_connected && !applied_muted) {
                WiFi.setSleep(false);
                state.registerNetActivity();
                cur_synth = SynthType::NONE;
                audio.stopSong();
                audio.connecttohost(url_buf);
            }
        }

        bool running = audio.isRunning();
        if (running) {
            state.audio_playing = true;
            state.registerNetActivity();
            audio.loop();
        } else {
            tickProceduralSynth();
            state.audio_playing = (cur_synth != SynthType::NONE);
        }
    }
};
