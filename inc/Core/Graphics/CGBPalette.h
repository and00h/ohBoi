//
// Created by antonio on 17/09/20.
//

#ifndef OHBOI_CGBPALETTE_H
#define OHBOI_CGBPALETTE_H

#include <stdint.h>

class CGBPalette {
public:
    CGBPalette();
    ~CGBPalette() = default;

    uint32_t get_color(int palette_number, int color_number);
    uint8_t get_byte(int index);
    void set_byte(uint8_t val, int index);
    uint32_t* get_palette(int palette_number);
    void update(int palette_number);
private:
    uint8_t values[64];
    uint32_t palettes[8][4];
};

#endif //OHBOI_CGBPALETTE_H
