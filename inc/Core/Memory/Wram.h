//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_WRAM_H
#define OHBOI_WRAM_H

#include "Core/Memory/Address_space.h"

namespace gb::memory {
    class Wram : public Address_space {
    public:
        explicit Wram(unsigned int size) : Address_space(size) { clear(); bank = 1; };

        uint8_t read_bank_1(unsigned int addr) {
            if ( addr >= 0x1000 ) {
                return 0xFF;
            }
            return m_[bank * 0x1000 + addr];
        }
        void write_bank_1(unsigned int addr, uint8_t val) {
            if ( addr >= 0x1000 ) {
                return;
            }
            m_[bank * 0x1000 + addr] = val;
        }
        void switch_bank(int b) {
            this->bank = b;
        }
        int get_bank() {
            return bank;
        }
    private:
        int bank;
    };
}


#endif //OHBOI_WRAM_H
