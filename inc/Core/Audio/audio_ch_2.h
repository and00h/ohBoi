//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_AUDIO_CH_2_H
#define OHBOI_AUDIO_CH_2_H


#include <cstdint>

class audio_ch_2 {
public:
    audio_ch_2();

    [[nodiscard]] uint8_t read(uint8_t reg) const;
    void write(uint8_t reg, uint8_t val);
    void step();
    [[nodiscard]] uint8_t get_output() const;
    [[nodiscard]] bool is_running() const;
    void update_length();
    void update_envelope();
    void update_sweep();
private:
    void trigger();
    unsigned short sweep_calc();

    unsigned int output_vol;
    unsigned int sequence_oointer;
    unsigned int duty;
    int freq;
    unsigned short freq_load;
    bool length_enable;
    uint8_t vol;
    uint8_t length_counter;
    int envelope_period;
    bool envelope_running;
    bool dac_enable;
    bool enable;

    int length_timer;
    union {
        struct {
            uint8_t sound_length_data : 6;
            uint8_t wave_duty : 2;
        };
        uint8_t val;
    } nr21;

    union {
        struct {
            uint8_t env_sweep : 3;
            uint8_t env_dir : 1;
            uint8_t env_init_vol : 4;
        };
        uint8_t val;
    } nr22;

    uint8_t nr23;

    union {
        struct {
            uint8_t freq_hi : 3;
            uint8_t unused : 3;
            uint8_t count_cons_sel : 1;
            uint8_t init : 1;
        };
        uint8_t val;
    } nr24;
};


#endif //OHBOI_AUDIO_CH_2_H
