//
// Created by antonio on 2/25/22.
//

#include <ranges>
#include "Core/Gameboy.h"
#include "Hdma_controller.h"

namespace gb::graphics {
    Hdma_controller::Hdma_controller(gb::Gameboy& gb) :
    hdma_src_{0},
    hdma_dst_{0},
    hdma_len_mode_{0},
    hdma_running_{false},
    gb_{gb}
    {}

    void Hdma_controller::launch_gp_hdma() {
        using std::ranges::iota_view;

        int len = (hdma_len_mode_.len + 1) << 4;
        for ( uint16_t i : iota_view{0, len} )
            gb_.mmu_->write(hdma_dst_.val + i, gb_.mmu_->read(hdma_src_.val + i));
        hdma_len_mode_.val = 0xFF;
    }

    void Hdma_controller::step() {
        using std::ranges::iota_view;

        uint16_t index = hdma_len_mode_.len << 4;
        uint16_t src = hdma_src_.val + index;
        uint16_t dst = hdma_dst_.val + index;
        for ( uint16_t i : iota_view{0, 0x10})
            gb_.mmu_->write(dst + i, gb_.mmu_->read(src + i));
        if (--hdma_len_mode_.val == 0xFF )
            hdma_running_ = false;
    }

    void Hdma_controller::set_length(uint8_t len) {
        if ( not hdma_running_ ) {
            hdma_len_mode_.val = len;
            if ( hdma_len_mode_.type == 0 ) {
                launch_gp_hdma();
            } else {
                hdma_running_ = true;
                hdma_len_mode_.type = 0;
            }
        }
    }
}