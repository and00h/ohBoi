//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_MBC3_H
#define OHBOI_MBC3_H


#include <Core/Memory/MBC/Mbc.h>
#include <Core/Memory/MBC/RTC.h>

namespace gb::memory::mbc {
    class Mbc3 : public gb::memory::mbc::Mbc {
    public:
        Mbc3(std::filesystem::path& rom_path, bool battery, bool hasRam, bool rtc, unsigned int rom_banks, unsigned int ram_banks);

        uint8_t read(uint16_t) override;
        void write(uint16_t, uint8_t val) override;
        uint8_t read_ram(uint16_t) override;
        void write_ram(uint16_t, uint8_t) override;

        void write_rtc(std::ofstream& out) { if ( mHasRtc ) rtc_clock.write_saved_time(out, 48); }
        void read_rtc(std::ifstream& in) { if ( mHasRtc ) rtc_clock.read_saved_time(in, 48); }
    private:
        uint8_t mbc3_ram_rtc_select;
        uint8_t mbc3_rom_bank;

        uint8_t latch;
        RTC rtc_clock;
    };
}


#endif //OHBOI_MBC3_H
