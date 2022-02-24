//
// Created by antonio on 30/07/20.
//

#include <Core/Audio/apu.h>
#include <Core/Audio/audio_ch_1.h>
#include <Core/Audio/audio_ch_2.h>
#include <Core/Audio/noise_ch.h>
#include <Core/Audio/wave_ch.h>
#include <iostream>

const unsigned int SAMPLE_SIZE = 4096;

static uint8_t readOrValues[23] = {  0x80,0x3f,0x00,0xff,0xbf,
                                     0xff,0x3f,0x00,0xff,0xbf,
                                     0x7f,0xff,0x9f,0xff,0xbf,
                                     0xff,0xff,0x00,0x00,0xbf,
                                     0x00,0x00,0x70 };

void apu::reset() {
    frame_sequence_counter = 8192;
    downsample_count = 95;
    frame_sequencer = 0;

    ch1_enabled = true;
    ch2_enabled = true;
    wave_enabled = true;
    noise_enabled = true;

    mAudioOutput = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    new_audio = false;
    ch1.write(0x10u, 0x80u);
    ch1.write(0x11u, 0xBFu);
    ch1.write(0x12u, 0xF3u);
    ch1.write(0x13u, 0);
    ch1.write(0x14u, 0xBFu);
    ch2.write(0x16u, 0x3Fu);
    ch2.write(0x17u, 0x00u);
    ch2.write(0x19u, 0xBFu);
    wave.write(0x1Au, 0x7Fu);
    wave.write(0x1Bu, 0xFFu);
    wave.write(0x1Cu, 0x9Fu);
    wave.write(0x1Eu, 0xBFu);
    noise.write(0x20u, 0xFFu);
    noise.write(0x21u, 0x00u);
    noise.write(0x22u, 0x00u);
    noise.write(0x23u, 0xBFu);
    send(0x24, 0x77);
    send(0x25, 0xF3);
    send(0x26, 0x81);

    wave.clear_wave_pattern();
//    sound_control.values = 0x81;
}

apu::apu() : 
        ch1(audio_ch_1()),
        ch2(audio_ch_2()),
        wave(wave_ch()),
        noise(noise_ch()) {
    reset();
}

void apu::toggle_ch1() { 
    ch1_enabled = !ch1_enabled; 
}

void apu::toggle_ch2() { 
    ch2_enabled = !ch2_enabled; 
}

void apu::toggle_wave() { 
    wave_enabled = !wave_enabled; 
}

void apu::toggle_noise() { 
    noise_enabled = !noise_enabled; 
}

const apu::audio_output& apu::get_audio_output() const {
    return mAudioOutput;
}

bool apu::new_audio_available() const { 
    return new_audio; 
}

void apu::set_reproduced() { 
    new_audio = false; 
}

void apu::send(uint16_t addr, uint8_t val) {
    if ( not sound_control.sound_enable && addr != 0xFF26 ) {
        return;
    }
    uint8_t reg = addr & 0xFF;
    if ( reg >= 0x10 && reg <= 0x14 ) /*reg >= 0x10 && reg <= 0x14*/
        ch1.write(reg, val);
    else if ( reg >= 0x16 && reg <= 0x19 )
        ch2.write(reg, val);
    else if ( (reg >= 0x1A && reg <= 0x1E) || (reg >= 0x30 && reg <= 0x3F) )
        wave.write(reg, val);
    else if ( reg >= 0x1F && reg <= 0x23 )
        noise.write(reg, val);
    else if ( reg >= 0x24 && reg <= 0x26 ) {
        switch (reg) {
            case 0x24:
                vin_control.val = val;
                break;
            case 0x25:
                output_select.val = val;
                break;
            case 0x26:
                if ( sound_control.sound_enable && (val & 0x80) == 0 ) {
                    for ( unsigned int i = 0xFF10; i <= 0xFF25; i++ )
                        send(i, 0);
                }
                else if ( not sound_control.sound_enable && (val & 0x80) ){
                    frame_sequencer = 0;
                    wave.clear_wave_pattern();
                }
                sound_control.val = val & 0x80;
                break;
            default:
                break;
        }
    }
}

uint8_t apu::read(uint16_t addr) const {
    uint8_t reg = addr & 0xFF;
    if ( reg >= 0x10 && reg <= 0x14 )
        return ch1.read(reg) | readOrValues[reg - 0x10];
    else if ( reg >= 0x16 && reg <= 0x19 )
        return ch2.read(reg) | readOrValues[reg - 0x10];
    else if ( reg >= 0x1A && reg <= 0x1E )
        return wave.read(reg) | readOrValues[reg - 0x10];
    else if ( reg >= 0x1F && reg <= 0x23 )
        return noise.read(reg) | readOrValues[reg - 0x10];
    else if ( reg >= 0x24 && reg <= 0x26 ) {
        switch (reg) {
            case 0x24:
                return vin_control.val | readOrValues[reg - 0x10];
            case 0x25:
                return output_select.val | readOrValues[reg - 0x10];
            case 0x26:
                return (sound_control.sound_enable ? 0x80 : 0)
                    | readOrValues[reg - 0x10]
                    | (ch1.is_running() ? 1 : 0)
                    | ((ch2.is_running() ? 1 : 0) << 1)
                    | ((wave.is_running() ? 1 : 0) << 2)
                    | ((noise.is_running() ? 1 : 0) << 3)
;
            default:
                return 0xFF;
        }
    } else if ( reg >= 0x30 && reg <= 0x3F )
        return wave.read(reg);
    return 0xFF;
}

void apu::step(int cycles) {
    if ( not sound_control.sound_enable ) {
        return;
    }
    while ( cycles-- != 0 ) {
        if ( --frame_sequence_counter <= 0 ) {
            frame_sequence_counter = 8192;
            switch ( frame_sequencer ) {
                case 0:
                    ch1.update_length();
                    ch2.update_length();
                    wave.update_length();
                    noise.update_length();
                    break;
                case 2:
                    ch1.update_sweep();
                    ch1.update_length();
                    ch2.update_length();
                    wave.update_length();
                    noise.update_length();
                    break;
                case 4:
                    ch1.update_length();
                    ch2.update_length();
                    wave.update_length();
                    noise.update_length();
                    break;
                case 6:
                    ch1.update_sweep();
                    ch1.update_length();
                    ch2.update_length();
                    wave.update_length();
                    noise.update_length();
                    break;
                case 7:
                    ch1.update_envelope();
                    ch2.update_envelope();
                    noise.update_envelope();
                    break;
            }
            if ( ++frame_sequencer >= 8 )
                frame_sequencer = 0;
        }
        ch1.step();
        ch2.step();
        wave.step();
        noise.step();

        if ( --downsample_count <= 0 ) {
            new_audio = true;
            downsample_count = 95;

            uint8_t ch2out = ch2.get_output();

            mAudioOutput = {
                    output_select.channel_1_left && ch1_enabled ? ch1.get_output() : 0,
                    output_select.channel_2_left && ch2_enabled ? ch2out : 0,
                    output_select.channel_3_left && wave_enabled ? wave.get_output() : 0,
                    output_select.channel_4_left && noise_enabled ? noise.get_output() : 0,

                    output_select.channel_1_right && ch1_enabled ? ch1.get_output() : 0,
                    output_select.channel_2_right && ch2_enabled ? ch2out : 0,
                    output_select.channel_3_right && wave_enabled ? wave.get_output() : 0,
                    output_select.channel_4_right && noise_enabled ? noise.get_output() : 0,

                    vin_control.left_volume,
                    vin_control.right_volume
            };
        }
    }
}
