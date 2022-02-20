//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_RTC_H
#define OHBOI_RTC_H


#include <fstream>

class RTC {
public:
    void read_saved_time(std::ifstream &in, int size);
    void write_saved_time(std::ofstream &out, int size);
    void latch_time();
    void update_time();

    void set_secs(uint8_t secs) { secs_ = secs; };
    void set_mins(uint8_t mins) { mins_ = mins; };
    void set_hrs(uint8_t hrs) { hrs_ = hrs; };
    void set_day_lo(uint8_t day_lo) { day_lo_ = day_lo; };
    void set_day_hi(uint8_t day_hi) { day_hi_ = day_hi; };

    [[nodiscard]] uint8_t get_latch_secs() const { return latch_secs_; };
    [[nodiscard]] uint8_t get_latch_mins() const { return latch_mins_; };
    [[nodiscard]] uint8_t get_latch_hrs() const { return latch_hrs_; };
    [[nodiscard]] uint8_t get_latch_day_lo() const { return latch_day_lo_; };
    [[nodiscard]] uint8_t get_latch_day_hi() const { return latch_day_hi_; };
private:
    uint8_t secs_ = 0;
    uint8_t mins_ = 0;
    uint8_t hrs_ = 0;
    uint8_t day_lo_ = 0;
    uint8_t day_hi_ = 0;
    uint8_t latch_secs_ = 0;
    uint8_t latch_mins_ = 0;
    uint8_t latch_hrs_ = 0;
    uint8_t latch_day_lo_ = 0;
    uint8_t latch_day_hi_ = 0;

    time_t current_time_ = 0;
};

#endif //OHBOI_RTC_H
