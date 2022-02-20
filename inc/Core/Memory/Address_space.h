//
// Created by antonio on 27/07/20.
//

#ifndef OHBOI_ADDRESS_SPACE_H
#define OHBOI_ADDRESS_SPACE_H

#include <cstdint>
#include <cstring>

#include <vector>

namespace gb::memory {
    class Address_space {
    public:
        explicit Address_space(unsigned int space_size) : size_ {space_size} {
            m_.reserve(space_size);
        }
        virtual ~Address_space() = default;

        virtual uint8_t read(unsigned int address) {
            return m_[address];
        }
        virtual void write(unsigned int address, uint8_t value) {
            m_[address] = value;
        }
        virtual uint8_t& operator[](unsigned int i) {
            return m_[i];
        }

        void clear() { std::fill(m_.begin(), m_.end(), 0); }
    protected:
        std::vector<uint8_t> m_;
        unsigned int size_;
    };
}


#endif //OHBOI_ADDRESS_SPACE_H
