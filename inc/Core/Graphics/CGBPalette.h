//
// Created by antonio on 17/09/20.
//

#ifndef OHBOI_CGBPALETTE_H
#define OHBOI_CGBPALETTE_H


#include <SDL2/SDL_types.h>

class CGBPalette {
public:
    CGBPalette();
    ~CGBPalette() = default;

    Uint32 get_color(int palette_number, int color_number);
    uint8_t get_byte(int index);
    void set_byte(uint8_t val, int index);
    Uint32* get_palette(int palette_number);
    void update(int palette_number);
private:
    uint8_t val[64];
    Uint32 palettes[8][4];
};

#endif //OHBOI_CGBPALETTE_H
