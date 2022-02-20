//
// Created by antonio on 29/07/20.
//

#include <Core/Memory/MBC/Mbc3.h>

using gb::memory::mbc::Mbc3;

Mbc3::Mbc3(std::filesystem::path& rom_path, bool battery, bool hasRam, bool rtc, unsigned int rom_banks, unsigned int ram_banks)
        : Mbc(rom_path, battery, rtc, hasRam, rom_banks, ram_banks),
          mbc3_ram_rtc_select(0),
          mbc3_rom_bank(1),
          latch(0) {
    this->mRAM.set_enabled(false);
}

void Mbc3::write(uint16_t addr, uint8_t val) {
    if ( addr <= 0x1FFF ) {
        mRAM.set_enabled((val & 0xF) == 0xA);
    }
    else if ( addr >= 0x2000 && addr <= 0x3FFF ) {
        mbc3_rom_bank = ((val & 0x7F) == 0) ? 1 : (val & 0x7F);
    }
    else if ( addr >= 0x4000 && addr <= 0x5FFF ) {
        mbc3_ram_rtc_select = (val & 0xF) % 0xD;
    }
    else if ( addr >= 0x6000 && addr <= 0x7FFF ) {
        if ( mHasRtc && !latch && ( val & 1) == 1) {
            rtc_clock.latch_time();
        }
        latch = val & 1;
    }
}

uint8_t Mbc3::read(uint16_t addr) {
    unsigned int bank = addr < 0x4000 ? 0 : mbc3_rom_bank;
    bank %= nRomBanks;
    return mCartridge.read(bank * rom_bank_size + (addr % 0x4000));
}

void Mbc3::write_ram(uint16_t addr, uint8_t val) {
    if ( mRAM.is_enabled() ) {
        if ( mbc3_ram_rtc_select < 4 && mHasRam )
            mRAM.write(mbc3_ram_rtc_select * ram_bank_size + addr, val);
        else if ( mHasRtc && mbc3_ram_rtc_select >= 0x8 && mbc3_ram_rtc_select <= 0xC ) {
            rtc_clock.update_time();
            switch (mbc3_ram_rtc_select) {
                case 0x8:
                    rtc_clock.set_secs(val);
                    break;
                case 0x9:
                    rtc_clock.set_mins(val);
                    break;
                case 0xA:
                    rtc_clock.set_hrs(val);
                    break;
                case 0xB:
                    rtc_clock.set_day_lo(val);
                    break;
                case 0xC:
                    rtc_clock.set_day_hi(val);
                    break;
            }
        }
    }
}

uint8_t Mbc3::read_ram(uint16_t addr) {
    if ( mRAM.is_enabled() ) {
        if ( mbc3_ram_rtc_select < 4 && mHasRam ) {
            return mRAM.read(mbc3_ram_rtc_select * ram_bank_size + addr);
        }
        else if ( mHasRtc && mbc3_ram_rtc_select >= 0x8 && mbc3_ram_rtc_select <= 0xC ) {
            switch (mbc3_ram_rtc_select) {
                case 0x8:
                    return rtc_clock.get_latch_secs();
                case 0x9:
                    return rtc_clock.get_latch_mins();
                case 0xA:
                    return rtc_clock.get_latch_hrs();
                case 0xB:
                    return rtc_clock.get_latch_day_lo();
                case 0xC:
                    return rtc_clock.get_latch_day_hi();
            }
        }
    }
    return 0xFF;
}
