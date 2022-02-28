//
// Created by antonio on 30/07/20.
//

#include "Core/Cpu/Registers.h"

gb::cpu::Registers::Registers() : Registers(false) {}

gb::cpu::Registers::Registers(bool cgb)
        : a(cgb ? 0x11 : 0x01),
          flags(0xB0),
          bc(0x0013),
          de(0x00D8),
          hl(0x014D) {}

uint8_t gb::cpu::Registers::read_byte(unsigned int r) const {
    switch (r) {
        case REG_A:
            return a;
        case REG_B:
            return b;
        case REG_C:
            return c;
        case REG_D:
            return d;
        case REG_E:
            return e;
        case REG_F:
            return flags.to_ulong() & 0xF0;
        case REG_H:
            return h;
        case REG_L:
            return l;
        default:
            return 0xFF;
    }
}

void gb::cpu::Registers::load_byte(unsigned int r, uint8_t val) {
    switch (r) {
        case REG_A:
            a = val;
            break;
        case REG_B:
            b = val;
            break;
        case REG_C:
            c = val;
            break;
        case REG_D:
            d = val;
            break;
        case REG_E:
            e = val;
            break;
        case REG_F:
            flags = val & 0xF0;
            break;
        case REG_H:
            h = val;
            break;
        case REG_L:
            l = val;
            break;
        default:
            break;
    }
}

uint16_t gb::cpu::Registers::read_short(unsigned int r) const {
    switch (r) {
        case AF:
            return ((uint16_t) a << 8) | (flags.to_ulong() & 0xF0);
        case BC:
            return bc;
        case DE:
            return de;
        case HL:
            return hl;
        default:
            return 0xFFFF;
    }
}

void gb::cpu::Registers::load_short(unsigned int r, uint16_t val) {
    switch (r) {
        case AF:
            a = (val & 0xFF00) >> 8;
            flags = (val & 0xF0);
            break;
        case BC:
            bc = val;
            break;
        case DE:
            de = val;
            break;
        case HL:
            hl = val;
            break;
        default:
            break;
    }
}

bool gb::cpu::Registers::zero() const {
    return flags.test(ZERO_FLAG);
}

bool gb::cpu::Registers::sub() const {
    return flags.test(SUB_FLAG);
}

bool gb::cpu::Registers::half_carry() const {
    return flags.test(HALF_CARRY_FLAG);
}

bool gb::cpu::Registers::carry() const {
    return flags.test(CARRY_FLAG);
}

void gb::cpu::Registers::set_zero(bool val) {
    flags.set(ZERO_FLAG, val);
}

void gb::cpu::Registers::set_sub(bool val) {
    flags.set(SUB_FLAG, val);
}

void gb::cpu::Registers::set_half_carry(bool val) {
    flags.set(HALF_CARRY_FLAG, val);
}

void gb::cpu::Registers::set_carry(bool val) {
    flags.set(CARRY_FLAG, val);
}

void gb::cpu::Registers::load(unsigned int dest, unsigned int src) {
    if ( dest > 7 || src > 7 )
        return;
    uint8_t temp = this->read_byte(src);
    this->load_byte(dest, temp);
}
