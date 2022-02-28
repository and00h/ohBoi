//
// Created by antonio on 31/07/20.
//

#ifndef OHBOI_INTERRUPTS_H
#define OHBOI_INTERRUPTS_H

#include <cstdint>

namespace gb::cpu {
    class Interrupts {
    public:
        enum { v_blank = 0, lcd, timer, serial, jpad };

        void set_ime(bool val) { ime_flag = val; };
        [[nodiscard]] bool ime() const { return ime_flag; };

        [[nodiscard]] uint8_t if_flag() const { return int_req; };
        [[nodiscard]] uint8_t ie_flag() const { return int_enable; };

        void set_if(uint8_t val) { int_req = val | 0xE0; };
        void set_ie(uint8_t val) { int_enable = val; };

        void request(int i) { int_req |= (1 << i); };
        void unrequest(int i) { int_req &= ~(1 << i); };

        [[nodiscard]] bool is_enabled(int i) const { return int_enable & (1 << i); }
        [[nodiscard]] bool is_requested(int i) const { return int_req & (1 << i); }

        // Checks if there have been interrupt requests and their corresponding bit in the interrupt enable flag is set
        [[nodiscard]] bool interrupts_pending() const { return (int_enable & int_req & 0x1F); }
        // Checks if there have been interrupt requests without checking if their corresponding bit in the interrupt enable
        // flag is set
        [[nodiscard]] bool interrupts_requested() const { return (int_req & 0x1F); }
    private:
        bool ime_flag = false;
        uint8_t int_req = 0xE1;
        uint8_t int_enable = 0;
    };
}


#endif //OHBOI_INTERRUPTS_H
