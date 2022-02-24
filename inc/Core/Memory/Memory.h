//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_MEMORY_H
#define OHBOI_MEMORY_H

#include <iostream>

#include <Core/Memory/MBC/Mbc.h>
#include <Core/Memory/Wram.h>
#include "Core/cpu/Interrupts.h"
#include "Core/Memory/Address_space.h"
#include "Ppu.h"

// Forward declaration
namespace gb {
    class Gameboy;
}

namespace gb::memory {
    enum io_ports: uint16_t;

    class Memory {
    public:
        Memory(gb::Gameboy &gb, std::shared_ptr<gb::cpu::Interrupts> interrupts, std::unique_ptr<mbc::Mbc> controller);
        ~Memory() = default;

        uint8_t read(uint16_t addr);
        void write(uint16_t addr, uint8_t val);

        void step_dma(unsigned int cycles);
        [[nodiscard]] bool is_dma_completed() const { return dma_controller_.is_completed(); }
    private:
        class Dma_controller {
        public:
            Dma_controller(Memory &mem_);

            void trigger(uint8_t index);
            void step(int cycles);

            [[nodiscard]] uint8_t get_index() const { return mem_index_; }
            [[nodiscard]] bool is_running() const { return !(dma_completed_ || dma_wait_); }
            [[nodiscard]] bool is_completed() const { return dma_completed_; }
        private:
            Memory& mem_;
            uint8_t mem_index_;
            uint16_t dma_addr_;
            bool dma_completed_;
            unsigned int dma_index_;
            bool dma_trigger_;
            bool dma_wait_;
        };

        Gameboy& gb_;
        Dma_controller dma_controller_;

        std::unique_ptr<mbc::Mbc> controller_;
        std::shared_ptr<cpu::Interrupts> interrupts_;

        Address_space hram_;
        Address_space io_ports_;
        std::array<char, 256> bootrom_;
        Wram wram_;

        bool booting_;

        uint8_t read_io_port(uint16_t port_addr);
        void write_io_port(uint16_t port_addr, uint8_t val);

        void dma_write_oam(uint16_t addr, uint8_t val);
    };

    enum boundaries: uint16_t {
        bank0_start      = 0,
        vram_start       = 0x8000,
        extram_start     = 0xA000,
        wram_bank0_start = 0xC000,
        wram_bank1_start = 0xD000,
        echo_start       = 0xE000,
        oam_start        = 0xFE00,
        prohibited_start = 0xFEA0,
        io_start         = 0xFF00,
        hram_start       = 0xFF80,
        int_enable_start = 0xFFFF
    };

    enum memory_areas_size: uint16_t {
        rom              = 0x8000,
        vram             = 0x2000,
        extram           = 0x2000,
        wram_bank0       = 0x1000,
        wram_bank1       = 0x1000,
        echo             = 0x1E00,
        oam              = 0x00A0,
        prohibited       = 0x0060,
        io               = 0x0080,
        hram             = 0x007F,
        int_enable       = 1
    };

    enum io_ports: uint16_t {
        joypad_reg       = 0xFF00,
        serial_data      = 0xFF01,
        serial_control   = 0xFF02,
        div_reg          = 0xFF04,
        tima             = 0xFF05,
        tma              = 0xFF06,
        tac              = 0xFF07,
        int_req          = 0xFF0F,
        dma_transfer     = 0xFF46,
        cpu_double_speed = 0xFF4D,
        bootrom_enable   = 0xFF50,
        wram_bank_select = 0xFF70,
        undocumented_1   = 0xFF72,
        undocumented_2   = 0xFF74,
        undocumented_3   = 0xFF75,
        pcm_12           = 0xFF76,
        pcm_34           = 0xFF77
    };

    enum io_boundaries: uint16_t {
        io_head_start    = 0xFF00,
        apu_io_start     = 0xFF10,
        gpu_io_start     = 0xFF40,
        io_tail_start    = 0xFF6D
    };

    enum io_sizes: uint16_t {
        io_head          = 0x10,
        apu_io           = 0x30,
        gpu_io           = 0x2D,
        io_tail          = 0x14
    };
}


#endif //OHBOI_MEMORY_H
