//
// Created by antonio on 30/07/20.
//

#include <Core/Audio/audio_ch_1.h>

static const uint8_t duty_table[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0}
};

audio_ch_1::audio_ch_1() {
    output_vol = 0;
    duty_pointer = 0;
    duty = 0;
    frequency = 0;
    frequency_load = 0;
    length_enable = false;
    volume = 0;
    length_counter = 0;
    envelope_period = 0;
    is_envelope_running = false;
    dac_enable = false;
    enable = false;
    sweep_period = 0;
    sweep_enable = false;
    frequency_shadow = 0;
    nr10.val = 0;
    nr11.val = 0;
    nr12.val = 0;
    nr13 = 0;
    nr14.val = 0;

}

uint8_t audio_ch_1::read(uint8_t reg) const {
    reg &= 0xF;
    reg %= 0x5;
    switch (reg) {
        case 0:
            return nr10.val & 0x7F;
        case 1:
            return nr11.val;
        case 2:
            return nr12.val;
        case 3:
            return nr13;
        case 4:
            return nr14.val & 0xC7;
        default:
            return 0;
    }
}

void audio_ch_1::write(uint8_t reg, uint8_t val) {
    reg &= 0xF;
    reg %= 0x5;

    switch (reg) {
        case 0:
            nr10.val = val & 0x7F;
            break;
        case 1:
            nr11.val = val;
            length_counter = 64 - nr11.sound_length_data;
            break;
        case 2:
            dac_enable = (val & 0xF8) != 0;
            nr12.val = val;
            envelope_period = nr12.env_sweep;
            volume = nr12.env_init_vol;
            is_envelope_running = volume > 0 && volume < 15;
            break;
        case 3:
            nr13 = val;
            frequency_load = (frequency_load & 0x700) | val;
            break;
        case 4:
            nr14.val = val;
            frequency_load = (frequency_load & 0xFF) | (nr14.freq_hi << 8);
            if ( nr14.count_cons_sel ) {
                if ( length_counter == 0 )
                    length_counter = 64;
                enable = length_counter > 0;
            }
            if ( nr14.init )
                trigger();
            break;
        default:
            break;
    }
}

void audio_ch_1::step() {
    if ( --frequency <= 0 ) {
        frequency = (2048 - frequency_load ) << 2;
        duty_pointer = (duty_pointer + 1) & 0x7;
    }
    output_vol = enable && dac_enable ? volume : 0;
    if ( not duty_table[nr11.wave_duty][duty_pointer] )
        output_vol = 0;
}

uint8_t audio_ch_1::get_output() const {
    return output_vol;
}

void audio_ch_1::update_length() {
    if ( length_counter > 0 && nr14.count_cons_sel ) {
        length_counter--;
        if ( length_counter == 0 ) {
            enable = false;
        }
    }
}

void audio_ch_1::update_envelope() {
    if ( --envelope_period <= 0 ) {
        envelope_period = nr12.env_sweep;
        if ( envelope_period == 0 )
            envelope_period = 8;
        if ( is_envelope_running && nr12.env_sweep > 0 ) {
            if ( nr12.env_dir && volume < 15 )
                volume++;
            else if ( not nr12.env_dir && volume > 0 )
                volume--;
        }
        if ( volume == 0 || volume == 15 )
            is_envelope_running = false;
    }
}

void audio_ch_1::update_sweep() {
    if ( --sweep_period <= 0 ) {
        sweep_period = nr10.sweep_time;
        if ( sweep_period == 0 )
            sweep_period = 8;
        if ( sweep_enable && nr10.sweep_time > 0 ) {
            unsigned short newFreq = sweep_calc();
            if ( newFreq <= 2047 && nr10.sweep_shift > 0 ) {
                frequency_shadow = newFreq;
                frequency_load = newFreq;
                sweep_calc();
            }
            sweep_calc();
        }
    }
}

bool audio_ch_1::is_running() const {
    return enable && dac_enable;
}

unsigned short audio_ch_1::sweep_calc() {
    unsigned short new_freq = frequency_shadow >> nr10.sweep_shift;
    new_freq = frequency_shadow + (nr10.increasing ? -new_freq : new_freq);
    if ( new_freq > 2047 )
        enable = false;

    return new_freq;
}

void audio_ch_1::trigger() {
    enable = true;
    if ( length_counter == 0 )
        length_counter = 64;

    frequency = ( 2048 - frequency ) * 4;
    is_envelope_running = true;
    envelope_period = nr12.env_sweep;
    volume = nr12.env_init_vol;
    frequency_shadow = frequency_load;
    sweep_period = nr10.sweep_time;
    if ( sweep_period == 0 )
        sweep_period = 8;
    sweep_enable = sweep_period > 0 || nr10.sweep_shift > 0;
    if ( nr10.sweep_shift > 0 )
        sweep_calc();
}
