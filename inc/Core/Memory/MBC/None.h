//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_NONE_H
#define OHBOI_NONE_H

#include <Core/Memory/MBC/Mbc.h>


namespace gb::memory::mbc {
    class None : public Mbc {
    public:
        explicit None(std::filesystem::path& rom_path)
                : Mbc(rom_path, false, false, false, 2, 0) {
            mRAM.set_enabled(false);
        }

        uint8_t read(uint16_t) override;
        uint8_t read_ram(uint16_t) override;
    private:
        [[maybe_unused]] void write(uint16_t, uint8_t) override {};
        [[maybe_unused]] void write_ram(uint16_t, uint8_t) override {};
    };
}


#endif //OHBOI_NONE_H
