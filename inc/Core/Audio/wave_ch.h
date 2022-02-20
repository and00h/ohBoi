//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_WAVE_CH_H
#define OHBOI_WAVE_CH_H


#include <cstdint>
#include <vector>
#include <array>

class wave_ch {
public:
    wave_ch();

    [[nodiscard]] uint8_t read(uint8_t reg) const;
    void write(uint8_t reg, uint8_t val);
    void clear_wave_pattern();
    void step();
    [[nodiscard]] uint8_t get_output() const;
    void update_length();
    [[nodiscard]] bool is_running() const;
private:
    int m_length_timer;
    void trigger();

    union {
        struct {
            uint8_t unused : 7;
            uint8_t enable : 1;
        };
        uint8_t val;
    } nr30;

    uint8_t nr31;

    union {
        struct {
            uint8_t unused : 5;
            uint8_t output_level : 2;
            uint8_t unused2 : 1;
        };
        uint8_t val;
    } nr32;

    uint8_t nr33;

    union {
        struct {
            uint8_t freq_hi : 3;
            uint8_t unused : 3;
            uint8_t count_cons_sel : 1;
            uint8_t init : 1;
        };

        uint8_t val;
    } nr34;

    unsigned int output_vol;
    int freq;
    int length_counter;
    unsigned short freq_load;
    uint8_t position_counter;
    bool enable;
    bool length_enable;

    std::array<uint8_t, 0xF> wave_pattern;
};


#endif //OHBOI_WAVE_CH_H
