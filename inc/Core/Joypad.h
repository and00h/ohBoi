//
// Created by antonio on 29/07/20.
//

#ifndef OHBOI_JOYPAD_H
#define OHBOI_JOYPAD_H

#include <cstdint>
#include <bitset>


const uint8_t button_keys = 0x20u;
const uint8_t direction_keys = 0x10u;
const uint8_t down_start = 0x8u;
const uint8_t up_select = 0x4u;
const uint8_t left_b = 0x2u;
const uint8_t right_a = 0x1u;

class Joypad {
public:
    Joypad();

    enum key_e: uint8_t {
        KEY_A = 0, KEY_B, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SELECT, KEY_START
    };

    enum key_state: bool {
        KEY_PRESSED = false, KEY_RELEASED = true
    };

    [[nodiscard]] uint8_t get_keys_reg() const;
    void press(key_e key);
    void release(key_e key);
    void set_key_state(key_e key, key_state state) { m_keys[key] = state; }

    void select_key_group(uint8_t val);

    [[nodiscard]] bool buttons_enabled() const;
    [[nodiscard]] bool buttons_pressed() const;
    [[nodiscard]] bool direction_enabled() const;
    [[nodiscard]] bool direction_pressed() const;

    [[nodiscard]] bool is_pressed(key_e key) const;
private:
    bool m_dir_selected;
    bool m_buttons_selected;

    bool m_keys[8];
    bool m_a, m_b, m_up, m_down, m_left, m_right, m_select, m_start;
};


#endif //OHBOI_JOYPAD_H
