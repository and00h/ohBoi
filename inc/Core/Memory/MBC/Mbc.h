//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_MBC_H
#define OHBOI_MBC_H


#include <Core/Memory/Rom.h>
#include <Core/Memory/Ext_ram.h>
#include "RTC.h"

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

        ~Mbc() {
            std::ofstream out {rom_path_.replace_extension(".sav")};
            if ( has_battery_ ) {
                ram_.save_to_savfile(out);
            }
            if ( has_rtc_ ) {
                rtc_clock.write_saved_time(out, 48);
            }
        };
        virtual uint8_t read(uint16_t) = 0;
        virtual void write(uint16_t, uint8_t) = 0;
        virtual uint8_t read_ram(uint16_t) = 0;
        virtual void write_ram(uint16_t, uint8_t) = 0;

        [[nodiscard]] bool has_battery() const { return has_battery_; }
        [[nodiscard]] bool has_rtc() const { return has_rtc_; }
        [[nodiscard]] bool is_cgb() const { return cgb; }

    protected:
        Rom rom_;
        Ext_ram ram_;
        uint8_t latch;
        RTC rtc_clock;

        std::filesystem::path rom_path_;

        bool has_battery_;
        bool has_rtc_;
        bool has_ram_;

        unsigned int rom_banks_n;
        unsigned int ram_banks_n;

        mbc_banking_mode banking_mode_;
    private:
        bool cgb;
    };

    std::unique_ptr<Mbc> make_mbc(std::filesystem::path& rom_path);
}


#endif //OHBOI_MBC_H
