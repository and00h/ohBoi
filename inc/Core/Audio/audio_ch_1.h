//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_AUDIO_CH_1_H
#define OHBOI_AUDIO_CH_1_H


#include <bitset>
#include <cstdint>

class audio_ch_1 {
public:
    audio_ch_1();

    [[nodiscard]] uint8_t read(uint8_t reg) const;
    void write(uint8_t reg, uint8_t val);

    [[nodiscard]] uint8_t get_output() const;

    bool is_running() const;
    void step();
    void update_envelope();
    void update_length();
    void update_sweep();
private:
    void trigger();
    unsigned short sweep_calc();

    unsigned int output_vol;
    unsigned int duty_pointer;
    unsigned int duty;
    int frequency;
    unsigned short frequency_load;
    bool length_enable;
    uint8_t volume;
    uint8_t length_counter;
    int envelope_period;
    bool is_envelope_running;
    bool dac_enable;
    bool enable;
    int sweep_period;
    bool sweep_enable;
    int frequency_shadow;

    int length_timer;

    union {
        struct {
            uint8_t sweep_shift : 3;
            uint8_t increasing : 1;
            uint8_t sweep_time : 3;
            uint8_t unused : 1;
        };
        uint8_t val;
    } nr10;

    union {
        struct {
            uint8_t sound_length_data : 6;
            uint8_t wave_duty : 2;
        };
        uint8_t val;
    } nr11;

    union {
        struct {
            uint8_t env_sweep : 3;
            uint8_t env_dir : 1;
            uint8_t env_init_vol : 4;
        };
        uint8_t val;
    } nr12;

    uint8_t nr13;

    union {
        struct {
            uint8_t freq_hi : 3;
            uint8_t unused : 3;
            uint8_t count_cons_sel : 1;
            uint8_t init : 1;
        };
        uint8_t val;
    } nr14;
};


#endif //OHBOI_AUDIO_CH_1_H
