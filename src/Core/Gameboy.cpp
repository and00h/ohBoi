//
// Created by antonio on 01/08/20.
//

#include "Core/Gameboy.h"

#include <filesystem>
#include "Core/cpu/Interrupts.h"


gb::Gameboy::Gameboy(std::filesystem::path &rom_path)
: joypad_{ std::make_shared<Joypad>() } {
    auto ints {std::make_shared<cpu::Interrupts>()};
    std::unique_ptr<memory::mbc::Mbc> controller {memory::mbc::make_mbc(rom_path) };

    is_cgb_ = controller->is_cgb();

    cpu_ = std::make_unique<cpu::Cpu>(*this, ints, joypad_);
    gpu_ = std::make_unique<graphics::Ppu>(*this, ints);
    mmu_ = std::make_unique<memory::Memory>(*this, ints, std::move(controller));

    paused_ = false;
    speed_multiplier_ = 1.0;
}

void gb::Gameboy::step() {
    if ( paused_ )
        return;
    cpu_->step();
}

void gb::Gameboy::clock(unsigned int cycles) {
    if ( !mmu_->is_dma_completed() )
        mmu_->step_dma(cycles * speed_multiplier_);
    cpu_->update_timers(cycles * speed_multiplier_);
    gpu_->step(cycles * speed_multiplier_);
    apu_.step(static_cast<int>(cycles / speed_multiplier_));
}