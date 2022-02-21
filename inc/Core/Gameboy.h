//
// Created by antonio on 01/08/20.
//

#ifndef OHBOI_GAMEBOY_H
#define OHBOI_GAMEBOY_H

#include <filesystem>
#include <string>
#include <memory>

#include "Core/cpu/Cpu.h"
#include "Core/Audio/apu.h"
#include "Core/Graphics/Ppu.h"
#include "Core/Joypad.h"
#include "Core/Memory/Memory.h"

namespace gb {
    class Gameboy {
    public:
        explicit Gameboy(std::filesystem::path &rom_path);
        ~Gameboy() { std::cout << "Gameboy destroyed" << std::endl; }
        void disable_bg() const { gpu_->toggle_bg(); }
        void disable_sprites() const { gpu_->toggle_sprites(); }
        void disable_window() const { gpu_->toggle_window(); }

        [[nodiscard]] unsigned int get_cpu_cycles() const { return cpu_->get_cycles(); }
        [[nodiscard]] Uint32* get_screen() const { return gpu_->get_screen(); }

        [[nodiscard]] bool is_paused() const { return paused_; }
        void pause() { paused_ = true; }

        void press_key(Joypad::key_e k) const { joypad_->press(k); }
        void release_key(Joypad::key_e k) const { joypad_->release(k); }
        void set_key(Joypad::key_e k, Joypad::key_state state) { joypad_->set_key_state(k, state); }
        void reset_cpu_cycle_counter() const { cpu_->reset_cycle_counter(); }
        void resume() { paused_ = false; }
        void toggle_pause() { paused_ = not paused_; }
        void set_speed(unsigned int multiplier) { speed_multiplier_ = multiplier; }
        void step();

        void toggle_ch1() { apu_.toggle_ch1(); }
        void toggle_ch2() { apu_.toggle_ch2(); }
        void toggle_noise() { apu_.toggle_noise(); }
        void toggle_wave() { apu_.toggle_wave(); }

        [[nodiscard]] bool new_audio_available() { return apu_.new_audio_available(); }
        void set_audio_reproduced() { apu_.set_reproduced(); }
        [[nodiscard]] const apu::audio_output& get_audio_output() { return apu_.get_audio_output(); }

    private:
        friend class cpu::Cpu;
        friend class memory::Memory;
        friend class graphics::Ppu;

        /* Need to make these unique_ptr because they share a cpu::Interrupts object that I can only create in the
         * constructor's body because it's useless to make it a class member, so I can't initialize them in the constructor
         * initializer list.
         *
         * To do that I would have to provide a default constructor for each of them just to initialize an empty object
         * and substitute it in the constructor with another object created with the proper constructor, but that is
         * fucking stupid.
         *
         * Doing this would mean providing a way to legally construct for example a CPU without interrupts, access
         * to memory and input that, besides making no sense logically, would happily try to use a bunch of nullptrs. */
        std::unique_ptr<cpu::Cpu> cpu_;
        std::unique_ptr<graphics::Ppu> gpu_;
        std::unique_ptr<memory::Memory> mmu_;

        std::shared_ptr<Joypad> joypad_;

        apu apu_;

        bool is_cgb_;
        bool paused_;
        unsigned int speed_multiplier_;

        void clock(unsigned int cycles);
    };
}

#endif //OHBOI_GAMEBOY_H
