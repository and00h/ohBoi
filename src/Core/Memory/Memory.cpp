//
// Created by antonio on 29/07/20.
//

#include <util.h>
#include "Core/Memory/Memory.h"
#include "Core/Cpu/Cpu.h"
#include "Core/Graphics/Ppu.h"
#include "Core/Gameboy.h"
#include "Core/Memory/Address_space.h"
#include "Core/Memory/Wram.h"
#include "Core/Cpu/Interrupts.h"

namespace {
    using gb::memory::boundaries;
    using gb::memory::memory_areas_size;
    using gb::memory::io_boundaries;
    using gb::memory::io_sizes;

    typedef gb::util::Mem_range<boundaries, memory_areas_size> Mem_span;
    typedef gb::util::Mem_range<io_boundaries, io_sizes> IO_span;

    constexpr std::initializer_list<Mem_span> memory_map = {
            {boundaries::bank0_start, memory_areas_size::rom},
            {boundaries::vram_start, memory_areas_size::vram},
            {boundaries::extram_start, memory_areas_size::extram},
            {boundaries::wram_bank0_start, memory_areas_size::wram_bank0},
            {boundaries::wram_bank1_start, memory_areas_size::wram_bank1},
            {boundaries::echo_start,memory_areas_size::echo},
            {boundaries::oam_start, memory_areas_size::oam},
            {boundaries::prohibited_start, memory_areas_size::prohibited},
            {boundaries::io_start, memory_areas_size::io},
            {boundaries::hram_start, memory_areas_size::hram},
            {boundaries::int_enable_start, memory_areas_size::int_enable}
    };

    constexpr std::initializer_list<IO_span> io_ports_map = {
            {io_boundaries::io_head_start, io_sizes::io_head},
            {io_boundaries::apu_io_start, io_sizes::apu_io},
            {io_boundaries::gpu_io_start, io_sizes::gpu_io},
            {io_boundaries::io_tail_start, io_sizes::io_tail}
    };
}

gb::memory::Memory::Memory(gb::Gameboy &gb, std::shared_ptr<cpu::Interrupts> interrupts, std::unique_ptr<mbc::Mbc> controller)
    : gb_(gb), controller_(std::move(controller)), interrupts_(std::move(interrupts)), hram_(0x7F), io_ports_(0x80),
      wram_(0x1000 << (gb.is_cgb_ ? 3 : 1)),
      dma_controller_(*this),
      booting_(true) {
    std::ifstream b("DMG_ROM.bin", std::ios::binary | std::ios::in);
    b.read((char *) bootrom_.data(), 256);
    b.close();
}

uint8_t gb::memory::Memory::read(uint16_t addr) {
    auto memory_area = std::find_if(memory_map.begin(), memory_map.end(), [addr](Mem_span s) {
        return ((addr - s.start_) < s.size_);
    });

    // Not checking for std::find_if return value, because the GB's address space spans the whole range
    // of values representable on 16 bits, so addr will never be greater than 0xFFFF.

    switch (memory_area->start_) {
        case boundaries::bank0_start:
            return controller_->read(addr);
        case boundaries::vram_start:
            return gb_.gpu_->read_vram(addr - boundaries::vram_start);
        case boundaries::extram_start:
            return controller_->read_ram(addr - boundaries::extram_start);
        case boundaries::wram_bank0_start:
            return wram_.read(addr - boundaries::wram_bank0_start);
        case boundaries::wram_bank1_start:
            return wram_.read_bank_1(addr - boundaries::wram_bank1_start);
        case boundaries::echo_start:
            return read(addr - 0x2000);
        case boundaries::oam_start:
            return dma_controller_.is_running() ? 0xFF : gb_.gpu_->read_oam(addr - boundaries::oam_start);
        case boundaries::prohibited_start:
            break;
        case boundaries::io_start:
            return read_io_port(addr);
        case boundaries::hram_start:
            return hram_[addr - boundaries::hram_start];
        case boundaries::int_enable_start:
            return interrupts_->ie_flag();
        default:
            return 0xFF;
    }
    return 0xFF;
}

void gb::memory::Memory::write(uint16_t addr, uint8_t val) {
    auto memory_area = std::find_if(memory_map.begin(), memory_map.end(), [addr](Mem_span s) {
        return ((addr - s.start_) < s.size_);
    });

    switch (memory_area->start_) {
        case boundaries::bank0_start:
            controller_->write(addr, val);
            break;
        case boundaries::vram_start:
            gb_.gpu_->write_vram(addr - boundaries::vram_start, val);
            break;
        case boundaries::extram_start:
            controller_->write_ram(addr - boundaries::extram_start, val);
            break;
        case boundaries::wram_bank0_start:
            wram_.write(addr - boundaries::wram_bank0_start, val);
            break;
        case boundaries::wram_bank1_start:
            wram_.write_bank_1(addr - boundaries::wram_bank1_start, val);
            break;
        case boundaries::echo_start:
            write(addr - 0x2000, val);
            break;
        case boundaries::oam_start:
            if ( !dma_controller_.is_running() ) {
               gb_.gpu_->write_oam(addr - boundaries::oam_start, val);
            }
            break;
        case boundaries::prohibited_start:
            break;
        case boundaries::io_start:
            write_io_port(addr, val);
            break;
        case boundaries::hram_start:
            hram_[addr - boundaries::hram_start] = val;
            break;
        case boundaries::int_enable_start:
            interrupts_->set_ie(val);
            break;
    }
}

void gb::memory::Memory::step_dma(unsigned int cycles) {
    dma_controller_.step(cycles);
}

uint8_t gb::memory::Memory::read_io_port(uint16_t port_addr) {
    auto io_area = std::find_if(io_ports_map.begin(), io_ports_map.end(), [port_addr](IO_span s) {
        return (port_addr - s.start_) < s.size_;
    });

    switch (io_area->start_) {
        case io_boundaries::io_head_start:
        case io_boundaries::io_tail_start:
            switch (port_addr) {
                case io_ports::joypad_reg:
                    return gb_.joypad_->get_keys_reg();
                case io_ports::div_reg:
                    return gb_.cpu_->get_div_reg();
                case io_ports::tima:
                    return gb_.cpu_->get_tima();
                case io_ports::tac:
                    return gb_.cpu_->get_tac();
                case io_ports::tma:
                    return gb_.cpu_->get_tma();
                case io_ports::int_req:
                    return interrupts_->if_flag();
                case io_ports::wram_bank_select:
                    return wram_.get_bank();
                default:
                    return io_ports_[port_addr - boundaries::io_start];
            }
        case io_boundaries::apu_io_start:
            return gb_.apu_.read(port_addr);
        case io_boundaries::gpu_io_start:
            switch (port_addr) {
                case io_ports::cpu_double_speed:
                    return gb_.cpu_->double_speed() ? 0x80 : 0;
                case io_ports::dma_transfer:
                    return dma_controller_.get_index();
                default:
                    return gb_.gpu_->read(port_addr);
            }
    }
    return 0xFF;
}

void gb::memory::Memory::write_io_port(uint16_t port_addr, uint8_t val) {
    auto io_area = std::find_if(io_ports_map.begin(), io_ports_map.end(), [port_addr](IO_span s) {
        return (port_addr - s.start_) < s.size_;
    });

    uint8_t prev_tac;
    switch (io_area->start_) {
        case io_boundaries::io_head_start:
        case io_boundaries::io_tail_start:
            switch (port_addr) {
                case io_ports::serial_data:
                    printf("%x ", val);
                    break;
                case io_ports::joypad_reg:
                    gb_.joypad_->select_key_group(val);
                    break;
                case io_ports::div_reg:
                    gb_.cpu_->reset_div_reg();
                    break;
                case io_ports::tima:
                    gb_.cpu_->set_tima(val);
                    break;
                case io_ports::tma:
                    gb_.cpu_->set_tma(val);
                    break;
                case io_ports::tac:
                    prev_tac = gb_.cpu_->get_tac();
                    gb_.cpu_->set_tac(val);
                    if ( prev_tac != (val | 0xF8) ) {
                        gb_.cpu_->update_timer_counter();
                    }
                    break;
                case io_ports::int_req:
                    interrupts_->set_if(val);
                    break;
                case io_ports::wram_bank_select:
                    if ( gb_.is_cgb_ ) {
                        wram_.switch_bank((val & 7) == 0 ? 1 : (val & 7));
                    }
                    break;
                case io_ports::bootrom_enable:
                    io_ports_[port_addr - boundaries::io_start] = val;
                    if ( val & 1 )
                        booting_ = false;
                    break;
                default:
                    io_ports_[port_addr - boundaries::io_start] = val;
            }
            break;
        case io_boundaries::apu_io_start:
            gb_.apu_.send(port_addr, val);
            break;
        case io_boundaries::gpu_io_start:
            switch (port_addr) {
                case io_ports::cpu_double_speed:
                    gb_.cpu_->set_double_speed(val & 1);
                    break;
                case io_ports::dma_transfer:
                    dma_controller_.trigger(val);
                    break;
                default:
                    gb_.gpu_->send(port_addr, val);
                    break;
            }
            break;
    }
}

void gb::memory::Memory::dma_write_oam(uint16_t addr, uint8_t val) {
    gb_.gpu_->write_oam(addr, val);
}