//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_NOISE_CH_H
#define OHBOI_NOISE_CH_H


#include <cstdint>

class noise_ch {
public:
    noise_ch();

    [[nodiscard]] uint8_t read(uint8_t reg) const;
    void write(uint8_t reg, uint8_t val);
    void step();
    [[nodiscard]] uint8_t get_output() const;
    void update_length();
    void update_envelope();
    [[nodiscard]] bool is_running() const;
private:
    int length_timer;
    void trigger();

    union {
        struct {
            uint8_t sound_length_data : 6;
            uint8_t unused : 2;
        };
        uint8_t val;
    } nr41;

    union {
        struct {
            uint8_t env_sweep : 3;
            uint8_t env_dir : 1;
            uint8_t env_init_vol : 4;
        };
        uint8_t val;
    } nr42;

    union {
        struct {
            uint8_t div_ratio : 3;
            uint8_t cnt_step : 1;
            uint8_t clock_freq : 4;
        };
        uint8_t val;
    } nr43;

    union {
        struct {
            uint8_t space1 : 3;
            uint8_t space2 : 3;
            uint8_t count_cons_sel : 1;
            uint8_t init : 1;
        };
        uint8_t val;
    } nr44;

    unsigned int output_vol;
    int freq;
    bool length_enable;
    uint8_t vol;
    uint8_t length_counter;
    int envelope_period;
    bool envelope_running;
    bool dac_enable;
    bool enable;
    unsigned short lfsr;
};


#endif //OHBOI_NOISE_CH_H
