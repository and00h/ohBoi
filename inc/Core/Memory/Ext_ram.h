//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_EXT_RAM_H
#define OHBOI_EXT_RAM_H

#include <fstream>
#include <filesystem>
#include "Core/Memory/Address_space.h"

namespace gb::memory {
    class Ext_ram : public gb::memory::Address_space {
    public:
        Ext_ram(unsigned int size, std::filesystem::path& sav_path) : Address_space(size), enabled(true), sav_path(sav_path) {
            load_from_savfile(sav_path);
        };

        ~Ext_ram() override {
            save_to_savfile(sav_path);
        }

        uint8_t read(unsigned int address) override { return enabled ? this->m_[address] : 0xFF; };
        void write(unsigned int address, uint8_t value) override { if (enabled) m_[address] = value; };

        void set_enabled(bool e) { enabled = e; }
        [[nodiscard]] bool is_enabled() const { return enabled; }

        void save_to_savfile(const std::filesystem::path& sav_path) {
            std::ofstream out(sav_path.c_str(), std::ios::out | std::ios::binary);
            out.write((char *) m_.data(), size_);
            out.close();
        }

        void load_from_savfile(const std::filesystem::path& sav_path) {
            if ( std::filesystem::exists(sav_path) ) {
                std::ifstream in(sav_path, std::ios::in | std::ios::binary);
                in.read((char *) m_.data(), size_);
                in.close();
            }
        }

    private:
        bool enabled;
        std::filesystem::path sav_path;
    };
}


#endif //OHBOI_EXT_RAM_H
