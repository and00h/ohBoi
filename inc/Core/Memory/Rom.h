//
// Created by antonio on 27/07/20.
//

#ifndef OHBOI_ROM_H
#define OHBOI_ROM_H

#include <filesystem>
#include <fstream>
#include <iterator>

#include "Core/Memory/Address_space.h"

namespace gb::memory {
    class Rom : public gb::memory::Address_space {
    public:
        Rom(std::filesystem::path& rom_path, unsigned int size) : Address_space(size) {
            std::ifstream file(rom_path.c_str(), std::ios::in | std::ios::binary);
            m_.reserve(size);
            file.read((char *) m_.data(), size);
            //m_.insert(m_.begin(), std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>());
        };
    private:
        // Cannot write to ROM. The Mbc takes care of eventual writes to ROM address space, so make Rom::write private.
        using Address_space::write;
    };
}


#endif //OHBOI_ROM_H
