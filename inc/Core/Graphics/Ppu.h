//
// Created by antonio on 31/07/20.
//

#ifndef OHBOI_PPU_H
#define OHBOI_PPU_H

// lcd Status bits
#define COINCIDENCE_FLAG        2

#include <bitset>
#include <memory>

#include <iostream>
#include <queue>
#include <map>
#include <functional>
#include <stdint-gcc.h>

#include "Core/Memory/Address_space.h"
#include "Core/Cpu/Interrupts.h"
#include "CGBPalette.h"
#include "Tile.h"
#include "util.h"
#include "Hdma_controller.h"

using std::bitset;

namespace gb {
    class Gameboy;
}

namespace gb::graphics {
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
    };

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

    class Ppu {
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
    public:
        Ppu(Gameboy &pGB, std::shared_ptr<cpu::Interrupts> interrupts);

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

        enum Ppu_state : uint8_t {
            hblank, vblank, oam_search, pixel_transfer
        } state_;

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
        [[nodiscard]] Ppu_state get_state() const { return state_; }
    private:
        Gameboy& gb_;
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
            uint8_t val;

            gb::util::Bit_field<uint8_t,0> bg_window_enable_priority;
            gb::util::Bit_field<uint8_t,1> obj_enable;
            gb::util::Bit_field<uint8_t,2> obj_size;
            gb::util::Bit_field<uint8_t,3> bg_tile_map;
            gb::util::Bit_field<uint8_t,4> bg_window_tile_data;
            gb::util::Bit_field<uint8_t,5> window_enable;
            gb::util::Bit_field<uint8_t,6> window_tile_map;
            gb::util::Bit_field<uint8_t,7> lcd_enable;
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

        Hdma_controller hdma_ctrl_;

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

        void disable_lcd();
    };
}

#endif //OHBOI_PPU_H
