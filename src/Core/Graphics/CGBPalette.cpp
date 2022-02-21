//
// Created by antonio on 17/09/20.
//

#include <Core/Graphics/CGBPalette.h>
#include <Logger/Logger.h>

#include <cstring>
#include <sstream>
#include <SDL2/SDL_types.h>

static uint8_t color_map[0x20] = {
        0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
        0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78,
        0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8,
        0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xFF
};

static Uint32 get_color_rgb(uint8_t c) {
    if ( c > 0x1F ) {
        std::ostringstream s;
        s << "(get_color_rgb) Invalid color_: $" << std::hex << c;
        Logger::error("CGBPalette", s.str());
        return 0xFF;
    }
    return (Uint32) color_map[c];
}
CGBPalette::CGBPalette() {
    memset(val, 0, sizeof(uint8_t) * 64);
    memset(palettes, 0xFF, sizeof(Uint32) * 32);
}

Uint32 CGBPalette::get_color(int palette_number, int color_number) {
    if ( palette_number > 7 || color_number > 3 ) {
        std::ostringstream s;
        s << "(get_color) Invalid palette_/color_: " << palette_number << "/" << color_number;
        Logger::error("CGBPalette", s.str());
        return 0xFFFFFFFF;
    }
    return palettes[palette_number][color_number];
}

uint8_t CGBPalette::get_byte(int index) {
    if ( index > 0x3F ) {
        std::ostringstream s;
        s << "(get_byte) Invalid palette_/color_: " << index;
        Logger::error("CGBPalette", s.str());
        return 0xFF;
    }
    return val[index];
}

void CGBPalette::set_byte(uint8_t val, int index) {
    if ( index > 0x3F ) {
        std::ostringstream s;
        s << "(set_byte) Invalid palette_/color_: " << index;
        Logger::error("CGBPalette", s.str());
        return;
    }
    this->val[index] = val;
}

Uint32 *CGBPalette::get_palette(int palette_number) {
    if ( palette_number > 7 ) {
        std::ostringstream s;
        s << "(get_palette) Invalid palette_: " << palette_number;
        Logger::error("CGBPalette", s.str());
        return palettes[0];
    }
    return palettes[palette_number];
}

void CGBPalette::update(int palette_number) {
    Logger::info("CGBPalette", "Updating palette_");
    if ( palette_number > 7 ) {
        std::ostringstream s;
        s << "(update) Invalid palette_: " << palette_number;
        Logger::error("CGBPalette", s.str());
        return;
    }
    int index = palette_number << 3;
    Uint32 *pal = this->palettes[palette_number];
    union color_u {
        struct __attribute__((packed)) {
            uint16_t red : 5;
            uint16_t green : 5;
            uint16_t blue : 5;
            uint16_t unused : 1;
        };
        struct {
            uint8_t lo;
            uint8_t hi;
        };
        uint16_t val;
    } color;
    for ( uint8_t i = 0; i < 8; i += 2 ) {
        Uint32 res = 0xFF000000;
        color.val = val[index + i] | (val[index + i + 1] << 8);
        res |= get_color_rgb(color.red) << 16;
        res |= get_color_rgb(color.green) << 8;
        res |= get_color_rgb(color.blue);
        pal[i >> 1] = res;
    }
}

