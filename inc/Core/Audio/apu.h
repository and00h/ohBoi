//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_APU_H
#define OHBOI_APU_H


#include <bitset>

#include <cstdint>
#include <memory>
#include "audio_ch_1.h"
#include "audio_ch_2.h"
#include "noise_ch.h"
#include "wave_ch.h"

class apu {
public:
    struct audio_output {
        int ch1_output;
        int ch2_output;
        int wave_output;
        int noise_output;

        int ch1_output_right;
        int ch2_output_right;
        int wave_output_right;
        int noise_output_right;

        int left_volume;
        int right_volume;
    };

    apu();
    ~apu() = default;

    void send(uint16_t addr, uint8_t val);
    [[nodiscard]] uint8_t read(uint16_t addr) const;
    void step(int cycles);

    void toggle_ch1();
    void toggle_ch2();
    void toggle_wave();
    void toggle_noise();

    [[nodiscard]] const audio_output& get_audio_output() const;

    [[nodiscard]] bool new_audio_available() const;
    void set_reproduced();
private:
    bool new_audio;

    union {
        struct {
            uint8_t right_volume : 3;
            uint8_t vin_right_enable : 1;
            uint8_t left_volume : 3;
            uint8_t vin_left_enable : 1;
        };
        uint8_t val;
    } vin_control;

    union {
        struct {
            uint8_t channel_1_right : 1;
            uint8_t channel_2_right : 1;
            uint8_t channel_3_right : 1;
            uint8_t channel_4_right : 1;
            uint8_t channel_1_left : 1;
            uint8_t channel_2_left : 1;
            uint8_t channel_3_left : 1;
            uint8_t channel_4_left : 1;
        };
        uint8_t val;
    } output_select;

    union {
        struct {
            uint8_t channel_1_enable : 1;
            uint8_t channel_2_enable : 1;
            uint8_t channel_3_enable : 1;
            uint8_t channel_4_enable : 1;
            uint8_t unused : 3;
            uint8_t sound_enable : 1;
        };
        uint8_t val;
    } sound_control;

    int frame_sequence_counter;
    int downsample_count;
    uint8_t frame_sequencer;

    apu::audio_output mAudioOutput;

    audio_ch_1 ch1;
    audio_ch_2 ch2;
    wave_ch wave;
    noise_ch noise;

    bool ch1_enabled;
    bool ch2_enabled;
    bool wave_enabled;
    bool noise_enabled;

    void reset();
};


#endif //OHBOI_APU_H
