//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_REGISTERS_H
#define OHBOI_REGISTERS_H

#include <bitset>

const unsigned int REG_B = 0;
const unsigned int REG_C = 1;
const unsigned int REG_D = 2;
const unsigned int REG_E = 3;
const unsigned int REG_H = 4;
const unsigned int REG_L = 5;
const unsigned int REG_F = 6;
const unsigned int REG_A = 7;

const unsigned int BC = 0;
const unsigned int DE = 1;
const unsigned int HL = 2;
const unsigned int AF = 3;
const unsigned int SP = 3;

const unsigned int ZERO_FLAG = 7;
const unsigned int SUB_FLAG = 6;
const unsigned int HALF_CARRY_FLAG = 5;
const unsigned int CARRY_FLAG = 4;

namespace gb::cpu {
    class Registers {
    public:
        Registers();
        explicit Registers(bool cgb);

        void load_byte(unsigned int r, uint8_t val);
        [[nodiscard]] uint8_t read_byte(unsigned int r) const;

        void load_short(unsigned int r, uint16_t val);
        [[nodiscard]] uint16_t read_short(unsigned int r) const;

        [[nodiscard]] bool zero() const;
        [[nodiscard]] bool carry() const;
        [[nodiscard]] bool sub() const;
        [[nodiscard]] bool half_carry() const;

        void set_zero(bool val);
        void set_carry(bool val);
        void set_sub(bool val);
        void set_half_carry(bool val);

        void load(unsigned int dest, unsigned int src);
    private:
        uint8_t a;
        std::bitset<8> flags;
        union {
            struct {
                uint8_t c{};
                uint8_t b{};
            };
            uint16_t bc;
        };
        union {
            struct {
                uint8_t e{};
                uint8_t d{};
            };
            uint16_t de;
        };
        union {
            struct {
                uint8_t l{};
                uint8_t h{};
            };
            uint16_t hl;
        };
    };
}


#endif //OHBOI_REGISTERS_H
