//
// Created by antonio on 31/07/20.
//

#ifndef OHBOI_PPU_H
#define OHBOI_PPU_H

// lcd Status bits
#define COINCIDENCE_FLAG        2

#define LCD_CONTROL         0xFF40
#define LCD_STATUS          0xFF41
#define SCROLL_Y            0xFF42
#define SCROLL_X            0xFF43
#define LCDC_Y              0xFF44
#define LY_COMPARE          0xFF45
#define DMA_TRANSFER        0xFF46
#define BG_PALETTE          0xFF47
#define OBJ_PAL0            0xFF48
#define OBJ_PAL1            0xFF49
#define WINDOW_Y            0xFF4A
#define WINDOW_X            0xFF4B
#define VRAM_BANK_SEL       0xFF4F
#define HDMA_SRC_HI         0xFF51
#define HDMA_SRC_LO         0xFF52
#define HDMA_DST_HI         0xFF53
#define HDMA_DST_LO         0xFF54
#define HDMA_LEN            0xFF55
#define BCPS                0xFF68
#define BCPD                0xFF69   // Nice
#define OCPS                0xFF6A
#define OCPD                0xFF6B

#include <bitset>
#include <memory>

#include <iostream>
#include <queue>
#include <map>
#include <functional>
#include <stdint-gcc.h>

#include "Core/Memory/Address_space.h"
#include "Core/cpu/Interrupts.h"
#include "CGBPalette.h"
#include "Tile.h"

using std::bitset;

union cgb_tile_attributes_t {
    struct __attribute__((packed)) {
        uint8_t pal_number: 3;
        uint8_t vram_bank: 1;
        unsigned : 1;
        uint8_t x_flip: 1;
        uint8_t y_flip: 1;
        uint8_t priority: 1;
    };
    uint8_t val;
};

namespace gb {
    class Gameboy;
}

namespace gb::graphics {
    enum Gpu_reg_location: uint16_t {
        lcd_control = 0xFF40,
        lcd_status = 0xFF41,
        scroll_y = 0xFF42,
        scroll_x = 0xFF43,
        ly = 0xFF44,
        lyc = 0xFF45,
        bg_palette = 0xFF47,
        obj_pal0 = 0xFF48,
        obj_pal1 = 0xFF49,
        window_y = 0xFF4A,
        window_x = 0xFF4B,
        vram_bank_sel = 0xFF4F,
        hdma_src_msb = 0xFF51,
        hdma_src_lsb = 0xFF52,
        hdma_dst_msb = 0xFF53,
        hdma_dst_lsb = 0xFF54,
        hdma_len = 0xFF55,
        bcps = 0xFF68,
        bcpd = 0xFF69,
        ocps = 0xFF6A,
        ocpd = 0xFF6B,
        opri = 0xFF6C
    };

    struct Sprite_pixel {
        uint8_t color_;
        uint8_t palette_;
        uint8_t priority_;
        uint8_t oam_offset_;
    };

    struct Tile_pixel {
        uint8_t color_;
        uint8_t priority_;
        uint8_t palette_;

        Tile_pixel(uint8_t c) {
            color_ = c;
            priority_ = 0;
            palette_ = 0;
        }

        Tile_pixel(uint8_t c, uint8_t p, uint8_t pal) {
            color_ = c;
            priority_ = p;
            palette_ = pal;
        }
    };

    struct Sprite {
        uint8_t oam_offset;
        uint8_t y;
        uint8_t x;
        uint8_t tile_location;
        uint8_t attributes;
        bool _removed;

        [[nodiscard]] bool overlap(const Sprite& b) const {
            return (x >= b.x && x < b.x + 8) || (b.x >= x && b.x < x + 8);
        }

        [[nodiscard]] bool overlap(const Sprite& b, bool _8x16, std::vector<Tile>& tileset0, std::vector<Tile>& tileset1) {
            if ( !overlap(b) )
                return false;
            uint8_t sprite_y = y - 16;
            uint8_t sprite_x = x - 8;
            uint8_t limit = _8x16 ? 16 : 8;
            for ( uint8_t _y = sprite_y; y < limit; y++ ) {
                for ( uint8_t _x = sprite_x; x < 8; x++ ) {
                    uint8_t p1 = get_pixel(_x, _y, _8x16, tileset0, tileset1);
                    uint8_t p2 = b.get_pixel(_x, _y, _8x16, tileset0, tileset1);
                    if ( p1 != 0  && p2 != 0 )
                        return true;
                }
            }
            return false;
        }

        bool operator==(const Sprite& b) const {
            return (oam_offset == b.oam_offset && y == b.y && x == b.x && tile_location == b.tile_location && attributes == b.attributes);
        }

        [[nodiscard]] uint8_t get_pixel(uint8_t _x, uint8_t _y, bool _8x16, std::vector<Tile>& tileset0, std::vector<Tile>& tileset1) const {
            uint16_t sprite_tile_index_ = tile_location;
            if ( _8x16 ) {
                if ( _y >= y - 8 )
                    sprite_tile_index_ |= 1;
                else
                    sprite_tile_index_ &= 0xFE;
            }
            uint8_t sprite_tile_y = (_y - y) & 7;

            if ( (attributes >> 6) & 1 ) {
                sprite_tile_y = 7 - sprite_tile_y;
                if ( _8x16 ) {
                    if ( _y >= y - 8 )
                        sprite_tile_index_ &= 0xFE;
                    else
                        sprite_tile_index_ |= 1;
                }
            }
            uint8_t sprite_tile_x = (_x - x) & 7;
            return tileset0[sprite_tile_index_].get_color((attributes & 0x20) ? sprite_tile_x : 7 - sprite_tile_x, sprite_tile_y);
        }
    };

    class Ppu {
    public:
        Ppu(Gameboy &pGB, std::shared_ptr<cpu::Interrupts> interrupts);

        void step(unsigned int cycles);
        void reset();

        uint8_t read(uint16_t addr);
        void send(uint16_t addr, uint8_t val);

        uint8_t read_vram(uint16_t addr);
        void write_vram(uint16_t addr, uint8_t val);

        uint8_t read_oam(uint16_t addr) { return oam_[addr]; }
        void write_oam(uint16_t addr, uint8_t val) { oam_[addr] = val; }

        void toggle_bg() { enable_bg_ = !enable_bg_; }
        void toggle_sprites() { enable_sprites_ = !enable_sprites_; }
        void toggle_window() { enable_window_ = !enable_window_; }

        uint32_t *get_screen() { return screen_; }

    private:
        class Pixel_fetcher {
        public:
            explicit Pixel_fetcher(Ppu &ppu);

            void step();
            void reset(uint8_t x, uint8_t y, bool r_window);
            void start_sprite_fetch(Sprite &s, uint8_t y);
            [[nodiscard]] bool is_rendering_sprites() const { return rendering_sprites_; }
            const Sprite& get_spr() { return spr_; }

        private:
            Ppu &ppu_;
            Sprite spr_;
            cgb_tile_attributes_t bg_tile_attributes_;

            enum class Pixel_fetcher_state {
                get_tile, get_tile_data_low, get_tile_data_high, sleep, push
            } fetcher_state_;

            uint16_t tile_row_index_;
            int tile_index_;
            uint16_t tile_row_addr_;
            uint8_t tile_y_;
            int scroll_pixels_;

            uint8_t sprite_tile_index_;
            uint8_t sprite_tile_y;
            bool rendering_sprites_;

            int dot_clock_divider_;

            std::map<Pixel_fetcher_state, std::function<void(void)>> state_callbacks_;

            bool step_dot_divider() {
                return (dot_clock_divider_++ & 1) == 1;
            }

            void initialize_state_callbacks();

            void get_tile();
            void get_tile_data_lo();
            void get_tile_data_hi();
            void sleep();
            void push();
        };

        enum class Ppu_state : uint8_t {
            hblank, vblank, oam_search, pixel_transfer
        } state_;

        Gameboy &gb_;
        std::shared_ptr<cpu::Interrupts> interrupts_;
        std::vector<Sprite> sprites_;

        std::vector<Tile> tileset_;
        std::vector<Tile> tileset_bank1_;

        Pixel_fetcher pixel_fetcher_;
        std::queue<Tile_pixel> bg_fifo_;
        std::deque<Sprite_pixel> spr_fifo_;

        memory::Address_space oam_;
        memory::Address_space vram_;

        uint32_t screen_[160 * 144]{};
        uint32_t bg_pal_colors_[4]{}, obj0_pal_colors_[4]{}, obj1_pal_colors_[4]{};

        union {
            struct __attribute__((packed)) {
                uint8_t bg_window_enable_priority: 1;
                uint8_t obj_enable: 1;
                uint8_t obj_size: 1;
                uint8_t bg_tile_map: 1;
                uint8_t bg_window_tile_data: 1;
                uint8_t window_enable: 1;
                uint8_t window_tile_map: 1;
                uint8_t lcd_enable: 1;
            };
            uint8_t val;
        } lcdc_{};

        uint8_t lcd_stat_{};
        uint8_t scroll_x_{};
        uint8_t scroll_y_{};
        uint8_t ly_{};
        uint8_t lyc_{};
        uint8_t window_x_{};
        uint8_t window_y_{};
        uint8_t bg_pal_{};
        uint8_t obj0_pal_{};
        uint8_t obj1_pal_{};
        uint8_t vram_bank_{};

        bool rendering_window_ = false;
        uint8_t internal_window_counter_ = 0;
        uint16_t scanline_counter_{};
        int current_pixel_ = 0;

        uint8_t hdma_source_lsb_{};
        uint8_t hdma_source_msb_{};
        uint8_t hdma_dest_lsb_{};
        uint8_t hdma_dest_msb_{};
        union {
            struct {
                uint8_t len: 7;
                uint8_t type: 1;
            };
            uint8_t val;
        } hdma_len_mode_{};
        bool hdma_running_{};

        union {
            struct {
                uint8_t index: 6;
                uint8_t unused: 1;
                uint8_t auto_increment: 1;
            };
            uint8_t val;
        } bcps_{};

        CGBPalette bcpd_;

        union {
            struct {
                uint8_t index: 6;
                uint8_t unused: 1;
                uint8_t auto_increment: 1;
            };
            uint8_t val;
        } ocps_{};

        CGBPalette ocpd_;
        uint8_t opri_;

        bool enable_window_{};
        bool enable_bg_{};
        bool enable_sprites_{};

        void render_pixel();

        void update_bg_palette();

        void update_obj0_palette();

        void update_obj1_palette();

        void launch_gp_hdma();

        void step_hdma();

        void update_state(Ppu_state new_state);

        bool is_window_visible() {
            return (lcdc_.window_enable)
                   && (window_y_ <= ly_)
                   && window_y_ <= 143
                   && window_x_ <= 166
                   && current_pixel_ >= (window_x_ - 7);
        }

        uint8_t advance_scanline();
        uint16_t advance_scanline_counter();
    };
}

#endif //OHBOI_PPU_H
