//
// Created by antonio on 29/07/20.
//

#include <Core/Memory/MBC/Mbc5.h>

using gb::memory::mbc::Mbc5;

Mbc5::Mbc5(std::filesystem::path& rom_path, bool battery, bool has_ram, unsigned int rom_banks, unsigned int ram_banks)
    : Mbc(rom_path, battery, false, has_ram, rom_banks, ram_banks) {
    mbc5_rom_lo = 1;
    mbc5_rom_hi = 0;
    mbc5_ram_bank = 0;
    if ( has_ram ) ram_.set_enabled(false);
}

void Mbc5::write(uint16_t addr, uint8_t val) {
    if ( addr <= 0x1FFF )
        ram_.set_enabled((val & 0xF) == 0xA);
    if ( addr >= 0x2000 && addr <= 0x2FFF )
        mbc5_rom_lo = val;
    if ( addr >= 0x3000 && addr <= 0x3FFF )
        mbc5_rom_hi = val & 1;
    if ( addr >= 0x4000 && addr <= 0x5FFF )
        mbc5_ram_bank = val & 0xF;
}

uint8_t Mbc5::read(uint16_t addr) {
    unsigned int bank = 0;
    if ( addr >= 0x4000 ) {
        bank = mbc5_rom_lo;
        bank |= ( (unsigned int) (mbc5_rom_hi) << 8);
        bank &= 0x1FF;
    }
    bank %= rom_banks_n;
    addr %= rom_bank_size;
    return rom_.read(bank * rom_bank_size + addr);
}
