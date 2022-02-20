//
// Created by antonio on 11/08/21.
//

#ifndef OHBOI_PROGRAMMABLE_TIMER_H
#define OHBOI_PROGRAMMABLE_TIMER_H

namespace gb::audio::utils {
    class Programmable_timer {
    public:
        Programmable_timer() : freq_(0), counter(0) {}

        void set_hi(uint8_t val) {
            freq_ &= 0xFF;
            freq_ |= (val & 0x7) << 8;
        }

        void set_lo(uint8_t val) {
            freq_ &= 0x700;
            freq_ |= val;
        }

        void set_freq(uint16_t val) {
            freq_ = val;
        }
        bool clock() {
            if ( --counter <= 0 ) {
                reload_counter();
                return true;
            }
            return false;
        }
        void reload_counter() {
            counter = (2048 - freq_) << 2;
        }

        uint16_t get_freq() {
            return freq_;
        }
    private:
        uint16_t freq_;
        int counter;
    };
}

#endif //OHBOI_PROGRAMMABLE_TIMER_H
