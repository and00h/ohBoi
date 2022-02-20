//
// Created by antonio on 30/07/20.
//

#include <Core/Audio/noise_ch.h>

static const int divisors[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

noise_ch::noise_ch() {
    output_vol = 0;
    freq = 0;
    length_enable = false;
    vol = 0;
    length_counter = 0;
    envelope_period = 0;
    envelope_running = 0;
    dac_enable = false;
    enable = false;
    lfsr = 0;
}

uint8_t noise_ch::read(uint8_t reg) const {
    switch (reg) {
        case 0x1F:
            return 0;
        case 0x20:
            return nr41.sound_length_data;
        case 0x21:
            return nr42.val;
        case 0x22:
            return nr43.val;
        case 0x23:
            return nr44.val & 0xC0;
        default:
            return 0;
    }
}

void noise_ch::write(uint8_t reg, uint8_t val) {
    switch (reg) {
        case 0x1F:
            break;
        case 0x20:
            nr41.val = val & 0x3F;
            length_counter = 64 - nr41.sound_length_data;
            break;
        case 0x21:
            dac_enable = (val & 0xF8) != 0;
            nr42.val = val;
            break;
        case 0x22:
            nr43.val = val;
            break;
        case 0x23:
            nr44.val = val & 0xC0;
            if ( nr44.count_cons_sel ) {
                if ( length_counter == 0 )
                    length_counter = 64;
                enable = true;
            }
            if ( nr44.init )
                trigger();
            break;
        default:
            break;
    }
}

void noise_ch::step() {
    if ( --freq <= 0 ) {
        freq = divisors[nr43.div_ratio] << nr43.clock_freq;
        uint8_t res = (lfsr & 1) ^ ((lfsr >> 1) & 1);
        lfsr >>= 1;
        lfsr |= res << 14;
        if ( nr43.cnt_step ) {
            lfsr &= ~0x40;
            lfsr |= res << 6;
        }
        output_vol = (enable && dac_enable && (lfsr & 1) == 0) ? vol : 0;
    }
}

uint8_t noise_ch::get_output() const {
    return output_vol;
}

void noise_ch::update_length() {
    if ( length_counter > 0 && nr44.count_cons_sel ) {
        length_counter--;
        if ( length_counter == 0 ) {
            enable = false;
        }
    }
}

void noise_ch::update_envelope() {
    if (--envelope_period <= 0 ) {
        envelope_period = nr42.env_sweep;
        if (envelope_period == 0 )
            envelope_period = 8;
        if ( envelope_running && nr42.env_sweep > 0 ) {
            if ( nr42.env_dir && vol < 15 )
                vol++;
            else if ( !nr42.env_dir && vol > 0 )
                vol--;
        }
        if ( vol == 0 || vol == 15 )
            envelope_running = false;
    }
}

bool noise_ch::is_running() const {
    return enable && dac_enable;
}

void noise_ch::trigger() {
    enable = true;
    if ( length_counter == 0 )
        length_counter = 64;
    freq = divisors[nr43.div_ratio] << nr43.clock_freq;
    envelope_period = nr42.env_sweep;
    envelope_running = true;
    vol = nr42.env_init_vol;
    lfsr = 0x7FFF;
}
