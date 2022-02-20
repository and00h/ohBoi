//
// Created by antonio on 12/08/21.
//

#ifndef OHBOI_SWEEP_H
#define OHBOI_SWEEP_H

namespace gb::audio::utils {
    class Sweep {
    public:
        Sweep(Programmable_timer& timer) : timer_(timer), sweep_period_(0), freq_shadow_(0), sweep_shift_(0), sweep_time_(0), sweep_enable_(false), sweep_negate_(false) {}
        bool clock() {
            bool res = true;
            if ( --sweep_period_ <= 0 ) {
                sweep_period_ = sweep_time_ == 0 ? 8 : sweep_time_;
                if ( sweep_enable_ && sweep_time_ > 0 ) {
                    auto new_freq = (uint16_t) sweep_calc();
                    if ( new_freq < 2048 && sweep_shift_ > 0 ) {
                        freq_shadow_ = new_freq;
                        timer_.set_freq(freq_shadow_);
                        sweep_calc();
                    } else if ( new_freq >= 2048 ) {
                        res = false;
                    }
                    sweep_calc();
                }
            }
            return res;
        }

        void set_period(uint8_t val) { sweep_period_ = val; }
        void set_shift(uint8_t val) { sweep_shift_ = val; }
        void set_sweep_time(uint8_t val) { sweep_time_ = val; }
        void set_enable(bool val) { sweep_enable_ = val; }
        bool get_enable() { return sweep_enable_; }
        void set_negate(bool val) { sweep_negate_ = val; }
        void set_freq_shadow(uint16_t val) { freq_shadow_ = val; }
        int sweep_calc() {
            int new_freq = freq_shadow_ >> sweep_shift_;
            new_freq = freq_shadow_ + (sweep_negate_ ? -new_freq : new_freq);
            sweep_enable_ = new_freq < 2047;
            return new_freq;
        }
    private:

        Programmable_timer& timer_;

        uint8_t sweep_period_;
        uint16_t freq_shadow_;
        uint8_t sweep_shift_;
        uint8_t sweep_time_;

        bool sweep_enable_;
        bool sweep_negate_;
    };
}

#endif //OHBOI_SWEEP_H
