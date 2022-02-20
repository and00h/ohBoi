//
// Created by antonio on 15/11/21.
//

#include <cstring>
#include "Tile.h"

Tile::Tile(const uint8_t *data) : tile_colors_{} {
    for ( int y = 0; y < 8; y++) {
        int line = y * 2;
        uint8_t data1 = tile_data_[line] = data[line];
        uint8_t data2 = tile_data_[line] = data[line + 1];
        for ( int x = 0; x < 8; x++ ) {
            tile_colors_[y][x] = ((data2 >> x) & 1) << 1 | ((data1 >> x) & 1);
        }
    }
}

void Tile::update_byte(int n, uint8_t val) {
    tile_data_[n] = val;
    int line = n & 0xFE;
    int y = line / 2;
    for ( int x = 0; x < 8; x++ )
        tile_colors_[y][x] = ((tile_data_[line + 1] >> x) & 1) << 1 | ((tile_data_[line] >> x) & 1);
}
