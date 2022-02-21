//
// Created by antonio on 2/21/22.
//

#include "Core/Memory/Memory.h"

namespace gb::memory {
    Memory::Dma_controller::Dma_controller(Memory &mem_) :
            mem_{mem_},
            mem_index_{0},
            dma_addr_{0},
            dma_completed_{true},
            dma_index_{0},
            dma_trigger_{false},
            dma_wait_{false}
        {};

    void Memory::Dma_controller::trigger(uint8_t index) {
        mem_index_ = index;

        dma_completed_ = false;

        dma_addr_ = ((uint16_t) index) << 8;
        dma_index_ = 0;
        dma_trigger_ = true;
    }

    void Memory::Dma_controller::step(int cycles) {
        static const uint8_t dma_size = 0xA0;

        while ( cycles > 0 && !dma_completed_ ) {
            if ( dma_trigger_ ) {
                dma_trigger_ = false;
                dma_wait_ = true;
                cycles -= 4;
                continue;
            }
            if ( !dma_wait_ ) {
                mem_.dma_write_oam(dma_index_, mem_.read(dma_addr_ + dma_index_));
                if (++dma_index_ == dma_size ) {
                    dma_trigger_ = false;
                    dma_completed_ = true;
                }
            } else
                dma_wait_ = false;
            cycles -= 4;
        }
    }
}