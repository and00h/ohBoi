//
// Created by antonio on 22/11/21.
//

#include <Core/Gameboy.h>
#include <Core/Graphics/Ppu.h>
#include <functional>

gb::graphics::Ppu::Pixel_fetcher::Pixel_fetcher(gb::graphics::Ppu &ppu) :
        g{ppu},
        spr{},
        fetcher_state_{Pixel_fetcher_state::get_tile},
        tile_row_index_{0},
        tile_index_{0},
        tile_row_addr_{0},
        tile_y{0},
        scroll_pixels{0},
        sprite_tile_index_{0},
        sprite_tile_y{0},
        rendering_sprites(false),
        dot_clock_divider_{0},
        state_callbacks_{}
{
    initialize_state_callbacks();
}

void gb::graphics::Ppu::Pixel_fetcher::step() {
    state_callbacks_[fetcher_state_]();
}

void gb::graphics::Ppu::Pixel_fetcher::reset(uint8_t x, uint8_t y, bool r_window) {
    fetcher_state_ = Pixel_fetcher_state::get_tile;
    tile_index_ = 0;
    dot_clock_divider_ = 0;

    uint16_t wTileMap = (g.mLCDC.window_tile_map) ? 0x1C00 : 0x1800;
    uint16_t bgTileMap = (g.mLCDC.bg_tile_map) ? 0x1C00 : 0x1800;

    tile_y = y & 7;
    tile_row_index_ = x >> 3;
    scroll_pixels = r_window ? 0 : (g.mScrollX & 7);
    tile_row_addr_ = (r_window ? wTileMap : bgTileMap) + ((y >> 3) << 5);
    rendering_sprites = false;
}

void gb::graphics::Ppu::Pixel_fetcher::start_sprite_fetch(sprite &s, uint8_t y) {
    fetcher_state_ = Pixel_fetcher_state::get_tile;
    sprite_tile_index_ = s.tile_location;
    if ( g.mLCDC.obj_size ) {
        if ( y >= s.y - 8 )
            sprite_tile_index_ |= 1;
        else
            sprite_tile_index_ &= 0xFE;
    }
    sprite_tile_y = (y - s.y) & 7;
    dot_clock_divider_ = 0;

    if ( (s.attributes >> 6) & 1 ) {
        sprite_tile_y = 7 - sprite_tile_y;
        if ( g.mLCDC.obj_size ) {
            if ( y >= s.y - 8 )
                sprite_tile_index_ &= 0xFE;
            else
                sprite_tile_index_ |= 1;
        }
    }
    rendering_sprites = true;
    spr = s;
}

void gb::graphics::Ppu::Pixel_fetcher::initialize_state_callbacks() {
    state_callbacks_[Pixel_fetcher_state::get_tile]
        = [this]() { get_tile(); };

    state_callbacks_[Pixel_fetcher_state::get_tile_data_low]
        = [this]() { get_tile_data_lo(); };

    state_callbacks_[Pixel_fetcher_state::get_tile_data_high]
        = [this]() { get_tile_data_hi(); };

    state_callbacks_[Pixel_fetcher_state::sleep]
        = [this]() { sleep(); };

    state_callbacks_[Pixel_fetcher_state::push]
        = [this]() { push(); };
}

void gb::graphics::Ppu::Pixel_fetcher::get_tile() {
    if ( step_dot_divider() ) {
        if ( !rendering_sprites ) {
            tile_index_ = (int) g.mVRAM[tile_row_addr_ + tile_row_index_];
            if (!g.mLCDC.bg_window_tile_data)
                tile_index_ = ((int8_t) tile_index_) + 256;
            if ( g.m_GB.is_cgb_ ) {
                bg_tile_attributes_.val = g.mVRAM[0x2000 + tile_row_addr_ + tile_row_index_];
            } else {
                bg_tile_attributes_.val = 0;
            }
        }
        fetcher_state_ = Pixel_fetcher_state::get_tile_data_low;
    }
}

void gb::graphics::Ppu::Pixel_fetcher::get_tile_data_lo() {
    if ( step_dot_divider() )
        fetcher_state_ = Pixel_fetcher_state::get_tile_data_high;
}

void gb::graphics::Ppu::Pixel_fetcher::get_tile_data_hi() {
    if ( step_dot_divider() ) {
        if ( !rendering_sprites )
            fetcher_state_ = g.bg_fifo.size() <= 8 ? Pixel_fetcher_state::push : Pixel_fetcher_state::sleep;
        else
            fetcher_state_ = g.spr_fifo.size() <= 8 ? Pixel_fetcher_state::push : Pixel_fetcher_state::sleep;
    }
}

void gb::graphics::Ppu::Pixel_fetcher::sleep() {
    if ( step_dot_divider() )
        fetcher_state_ = Pixel_fetcher_state::push;
}

void gb::graphics::Ppu::Pixel_fetcher::push() {
    if ( !rendering_sprites ) {
        if (g.bg_fifo.size() <= 8) {
            int tile_x = 7;
            while ( scroll_pixels > 0 ) {
                tile_x--;
                scroll_pixels--;
            }
            while (tile_x >= 0) {
                if ( !g.m_GB.is_cgb_ )
                    g.bg_fifo.push(g.tileset[tile_index_].get_color(tile_x, tile_y));
                else {
                    g.bg_fifo.push({(bg_tile_attributes_.vram_bank == 0 ? g.tileset : g.tileset_bank1)[tile_index_].get_color(
                            bg_tile_attributes_.x_flip ? (7 - tile_x) : tile_x,
                            bg_tile_attributes_.y_flip ? (7 - tile_y) : tile_y), bg_tile_attributes_.priority,
                                    bg_tile_attributes_.pal_number});
                }
                tile_x--;
            }
            tile_row_index_ = (tile_row_index_ + 1) & 0x1F;
        }
    } else {
        if ( g.spr_fifo.size() <= 8 ) {
            for (uint8_t tile_x = 0; tile_x <= 7; tile_x++) {
                Sprite_pixel p = {g.tileset[sprite_tile_index_].get_color(((spr.attributes >> 5) & 1) ? tile_x : (7 - tile_x), sprite_tile_y),
                                  static_cast<uint8_t>((spr.attributes >> 4) & 1),
                                  static_cast<uint8_t>((spr.attributes >> 7) & 1)};
                if ( g.m_GB.is_cgb_ ) {
                    p = {((spr.attributes & 8) ? g.tileset_bank1 : g.tileset)[sprite_tile_index_].get_color(((spr.attributes >> 5) & 1) ? tile_x : (7 - tile_x), sprite_tile_y),
                         static_cast<uint8_t>(spr.attributes & 7),
                         static_cast<uint8_t>((spr.attributes >> 7) & 1)};
                }
                if ( g.spr_fifo.size() <= tile_x ) {
                    g.spr_fifo.push_back(p);
                } else {
                    if ( g.spr_fifo.at(tile_x).color == 0 )
                        g.spr_fifo[tile_x] = p;
                }
            }
            rendering_sprites = false;
        }
    }
    fetcher_state_ = Pixel_fetcher_state::get_tile;
}