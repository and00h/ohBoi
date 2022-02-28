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

    private:
        uint8_t mbc3_ram_rtc_select;
        uint8_t mbc3_rom_bank;
    };
}


#endif //OHBOI_MBC3_H
