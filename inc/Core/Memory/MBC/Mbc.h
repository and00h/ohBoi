//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_MBC_H
#define OHBOI_MBC_H


#include <Core/Memory/Rom.h>
#include <Core/Memory/Ext_ram.h>

namespace gb::memory::mbc {
    enum mbc_banking_mode {
        rom_mode,
        ram_mode
    };

    enum mbc_type {
        NONE = 0,
        MBC1, MBC1_R, MBC1_RB,
        MBC3_TB = 0x0F, MBC3_TRB, MBC3, MBC3_R, MBC3_RB,
        MBC5 = 0x19, MBC5_R, MBC5_RB, MBC5_RUM, MBC5_R_RUM, MBC5_RB_RUM
    };

    const unsigned int rom_bank_size = 0x4000;
    const unsigned int ram_bank_size = 0x2000;

    class Mbc {
    public:
        Mbc(std::filesystem::path &rom_path, bool battery, bool rtc, bool has_ram, unsigned int rom_banks,
            unsigned int ram_banks);

        virtual ~Mbc() = default;
        virtual uint8_t read(uint16_t) = 0;
        virtual void write(uint16_t, uint8_t) = 0;
        virtual uint8_t read_ram(uint16_t) = 0;
        virtual void write_ram(uint16_t, uint8_t) = 0;

        [[nodiscard]] bool has_battery() const { return mHasBattery; }
        [[nodiscard]] bool has_rtc() const { return mHasRtc; }
        [[nodiscard]] bool is_cgb() const { return cgb; }

    protected:
        Rom mCartridge;
        Ext_ram mRAM;

        bool mHasBattery;
        bool mHasRtc;
        bool mHasRam;

        unsigned int nRomBanks;
        unsigned int nRamBanks;

        mbc_banking_mode mBankingMode;
    private:
        bool cgb;
    };

    std::unique_ptr<Mbc> make_mbc(std::filesystem::path& rom_path);
}


#endif //OHBOI_MBC_H
