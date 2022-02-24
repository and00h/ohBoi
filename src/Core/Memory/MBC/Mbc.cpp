//
// Created by antonio on 02/08/21.
//
#include "Core/Memory/MBC/Mbc.h"

#include <cstdint>
#include <memory>

#include "Core/Memory/MBC/Mbc1.h"
#include "Core/Memory/MBC/Mbc3.h"
#include "Core/Memory/MBC/Mbc5.h"
#include "Core/Memory/MBC/None.h"
#include "Core/Memory/MBC/RTC.h"
#include "Core/Memory/Rom.h"

using gb::memory::mbc::Mbc;

struct cartridge_header {
    uint8_t entry_point[4];
    uint8_t logo[48];
    union {
        struct {
            uint8_t cgb_flag;
            char manufacturer_code[4];
            char title[11];
        };
        char title16[0x10];
    };
    uint8_t licensee_code[2];
    uint8_t sgb_flag;
    uint8_t cart_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t dest_code;
    uint8_t old_lic_code;
    uint8_t version;
    uint8_t checksum;
    uint8_t global_checksum[2];
} __attribute__ ((__packed__));

static constexpr unsigned int ram_size_map[6] = {
        1, 2048, 8192, 32768, 131072, 65535
};

Mbc::Mbc(std::filesystem::path& rom_path, bool battery, bool rtc, bool has_ram, unsigned int rom_banks,
         unsigned int ram_banks)
        : mCartridge(rom_path, 0x4000 * rom_banks),
          mRAM(ram_size_map[ram_banks], rom_path.replace_extension(".sav")),
          mHasBattery(battery),
          mHasRtc(rtc),
          mHasRam(has_ram),
          nRomBanks(rom_banks),
          nRamBanks(ram_banks),
          mBankingMode(rom_mode),
          cgb(mCartridge.read(0x143)) {}

std::unique_ptr<Mbc> gb::memory::mbc::make_mbc(std::filesystem::path& rom_path) {
    cartridge_header cart_hdr{};

    std::ifstream romFile(rom_path.c_str(), std::ios::binary);
    romFile.seekg(0x100, std::ifstream::beg);
    romFile.read(reinterpret_cast<char *>(&cart_hdr), sizeof(cart_hdr));
    romFile.close();

    bool battery = false;
    bool hasRam = false;
    bool timer = false;

    unsigned int romBanks = (0x8000 << cart_hdr.rom_size) / mbc::rom_bank_size;
    unsigned int ramBanks = ram_size_map[cart_hdr.ram_size] / mbc::ram_bank_size;

    switch (cart_hdr.cart_type) {
        case mbc_type::NONE:
            return std::make_unique<None>(rom_path);
        case mbc_type::MBC1_RB:
            battery = true;
            [[fallthrough]];
        case mbc_type::MBC1_R:
            hasRam = true;
            [[fallthrough]];
        case mbc_type::MBC1:
            return std::make_unique<Mbc1>(rom_path, battery, hasRam, romBanks, ramBanks);
        case mbc_type::MBC3_TRB:
            hasRam = true;
            [[fallthrough]];
        case mbc_type::MBC3_TB:
            timer = true;
            battery = true;
            return std::make_unique<Mbc3>(rom_path, battery, hasRam, timer, romBanks, ramBanks);
        case mbc_type::MBC3_RB:
            battery = true;
            [[fallthrough]];
        case mbc_type::MBC3_R:
            hasRam = true;
            [[fallthrough]];
        case mbc_type::MBC3:
            return std::make_unique<Mbc3>(rom_path, battery, hasRam, timer, romBanks, ramBanks);
        case mbc_type::MBC5_RB:
            battery = true;
            [[fallthrough]];
        case mbc_type::MBC5_R:
            hasRam = true;
            [[fallthrough]];
        case mbc_type::MBC5:
            return std::make_unique<Mbc5>(rom_path, battery, hasRam, romBanks, ramBanks);
        default:
            return std::make_unique<None>(rom_path);
    }
}