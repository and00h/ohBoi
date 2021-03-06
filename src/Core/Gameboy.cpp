//
// Created by antonio on 01/08/20.
//

#include "Core/Gameboy.h"

#include <filesystem>
#include "Core/Cpu/Interrupts.h"


gb::Gameboy::Gameboy(std::filesystem::path &rom_path)
: joypad_{ std::make_shared<Joypad>() } {
    auto ints {std::make_shared<cpu::Interrupts>()};
    auto controller {memory::mbc::make_mbc(rom_path) };

    is_cgb_ = controller->is_cgb();

    cpu_ = std::make_unique<cpu::Cpu>(*this, ints, joypad_);
    gpu_ = std::make_unique<graphics::Ppu>(*this, ints);
    mmu_ = std::make_unique<memory::Memory>(*this, ints, std::move(controller));

    paused_ = false;
    speed_multiplier_ = 10;
}

void gb::Gameboy::step() {
    if ( paused_ )
        return;
    cpu_->step();
}

void gb::Gameboy::clock(unsigned int cycles) {
    unsigned int adjusted_cycles = (cycles * speed_multiplier_) / 10;
    if ( !mmu_->is_dma_completed() )
        mmu_->step_dma(adjusted_cycles);
    cpu_->update_timers(adjusted_cycles);
    gpu_->step(adjusted_cycles);
    apu_.step((cycles * 10u) / speed_multiplier_);
}