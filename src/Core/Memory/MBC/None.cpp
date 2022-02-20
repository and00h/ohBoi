//
// Created by antonio on 29/07/20.
//

#include <Core/Memory/MBC/None.h>

uint8_t gb::memory::mbc::None::read(uint16_t addr) {
    return this->mCartridge.read(addr);
}

uint8_t gb::memory::mbc::None::read_ram(uint16_t) {
    return 0xFF;
}