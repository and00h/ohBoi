//
// Created by antonio on 11/08/21.
//

#ifndef OHBOI_LENGTH_COUNTER_H
#define OHBOI_LENGTH_COUNTER_H

namespace gb::audio::utils {
    class Length_counter {
    public:
        Length_counter() : Length_counter(64) {}
        explicit Length_counter(uint16_t counter_max) : enabled_(false), length_enabled_(false), length_counter_(0), counter_max_(counter_max), counter_mask_(counter_max == 64 ? 0x3F : 0xFF) {}

        void clock() {
            if ( length_enabled_ && length_counter_ > 0 ) {
                if ( --length_counter_ == 0 ) {
                    length_enabled_ = false;
                    enabled_ = false;
                }
            }
        }
        bool is_running() const { return length_enabled_; }
        void load_counter(uint16_t val) {
            length_counter_ = counter_max_ - (val & counter_mask_);
            length_enabled_ = true;
        }

        bool is_enabled() { return enabled_; }
        void set_length_enabled(bool val) {
            length_enabled_ = val;
        }
        void set_enabled(bool val) {
            enabled_ = val;
        }

        uint8_t get_counter() const { return length_counter_; }
    private:
        bool enabled_;
        bool length_enabled_;
        uint16_t length_counter_;
        uint16_t counter_max_;
        uint8_t counter_mask_;
    };
}

#endif //OHBOI_LENGTH_COUNTER_H
