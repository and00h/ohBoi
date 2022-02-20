//
// Created by antonio on 11/08/21.
//

#ifndef OHBOI_ENVELOPE_H
#define OHBOI_ENVELOPE_H

namespace gb::audio::utils {
    class Envelope {
    private:
        uint8_t envelope_sweep_;
        uint8_t initial_volume_;
        uint8_t volume_;
        int period_;
        bool direction_;
        bool running_;
    public:
        Envelope() : envelope_sweep_(0), initial_volume_(0), volume_(0), period_(0), direction_(false), running_(false) {};

        void clock() {
            if ( --period_ <= 0 ) {
                period_ = envelope_sweep_;
                if ( running_ && envelope_sweep_ > 0 ) {
                    if ( direction_ && volume < 15 )
                        volume_++;
                    else if ( !direction_ && volume > 0 )
                        volume_--;
                }
                running_ = volume > 0 && volume < 15;
            }
        }

        [[nodiscard]] uint8_t get_volume() const {
            return volume_;
        }

        void set_initial_volume(uint8_t val) {
            initial_volume_ = val;
            volume_ = initial_volume_;
        }

        void set_envelope_sweep(uint8_t val) {
            envelope_sweep_ = val == 0 ? 8 : val;
            period_ = envelope_sweep_;
        }

        void set_direction(bool val) {
            direction_ = val;
        }

        void set_running(bool val) {
            running_ = val;
        }
    };
}

#endif //OHBOI_ENVELOPE_H
