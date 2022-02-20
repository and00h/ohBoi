//
// Created by antonio on 30/07/20.
//

#include <Core/Audio/wave_ch.h>

wave_ch::wave_ch() {
    std::fill(wave_pattern.begin(), wave_pattern.end(), 0);

    output_vol = 0;
    freq = 0;
    freq_load = 0;
    position_counter = 0;
    length_counter = 0;
    enable = false;

    m_length_timer = 16384;
}

uint8_t wave_ch::read(uint8_t reg) const {
    if ( reg >= 0x1A && reg <= 0x1E ) {
        switch (reg & 0xF) {
            case 0xA:
                return nr30.enable << 7;
            case 0xB:
                return nr31;
            case 0xC:
                return nr32.output_level << 5;
            case 0xD:
                return nr33;
            case 0xE:
                return nr34.val;
            default:
                return 0xFF;
        }
    } else if ( reg >= 0x30 && reg <= 0x3F )
        return wave_pattern[reg & 0xF];
    return 0xFF;
}

void wave_ch::write(uint8_t reg, uint8_t val) {
    if ( reg >= 0x1A && reg <= 0x1E ) {
        switch (reg & 0xF) {
            case 0xA:
                nr30.val = val & 0x80;
                break;
            case 0xB:
                nr31 = val;
                length_counter = 256 - nr31;
                break;
            case 0xC:
                nr32.val = val & 0x60;
                break;
            case 0xD:
                nr33 = val;
                freq_load = (freq_load & 0x700) | val;
                break;
            case 0xE:
                nr34.val = val & 0xC7;
                freq_load = (freq_load & 0xFF) | (nr34.freq_hi << 8);
                if ( nr34.count_cons_sel ) {
                    if ( length_counter == 0 )
                        length_counter = 64;
                    enable = length_counter > 0;
                }
                if ( nr34.init )
                    trigger();
                break;
            default:
                break;
        }
    } else if ( reg >= 0x30 && reg <= 0x3F )
        wave_pattern[reg & 0xF] = val;
}

void wave_ch::clear_wave_pattern() {
    std::fill(wave_pattern.begin(), wave_pattern.end(), 0);
}

void wave_ch::step() {
    if ( --freq <= 0 ) {
        freq = (2048 - freq_load) << 1;
        position_counter = (position_counter + 1) & 0x1F;
        if ( enable && nr30.enable ) {
            uint8_t output = wave_pattern[position_counter >> 1];
            if ( (position_counter & 1) == 0 )
                output >>= 4;
            output &= 0xF;
            if ( nr32.output_level > 0 )
                output >>= nr32.output_level - 1;
            else
                output = 0;
            output_vol = output;
        } else
            output_vol = 0;
    }
}

uint8_t wave_ch::get_output() const {
    return output_vol;
}

void wave_ch::update_length() {
    if (length_counter > 0 && nr34.count_cons_sel ) {
        length_counter--;
        if (length_counter == 0 ) {
            enable = false;
        }
    }
}

bool wave_ch::is_running() const {
    return enable && nr30.enable;
}

void wave_ch::trigger() {
    enable = true;
    if (length_counter == 0 )
        length_counter = 256;
    freq = (2048 - freq_load) << 1;
    position_counter = 0;
}
