//
// Created by antonio on 15/11/21.
//

#ifndef OHBOI_TILE_H
#define OHBOI_TILE_H

#include <cstdint>
#include <array>
#include <iostream>

class Tile {
public:
    Tile();
    explicit Tile(const uint8_t data[16]);

    [[nodiscard]] uint8_t get_color(int x, int y) const { return tile_colors_[y][x]; }
    void update_byte(int n, uint8_t val);
private:
    std::array<std::array<uint8_t,8>, 8> tile_colors_{};
    std::array<uint8_t, 16> tile_data_{};
};


#endif //OHBOI_TILE_H
