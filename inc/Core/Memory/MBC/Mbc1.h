//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_MBC1_H
#define OHBOI_MBC1_H

#include <Core/Memory/MBC/Mbc.h>

namespace gb::memory::mbc {
    class Mbc1 : public Mbc {
    public:
        Mbc1(std::filesystem::path& rom_path, bool battery, bool has_ram, unsigned int rom_banks, unsigned int ram_banks);

        uint8_t read(uint16_t) override;
        void write(uint16_t, uint8_t val) override;
        uint8_t read_ram(uint16_t) override;
        void write_ram(uint16_t, uint8_t) override;
    private:
        int mRomBankLo;
        int mRomBankHi;
    };
}


#endif //OHBOI_MBC1_H
