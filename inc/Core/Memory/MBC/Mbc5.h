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

        [[nodiscard]] uint8_t read(uint16_t) override;
        void write(uint16_t, uint8_t) override;

        [[nodiscard]] uint8_t read_ram(uint16_t addr) override {
            return (has_ram_ && ram_.is_enabled()) ? ram_.read(mbc5_ram_bank * ram_bank_size + addr) : 0xFF;
        }
        void write_ram(uint16_t addr, uint8_t val) override {
            if ( has_ram_ && ram_.is_enabled() ) {
                ram_.write(mbc5_ram_bank * ram_bank_size + addr, val);
            }
        }
    private:
        uint8_t mbc5_rom_lo;
        uint8_t mbc5_rom_hi;
        uint8_t mbc5_ram_bank;
    };
}


#endif //OHBOI_MBC5_H
