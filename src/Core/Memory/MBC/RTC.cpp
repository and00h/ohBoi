//
// Created by antonio on 29/07/20.
//

#include <Core/Memory/MBC/RTC.h>
#include <vector>
#include <iterator>
#include <memory>

void RTC::read_saved_time(std::ifstream &in, int size) {
    if ( size < 40 )
        return;
    std::unique_ptr<uint8_t[]> rtc_time(new uint8_t[size]);

    in.read((char *) &rtc_time[0], size);

    secs_ = rtc_time[0];
    mins_ = rtc_time[4];
    hrs_ = rtc_time[8];
    day_lo_ = rtc_time[12];
    day_hi_ = rtc_time[16];

    latch_secs_ = rtc_time[20];
    latch_mins_ = rtc_time[24];
    latch_hrs_ = rtc_time[28];
    latch_day_lo_ = rtc_time[32];
    latch_day_hi_ = rtc_time[36];

    if ( size == 48 )
        for ( int i = 0; i < 8; i++ )
            current_time_ |= (rtc_time[40 + i] << (8 * i));
    else
        current_time_ = time(nullptr);

    update_time();
}

void RTC::write_saved_time(std::ofstream &out, int size) {
    if ( size < 40 )
        return;

    std::vector<uint8_t> rtc(size);
    update_time();

    rtc[0] = secs_;
    rtc[4] = mins_;
    rtc[8] = hrs_;
    rtc[12] = day_lo_;
    rtc[16] = day_hi_;

    rtc[20] = latch_secs_;
    rtc[24] = latch_mins_;
    rtc[28] = latch_hrs_;
    rtc[32] = latch_day_lo_;
    rtc[36] = latch_day_hi_;

    for ( int i = 0; i < 8; i++ )
        rtc[40 + i] = (uint8_t) (current_time_ << (8 * i));
    std::copy(rtc.begin(), rtc.end(), std::ostream_iterator<uint8_t>(out));
}

void RTC::latch_time() {
    update_time();
    latch_secs_ = secs_;
    latch_mins_ = mins_;
    latch_hrs_ = hrs_;
    latch_day_lo_ = day_lo_;
    latch_day_hi_ = day_hi_;
}

void RTC::update_time() {
    time_t newTime = time(nullptr);
    unsigned int difference = 0;

    current_time_ = newTime;
    if (newTime > current_time_ && !(day_hi_ & 0x40) ) {
        difference = static_cast<unsigned int>(newTime - current_time_);
    } else {
        return;
    }

    unsigned int new_sec = secs_ + difference;
    if (new_sec == secs_ )
        return;
    secs_ = newTime % 60;

    unsigned int new_min = mins_ + (new_sec / 60);
    if (new_min == mins_ )
        return;
    mins_ = new_min % 60;

    unsigned int new_hrs = hrs_ + (new_min / 60);
    if (new_hrs == hrs_ )
        return;
    hrs_ = new_hrs % 24;

    unsigned int days = ((day_hi_ & 1) << 8) | day_lo_;
    unsigned int new_days = days + (new_hrs / 24);
    if ( new_days == days )
        return;
    day_lo_ = new_days;
    day_hi_ &= 0xFE;
    day_hi_ |= (new_days >> 8) & 1;

    if ( new_days > 511 )
        day_hi_ |= 0x80;
}