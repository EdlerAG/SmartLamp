#pragma once
#include "core/IService.h"
#include "Config.h"
#include "core/AppState.h"
#include <WiFi.h>
#include <Audio.h>
#include <driver/i2s.h>
#include <math.h>
#include "ClickAudioSamples.h"

class AudioService : public IService {
private:
    AppState& state;
    Audio     audio;
    uint8_t   applied_volume = 0;
    bool      applied_muted  = false;
    bool      was_running    = false;

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
    static constexpr size_t CHUNK_SAMPLES = 512; // 128 стерео-семплів (512 байт) на тик

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
        cur_synth = type;
        synth_sample_idx = 0;
        synth_phase1 = 0.0f;
        synth_phase2 = 0.0f;
        synth_phase3 = 0.0f;

        switch (type) {
            case SynthType::TOUCH_CLICK:
                synth_total_samples = (SAMPLE_RATE * 32) / 1000;    // 32 мс (~1411 семплів)
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
        if (synth_total_samples > 0) {
            synth_total_samples = (synth_total_samples + 511) & ~((size_t)511);
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

        // Масштабування гучності: перцептивне для тактильних кліків та квадратичне для іншого аудіо
        float norm_vol = (float)constrain((int)state.audio_volume, 0, 21) / 21.0f;
        float vol_gain = (cur_synth == SynthType::TOUCH_CLICK)
            ? (0.55f + 0.45f * norm_vol) // виразний тактильний клік навіть на невисокій гучності
            : (norm_vol * norm_vol);
        constexpr float TWO_PI_F = 6.283185307f;

        for (size_t i = 0; i < samples_to_gen; i++) {
            uint32_t n = synth_sample_idx + i;
            float t = (float)n / (float)SAMPLE_RATE;
            float sample_val = 0.0f;

            switch (cur_synth) {
                case SynthType::TOUCH_CLICK: {
                    // Преміальний тактильний механічний клац (32 мс, чіткий клацаючий імпульс)
                    float progress = (float)n / (float)synth_total_samples;
                    // Частотний перепад тіла кліка: від 1600 Гц швидко до 440 Гц
                    float freq = 440.0f + 1160.0f * expf(-progress * 12.0f);
                    synth_phase1 += (freq * TWO_PI_F) / (float)SAMPLE_RATE;
                    if (synth_phase1 > TWO_PI_F) synth_phase1 -= TWO_PI_F;

                    // Атака (0.6 мс для уникнення DC-кліпінгу) та плавний експоненційний спад
                    float attack = (progress < 0.02f) ? (progress / 0.02f) : 1.0f;
                    float decay  = expf(-progress * 7.0f);
                    float env    = attack * decay;

                    // Високочастотний металевий транзієнт (2750 Гц) у перші 4 мс для відчуття тактильного клацання
                    float snap = 0.0f;
                    if (progress < 0.16f) {
                        synth_phase2 += (2750.0f * TWO_PI_F) / (float)SAMPLE_RATE;
                        if (synth_phase2 > TWO_PI_F) synth_phase2 -= TWO_PI_F;
                        snap = sinf(synth_phase2) * expf(-progress * 26.0f) * 0.40f;
                    }

                    sample_val = (sinf(synth_phase1) * 0.80f + snap) * env;
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
        i2s_write(I2S_NUM_0, pcm_buf, samples_to_gen * sizeof(int16_t) * 2, &bytes_written, portMAX_DELAY);
        synth_sample_idx += samples_to_gen;
        if (synth_sample_idx >= synth_total_samples) {
            cur_synth = SynthType::NONE;
            click_finish_time = millis() + 48;
            dma_needs_resync = true;
        }
    }

    SemaphoreHandle_t i2s_mutex = nullptr;
    volatile bool     dma_needs_resync = false;
    volatile uint32_t last_resync_ms   = 0;

    void resyncDmaQueueLocked() {
        i2s_zero_dma_buffer(I2S_NUM_0);
        i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
        last_resync_ms = millis();
        dma_needs_resync = false;
    }

    static inline int16_t softLimitSample(int32_t x) {
        if (x > 28000) {
            x = 28000 + ((x - 28000) >> 2);
            if (x > 32760) x = 32760;
        } else if (x < -28000) {
            x = -28000 - ((-28000 - x) >> 2);
            if (x < -32760) x = -32760;
        }
        return (int16_t)x;
    }

public:
    AudioService(AppState& st) : state(st) {}

    const char* getName() const override { return "AudioI2S"; }

    void init() override {
        if (!i2s_mutex) {
            i2s_mutex = xSemaphoreCreateMutex();
        }
        audio.setPinout(Config::PIN_I2S_BCLK, Config::PIN_I2S_LRC, Config::PIN_I2S_DOUT);
        applied_volume = state.audio_volume;
        applied_muted  = state.audio_muted;
        audio.setVolume(applied_muted ? 0 : applied_volume);
        if (i2s_mutex) xSemaphoreTake(i2s_mutex, portMAX_DELAY);
        resyncDmaQueueLocked();
        if (i2s_mutex) xSemaphoreGive(i2s_mutex);
    }

    volatile uint32_t click_finish_time = 0;

    bool isPlaying() {
        return audio.isRunning() || (cur_synth != SynthType::NONE) || (millis() < click_finish_time) || dma_needs_resync;
    }

    void playClickSample(uint8_t profile = 0, uint8_t variation = 1) {
        if (!state.touch_beep_enabled || state.audio_volume == 0) {
            return;
        }
        if (audio.isRunning()) return;

        // Перцептивна крива гучності для MAX98357A (+9dB GAIN), індекс 0..21 (масштаб Q8: /256)
        static constexpr uint8_t CLICK_VOL_LUT[22] = {
            0,  18, 22, 26, 30, 34, 38, 43, 48, 53, 58,
            64, 70, 76, 82, 88, 94, 100, 106, 112, 118, 124
        };
        uint8_t vol_idx = constrain(state.audio_volume, (uint8_t)0, (uint8_t)21);
        uint32_t vol_mult = CLICK_VOL_LUT[vol_idx];

        const int16_t* sample_data = CLICK_SAMPLE_APPLE;
        size_t sample_len = CLICK_SAMPLE_APPLE_LEN;

        if (profile == 1) {
            sample_data = CLICK_SAMPLE_WOOD;
            sample_len = CLICK_SAMPLE_WOOD_LEN;
        } else if (profile == 2) {
            sample_data = CLICK_SAMPLE_POP;
            sample_len = CLICK_SAMPLE_POP_LEN;
        }

        if (i2s_mutex) xSemaphoreTake(i2s_mutex, portMAX_DELAY);

        // Stop any procedural synth cleanly
        cur_synth = SynthType::NONE;

        static int16_t s_click_dma_buf[2048 * 2];
        memset(s_click_dma_buf, 0, sizeof(s_click_dma_buf));

        constexpr size_t PRE_ROLL = 32; // 0.72 ms guard margin at start of first descriptor
        int32_t hpf_y = 0;
        int32_t hpf_prev_x = 0;

        if (variation == 3) {
            // Haptic double-tap for long press with 1st-order 168 Hz High-Pass filter & smooth tail fade
            size_t pulse1_len = min((size_t)420, sample_len);
            for (size_t i = 0; i < pulse1_len; i++) {
                int32_t raw = (int16_t)pgm_read_word(&sample_data[i]);
                hpf_y = ((hpf_y + raw - hpf_prev_x) * 250) >> 8;
                hpf_prev_x = raw;
                uint32_t env = (i + 64 > pulse1_len) ? ((pulse1_len - i) * 256 / 64) : 256;
                int32_t scaled = (((hpf_y * (int32_t)(vol_mult * 70 / 100)) >> 8) * (int32_t)env) >> 8;
                int16_t val = softLimitSample(scaled);
                s_click_dma_buf[(PRE_ROLL + i) * 2]     = val;
                s_click_dma_buf[(PRE_ROLL + i) * 2 + 1] = val;
            }

            hpf_y = 0;
            hpf_prev_x = 0;
            size_t pulse2_offset = PRE_ROLL + 1050;
            size_t pulse2_len = min((size_t)650, sample_len);
            for (size_t i = 0; i < pulse2_len && (pulse2_offset + i) < 2048; i++) {
                int32_t raw = (int16_t)pgm_read_word(&sample_data[i]);
                hpf_y = ((hpf_y + raw - hpf_prev_x) * 250) >> 8;
                hpf_prev_x = raw;
                uint32_t env = (i + 64 > pulse2_len) ? ((pulse2_len - i) * 256 / 64) : 256;
                int32_t scaled = (((hpf_y * (int32_t)(vol_mult * 95 / 100)) >> 8) * (int32_t)env) >> 8;
                int16_t val = softLimitSample(scaled);
                s_click_dma_buf[(pulse2_offset + i) * 2]     = val;
                s_click_dma_buf[(pulse2_offset + i) * 2 + 1] = val;
            }
        } else {
            // Pure studio 1:1 playback with 168 Hz sub-bass High-Pass filter & 32-sample end taper
            size_t count = min((size_t)(2048 - PRE_ROLL), sample_len);
            for (size_t i = 0; i < count; i++) {
                int32_t raw = (int16_t)pgm_read_word(&sample_data[i]);
                hpf_y = ((hpf_y + raw - hpf_prev_x) * 250) >> 8;
                hpf_prev_x = raw;
                uint32_t env = (i + 32 > count) ? ((count - i) * 256 / 32) : 256;
                int32_t scaled = (((hpf_y * (int32_t)vol_mult) >> 8) * (int32_t)env) >> 8;
                int16_t val = softLimitSample(scaled);
                s_click_dma_buf[(PRE_ROLL + i) * 2]     = val;
                s_click_dma_buf[(PRE_ROLL + i) * 2 + 1] = val;
            }
        }

        uint32_t now_ms = millis();
        state.diag_clk_gap_ms = (state.diag_clk_last_ms > 0) ? (now_ms - state.diag_clk_last_ms) : 0;
        state.diag_clk_last_ms = now_ms;
        state.diag_clk_cnt++;
        state.diag_clk_cpu = getCpuFrequencyMhz();
        state.diag_clk_core = (uint8_t)xPortGetCoreID();

        // Pre-armed Zero-Latency DMA Burst (~50 us execution, <1 ms acoustic latency):
        // When background resync has primed desc[0..3] at the head of tx->queue (>=65ms after resync),
        // we pause GDMA for 45 us, copy 8192 bytes into desc[0..3] non-blockingly, and restart GDMA at desc[0].
        size_t bytes_written = 0;
        esp_err_t res = ESP_FAIL;
        bool fast_ok = false;
        uint32_t t0 = micros();

        if (!dma_needs_resync && (now_ms - last_resync_ms >= 65)) {
            i2s_stop(I2S_NUM_0);
            res = i2s_write(I2S_NUM_0, s_click_dma_buf, sizeof(s_click_dma_buf), &bytes_written, 0);
            i2s_start(I2S_NUM_0);
            fast_ok = (res == ESP_OK && bytes_written == sizeof(s_click_dma_buf));
        }

        if (!fast_ok) {
            bytes_written = 0;
            i2s_zero_dma_buffer(I2S_NUM_0);
            i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
            res = i2s_write(I2S_NUM_0, s_click_dma_buf, sizeof(s_click_dma_buf), &bytes_written, pdMS_TO_TICKS(60));
            click_finish_time = millis() + 195;
        } else {
            click_finish_time = millis() + 48;
        }

        uint32_t dt_us = micros() - t0;
        dma_needs_resync = true;

        if (i2s_mutex) xSemaphoreGive(i2s_mutex);

        state.diag_clk_us = dt_us;
        if (dt_us > state.diag_clk_max_us) {
            state.diag_clk_max_us = dt_us;
        }
        state.diag_clk_bytes = bytes_written;
        state.diag_clk_res = (int32_t)res;
    }

    void playTouchClick(uint8_t variation = 1) {
        playClickSample(state.touch_click_profile, variation);
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
            was_running = true;
        } else {
            if (i2s_mutex) xSemaphoreTake(i2s_mutex, portMAX_DELAY);
            if (was_running) {
                was_running = false;
                // Після завершення стріму повертаємо 44100 Гц та синхронізуємо чергу DMA для UI-кліків
                resyncDmaQueueLocked();
            }
            if (cur_synth != SynthType::NONE) {
                tickProceduralSynth();
            } else if (dma_needs_resync && (int32_t)(millis() - click_finish_time) >= 0) {
                // Фонова ресинхронізація DMA після повного завершення кліка (гарантує desc[0..3] на вході tx->queue)
                resyncDmaQueueLocked();
            }
            state.audio_playing = (cur_synth != SynthType::NONE);
            if (i2s_mutex) xSemaphoreGive(i2s_mutex);
        }
    }
};
