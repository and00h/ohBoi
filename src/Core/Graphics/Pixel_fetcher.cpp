//
// Created by antonio on 22/11/21.
//

#include <Core/Gameboy.h>
#include <Core/Graphics/Ppu.h>

namespace gb::graphics {
    Ppu::Pixel_fetcher::Pixel_fetcher(Ppu &ppu) :
            ppu_{ppu},
            spr_{},
            fetcher_state_{Pixel_fetcher_state::get_tile},
            tile_row_index_{0},
            tile_index_{0},
            tile_row_addr_{0},
            tile_y_{0},
            scroll_pixels_{0},
            sprite_tile_index_{0},
            sprite_tile_y{0},
            rendering_sprites_(false),
            dot_clock_divider_{0},
            state_callbacks_{}
    {
        initialize_state_callbacks();
    }

    void Ppu::Pixel_fetcher::step() {
        state_callbacks_[fetcher_state_]();
    }

    void Ppu::Pixel_fetcher::reset(uint8_t x, uint8_t y, bool r_window) {
        fetcher_state_ = Pixel_fetcher_state::get_tile;
        tile_index_ = 0;
        dot_clock_divider_ = 0;

        uint16_t wTileMap = (ppu_.lcdc_.window_tile_map) ? 0x1C00 : 0x1800;
        uint16_t bgTileMap = (ppu_.lcdc_.bg_tile_map) ? 0x1C00 : 0x1800;

        tile_y_ = y & 7;
        tile_row_index_ = x >> 3;
        scroll_pixels_ = r_window ? 0 : (ppu_.scroll_x_ & 7);
        tile_row_addr_ = (r_window ? wTileMap : bgTileMap) + ((y >> 3) << 5);
        rendering_sprites_ = false;
    }

    void Ppu::Pixel_fetcher::start_sprite_fetch(Sprite &s, uint8_t y) {
        fetcher_state_ = Pixel_fetcher_state::get_tile;
        sprite_tile_index_ = s.tile_location;
        if ( ppu_.lcdc_.obj_size ) {
            if ( y >= s.y - 8 )
                sprite_tile_index_ |= 1;
            else
                sprite_tile_index_ &= 0xFE;
        }
        sprite_tile_y = (y - s.y) & 7;
        dot_clock_divider_ = 0;

        if ( (s.attributes >> 6) & 1 ) {
            sprite_tile_y = 7 - sprite_tile_y;
            if ( ppu_.lcdc_.obj_size ) {
                if ( y >= s.y - 8 )
                    sprite_tile_index_ &= 0xFE;
                else
                    sprite_tile_index_ |= 1;
            }
        }
        rendering_sprites_ = true;
        spr_ = s;
    }

    void Ppu::Pixel_fetcher::initialize_state_callbacks() {
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

    void Ppu::Pixel_fetcher::get_tile() {
        if ( step_dot_divider() ) {
            if ( !rendering_sprites_ ) {
                tile_index_ = static_cast<int>(ppu_.vram_[tile_row_addr_ + tile_row_index_]);
                if (!ppu_.lcdc_.bg_window_tile_data)
                    tile_index_ = ((int8_t) tile_index_) + 256;
                if ( ppu_.gb_.is_cgb_ ) {
                    bg_tile_attributes_.val = ppu_.vram_[0x2000 + tile_row_addr_ + tile_row_index_];
                } else {
                    bg_tile_attributes_.val = 0;
                }
            }
            fetcher_state_ = Pixel_fetcher_state::get_tile_data_low;
        }
    }

    void Ppu::Pixel_fetcher::get_tile_data_lo() {
        if ( step_dot_divider() )
            fetcher_state_ = Pixel_fetcher_state::get_tile_data_high;
    }

    void Ppu::Pixel_fetcher::get_tile_data_hi() {
        if ( step_dot_divider() ) {
            if ( !rendering_sprites_ )
                fetcher_state_ = ppu_.bg_fifo_.size() <= 8 ? Pixel_fetcher_state::push : Pixel_fetcher_state::sleep;
            else
                fetcher_state_ = ppu_.spr_fifo_.size() <= 8 ? Pixel_fetcher_state::push : Pixel_fetcher_state::sleep;
        }
    }

    void Ppu::Pixel_fetcher::sleep() {
        if ( step_dot_divider() )
            fetcher_state_ = Pixel_fetcher_state::push;
    }

    void Ppu::Pixel_fetcher::push() {
        if ( !rendering_sprites_ ) {
            if (ppu_.bg_fifo_.size() <= 8) {
                int tile_x = 7;
                while (scroll_pixels_ > 0 ) {
                    tile_x--;
                    scroll_pixels_--;
                }
                while (tile_x >= 0) {
                    if ( !ppu_.gb_.is_cgb_ )
                        ppu_.bg_fifo_.push(ppu_.tileset_[tile_index_].get_color(tile_x, tile_y_));
                    else {
                        auto& tileset = bg_tile_attributes_.vram_bank == 0 ? ppu_.tileset_ : ppu_.tileset_bank1_;
                        uint8_t _x = bg_tile_attributes_.x_flip ? (7 - tile_x) : tile_x;
                        uint8_t _y = bg_tile_attributes_.y_flip ? (7 - tile_y_) : tile_y_;

                        Tile_pixel _p {
                                tileset[tile_index_].get_color(_x,_y),
                                bg_tile_attributes_.priority,
                                bg_tile_attributes_.pal_number
                        };
                        ppu_.bg_fifo_.push(_p);
                    }
                    tile_x--;
                }
                tile_row_index_ = (tile_row_index_ + 1) & 0x1F;
            }
        } else {
            if (ppu_.spr_fifo_.size() <= 8 ) {
                for (uint8_t tile_x = 0; tile_x <= 7; tile_x++) {
                    auto _color = ppu_.tileset_[sprite_tile_index_].get_color(((spr_.attributes) & 0x20) ? tile_x : (7 - tile_x),
                                                                              sprite_tile_y);
                    Sprite_pixel p {
                            _color,
                            static_cast<uint8_t>((spr_.attributes >> 4) & 1),
                            static_cast<uint8_t>((spr_.attributes >> 7) & 1),
                            spr_.oam_offset
                    };

                    if ( ppu_.gb_.is_cgb_ ) {
                        p = {((spr_.attributes & 8) ? ppu_.tileset_bank1_ : ppu_.tileset_)[sprite_tile_index_].get_color(((spr_.attributes >> 5) & 1) ? tile_x : (7 - tile_x), sprite_tile_y),
                             static_cast<uint8_t>(spr_.attributes & 7),
                             static_cast<uint8_t>((spr_.attributes >> 7) & 1),
                             spr_.oam_offset};
                    }
                    if (ppu_.spr_fifo_.size() <= tile_x ) {
                        ppu_.spr_fifo_.push_back(p);
                    } else {
                        if (ppu_.spr_fifo_.at(tile_x).color_ == 0 || ((ppu_.gb_.is_cgb_) && spr_.oam_offset < ppu_.spr_fifo_.at(tile_x).oam_offset_) )
                            ppu_.spr_fifo_[tile_x] = p;
                    }
                }
                rendering_sprites_ = false;
            }
        }
        fetcher_state_ = Pixel_fetcher_state::get_tile;
    }
}