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

namespace {
    constexpr unsigned int ram_size_map[6] = {
            1, 2048, 8192, 32768, 131072, 65535
    };
}

struct cartridge_header {
    [[maybe_unused]] uint8_t entry_point[4];
    [[maybe_unused]] uint8_t logo[48];
    union {
        struct {
            [[maybe_unused]] uint8_t cgb_flag;
            [[maybe_unused]] char manufacturer_code[4];
            [[maybe_unused]] char title[11];
        };
        [[maybe_unused]] char title16[0x10];
    };
    [[maybe_unused]] uint8_t licensee_code[2];
    [[maybe_unused]] uint8_t sgb_flag;
    uint8_t cart_type;
    uint8_t rom_size;
    uint8_t ram_size;
    [[maybe_unused]] uint8_t dest_code;
    [[maybe_unused]] uint8_t old_lic_code;
    [[maybe_unused]] uint8_t version;
    [[maybe_unused]] uint8_t checksum;
    [[maybe_unused]] uint8_t global_checksum[2];
} __attribute__ ((__packed__));

Mbc::Mbc(std::filesystem::path& rom_path, bool battery, bool rtc, bool has_ram, unsigned int rom_banks,
         unsigned int ram_banks) :
          rom_(rom_path, 0x4000 * rom_banks),
          ram_(ram_size_map[ram_banks]),
          rom_path_(rom_path),
          has_battery_(battery),
          has_rtc_(rtc),
          has_ram_(has_ram),
          rom_banks_n(rom_banks),
          ram_banks_n(ram_banks),
          banking_mode_(rom_mode),
          cgb(rom_.read(0x143)) {
    std::ifstream in {rom_path.replace_extension(".sav")};
    if ( has_battery_ ) {
        ram_.load_from_savfile(in);
    }
    if ( has_rtc_ ) {
        rtc_clock.read_saved_time(in, 48);
    }
}

std::unique_ptr<Mbc> gb::memory::mbc::make_mbc(std::filesystem::path& rom_path) {
    cartridge_header cart_hdr{};

    std::ifstream rom_file { rom_path.c_str(), std::ios::binary };
    rom_file.seekg(0x100, std::ifstream::beg);
    rom_file.read(reinterpret_cast<char *>(&cart_hdr), sizeof(cart_hdr));
    rom_file.close();

    bool battery = false;
    bool has_ram = false;
    bool timer = false;

    unsigned int rom_banks_n = (0x8000 << cart_hdr.rom_size) / mbc::rom_bank_size;
    unsigned int ram_banks_n = ram_size_map[cart_hdr.ram_size] / mbc::ram_bank_size;

    switch (cart_hdr.cart_type) {
        case mbc_type::NONE:
            return std::make_unique<None>(rom_path);

        case mbc_type::MBC1_RB:     battery = true; [[fallthrough]];
        case mbc_type::MBC1_R:      has_ram = true; [[fallthrough]];
        case mbc_type::MBC1:
            return std::make_unique<Mbc1>(rom_path, battery, has_ram, rom_banks_n, ram_banks_n);

        case mbc_type::MBC3_TRB:    has_ram = true; [[fallthrough]];
        case mbc_type::MBC3_TB:     timer = true; battery = true;
            return std::make_unique<Mbc3>(rom_path, battery, has_ram, timer, rom_banks_n, ram_banks_n);

        case mbc_type::MBC3_RB:     battery = true; [[fallthrough]];
        case mbc_type::MBC3_R:      has_ram = true; [[fallthrough]];
        case mbc_type::MBC3:
            return std::make_unique<Mbc3>(rom_path, battery, has_ram, timer, rom_banks_n, ram_banks_n);

        case mbc_type::MBC5_RB:     battery = true; [[fallthrough]];
        case mbc_type::MBC5_R:      has_ram = true; [[fallthrough]];
        case mbc_type::MBC5:
            return std::make_unique<Mbc5>(rom_path, battery, has_ram, rom_banks_n, ram_banks_n);

        default:
            return std::make_unique<None>(rom_path);
    }
}