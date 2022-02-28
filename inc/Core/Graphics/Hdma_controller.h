//
// Created by antonio on 2/25/22.
//

#ifndef OHBOI_HDMA_CONTROLLER_H
#define OHBOI_HDMA_CONTROLLER_H

#include <cstdint>
#include "util.h"

namespace gb {
    class Gameboy;
}

namespace gb::graphics {
    class Hdma_controller {
    public:
        explicit Hdma_controller(gb::Gameboy& gb);

        void launch_gp_hdma();
        void step();

        [[nodiscard]] bool is_running() { return hdma_running_; }

        void set_length(uint8_t len);
        [[nodiscard]] uint8_t get_length() const { return hdma_len_mode_.val; }

        union {
            uint16_t val;
            gb::util::Bit_field<uint16_t, 0, 8> lsb;
            gb::util::Bit_field<uint16_t, 8, 8> msb;
        } hdma_src_;

        union {
            uint16_t val;
            gb::util::Bit_field<uint16_t, 0, 8> lsb;
            gb::util::Bit_field<uint16_t, 8, 8> msb;
        } hdma_dst_;

    private:

        union {
            uint8_t val;
            gb::util::Bit_field<uint8_t, 0, 7> len;
            gb::util::Bit_field<uint8_t, 7, 1> type;
        } hdma_len_mode_;

        bool hdma_running_;
        gb::Gameboy& gb_;
    };
}

#endif //OHBOI_HDMA_CONTROLLER_H
