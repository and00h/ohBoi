//
// Created by antonio on 30/07/20.
//

#include <Core/Audio/audio_ch_2.h>

static const uint8_t duty_table[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0}
};

audio_ch_2::audio_ch_2() {
    output_vol = 0;
    sequence_oointer = 0;
    nr21.wave_duty = 0;
    freq = 0;
    freq_load = 0;
    length_enable = false;
    vol = 0;
    length_counter = 0;
    envelope_period = 0;
    envelope_running = false;
    dac_enable = false;
    enable = false;

    nr21.val = 0;
    nr22.val = 0;
    nr23 = 0;
    nr24.val = 0;

}

uint8_t audio_ch_2::read(uint8_t reg) const {
    reg &= 0xF;
    reg %= 0x5;
    switch (reg) {
        case 1:
            return nr21.val;
        case 2:
            return nr22.val;
        case 3:
            return nr23;
        case 4:
            return nr24.val;
        default:
            return 0;
    }
}

void audio_ch_2::write(uint8_t reg, uint8_t val) {
    reg &= 0xF;
    reg %= 0x5;

    switch (reg) {
        case 0x1:
            nr21.val = val;
            length_counter = 64 - nr21.sound_length_data;
            break;
        case 0x2:
            dac_enable = (val & 0xF8) != 0;
            nr22.val = val;
            envelope_period = nr22.env_sweep;
            vol = nr22.env_init_vol;
            break;
        case 0x3:
            nr23 = val;
            freq_load = (freq_load & 0x700) | val;
            break;
        case 0x4:
            nr24.val = val;
            freq_load = (freq_load & 0xFF) | (nr24.freq_hi << 8);
            if ( nr24.count_cons_sel ) {
                if ( length_counter == 0 )
                    length_counter = 64;
                enable = length_counter > 0;
            }
            if ( nr24.init )
                trigger();
            break;
        default:
            break;
    }
}

void audio_ch_2::step() {
    if ( --freq <= 0 ) {
        freq = (2048 - freq_load) << 2;
        sequence_oointer = (sequence_oointer + 1) & 0x7;
    }

    output_vol = enable && dac_enable ? vol : 0;
    if ( not duty_table[nr21.wave_duty][sequence_oointer] )
        output_vol = 0;
}

uint8_t audio_ch_2::get_output() const {
    return output_vol;
}

void audio_ch_2::update_length() {
    if ( length_counter > 0 && nr24.count_cons_sel ) {
        --length_counter;
        if ( length_counter == 0 ) {
            enable = false;
        }
    }
}

void audio_ch_2::update_envelope() {
    if ( --envelope_period <= 0 ) {
        envelope_period = nr22.env_sweep;
        if ( envelope_period == 0 )
            envelope_period = 8;
        if ( envelope_running && nr22.env_sweep > 0 ) {
            if ( nr22.env_dir && vol < 15 )
                vol++;
            else if ( not nr22.env_dir && vol > 0 )
                vol--;
        }
        if ( vol == 0 || vol == 15 )
            envelope_running = false;
    }
}

bool audio_ch_2::is_running() const {
    return enable && dac_enable;
}

void audio_ch_2::trigger() {
    enable = true;
    if ( length_counter == 0 )
        length_counter = 64;
    freq = (2048 - freq) << 2;
    envelope_running = true;
    envelope_period = nr22.env_sweep;
    vol = nr22.env_init_vol;
}
