//
// Created by antonio on 29/07/20.
//

#include <Core/Memory/MBC/Mbc1.h>

using gb::memory::mbc::Mbc1;

Mbc1::Mbc1(std::filesystem::path& rom_path, bool battery, bool has_ram, unsigned int rom_banks, unsigned int ram_banks)
    : Mbc(rom_path, battery, false, has_ram, rom_banks, ram_banks) {
    ram_.set_enabled(false);
    banking_mode_ = rom_mode;
    mRomBankHi = 0;
    mRomBankLo = 1;
}

void Mbc1::write(uint16_t addr, uint8_t val) {
    if ( addr <= 0x1FFF )
        ram_.set_enabled((val & 0xF) == 0xA);
    if ( addr >= 0x2000 && addr <= 0x3FFF )
        mRomBankLo = ( ( val & 0x1F) == 0) ? 1 : val & 0x1F;
    if ( addr >= 0x4000 && addr <= 0x5FFF )
        mRomBankHi = val & 0x3;
    if ( addr >= 0x6000 && addr <= 0x7FFF )
        banking_mode_ = val == 0 ? rom_mode : ram_mode;
}

uint8_t Mbc1::read(uint16_t addr) {
    unsigned int bank_offset = addr < 0x4000 ? 0 : ( ( (mRomBankHi << 5) | mRomBankLo) & 0x7F);
    if ( bank_offset == 0 && banking_mode_ == ram_mode )
        bank_offset = mRomBankHi << 5;

    bank_offset %= rom_banks_n;
    addr %= rom_bank_size;

    return rom_.read(0x4000 * bank_offset + addr);
}

void Mbc1::write_ram(uint16_t addr, uint8_t val) {
    unsigned int bank = banking_mode_ == ram_mode ? mRomBankHi : 0;
    if ( addr >= 0xA000) addr -= 0xA000;
    ram_.write(bank * ram_bank_size + addr, val);
}

uint8_t Mbc1::read_ram(uint16_t addr) {
    unsigned int bank = banking_mode_ == ram_mode ? mRomBankHi : 0;
    if ( addr >= 0xA000 )
        addr -= 0xA000;
    return ram_.read(bank * ram_bank_size + addr);
}
