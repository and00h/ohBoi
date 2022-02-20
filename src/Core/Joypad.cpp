//
// Created by antonio on 29/07/20.
//

#include <Core/Joypad.h>

Joypad::Joypad()
    : m_dir_selected{true},
    m_buttons_selected{true},
    m_keys{KEY_RELEASED},
    m_a{true},
    m_b{true},
    m_up{true},
    m_down{true},
    m_left{true},
    m_right{true},
    m_select{true},
    m_start{true}
{}

uint8_t Joypad::get_keys_reg() const {
    uint8_t state = 0xFF;
    if ( not m_dir_selected ) {
        state &= ~direction_keys;
        if ( not m_up )
            state &= ~up_select;
        if ( not m_down )
            state &= ~down_start;
        if ( not m_left )
            state &= ~left_b;
        if ( not m_right )
            state &= ~right_a;
    }
    if ( not m_buttons_selected ) {
        state &= ~button_keys;
        if ( not m_a )
            state &= ~right_a;
        if ( not m_b )
            state &= ~left_b;
        if ( not m_start )
            state &= ~down_start;
        if ( not m_select )
            state &= ~up_select;
    }
    return state;
}



void Joypad::press(Joypad::key_e key) {
    switch (key) {
        case KEY_A:
            m_a = false;
            break;
        case KEY_B:
            m_b = false;
            break;
        case KEY_UP:
            m_up = false;
            break;
        case KEY_DOWN:
            m_down = false;
            break;
        case KEY_LEFT:
            m_left = false;
            break;
        case KEY_RIGHT:
            m_right = false;
            break;
        case KEY_SELECT:
            m_select = false;
            break;
        case KEY_START:
            m_start = false;
            break;
        default:
            break;
    }
}

void Joypad::release(Joypad::key_e key) {
    switch (key) {
        case KEY_A:
            m_a = true;
            break;
        case KEY_B:
            m_b = true;
            break;
        case KEY_UP:
            m_up = true;
            break;
        case KEY_DOWN:
            m_down = true;
            break;
        case KEY_LEFT:
            m_left = true;
            break;
        case KEY_RIGHT:
            m_right = true;
            break;
        case KEY_SELECT:
            m_select = true;
            break;
        case KEY_START:
            m_start = true;
            break;
        default:
            break;
    }
}

void Joypad::select_key_group(uint8_t val) {
    m_buttons_selected = (val & 0x20) != 0;
    m_dir_selected = (val & 0x10) != 0;
}

bool Joypad::buttons_enabled() const {
    return not m_buttons_selected;
}
bool Joypad::buttons_pressed() const {
    return not (m_a && m_b && m_start && m_select);
}
bool Joypad::direction_enabled() const {
    return not m_dir_selected;
}
bool Joypad::direction_pressed() const {
    return not (m_up && m_down && m_left && m_right);
}
bool Joypad::is_pressed(Joypad::key_e key) const {
    switch (key) {
        case KEY_A:
            return not m_a;
        case KEY_RIGHT:
            return not m_right;
        case KEY_B:
            return not m_b;
        case KEY_LEFT:
            return not m_left;
        case KEY_SELECT:
            return not m_select;
        case KEY_UP:
            return not m_up;
        case KEY_START:
            return not m_start;
        case KEY_DOWN:
            return not m_down;
        default:
            return false;
    }
}
