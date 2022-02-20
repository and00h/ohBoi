//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_MBC5_H
#define OHBOI_MBC5_H


#include <Core/Memory/MBC/Mbc.h>

namespace gb::memory::mbc {
    class Mbc5 : public gb::memory::mbc::Mbc {
    public:
        Mbc5(std::filesystem::path& rom_path, bool battery, bool has_ram, unsigned int rom_banks, unsigned int ram_banks);
        ~Mbc5() override = default;

        uint8_t read(uint16_t) override;
        void write(uint16_t, uint8_t val) override;
        uint8_t read_ram(uint16_t) override;
        void write_ram(uint16_t, uint8_t) override;

//        void write_rtc(std::ofstream& out);
//        void read_rtc(std::ifstream& in);
    private:
        uint8_t mbc5_rom_lo;
        uint8_t mbc5_rom_hi;
        uint8_t mbc5_ram_bank;
    };
}


#endif //OHBOI_MBC5_H
