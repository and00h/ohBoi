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

#include "Core/Memory/Address_space.h"
#include "Core/cpu/Interrupts.h"
#include "CGBPalette.h"
#include "Tile.h"

using std::bitset;

struct sprite {
    uint8_t oam_offset;
    uint8_t y;
    uint8_t x;
    uint8_t tile_location;
    uint8_t attributes;
    bool _removed;
};

union cgb_tile_attributes_t {
    struct __attribute__((packed)) {
        uint8_t pal_number : 3;
        uint8_t vram_bank : 1;
        unsigned : 1;
        uint8_t x_flip : 1;
        uint8_t y_flip : 1;
        uint8_t priority : 1;
    };
    uint8_t val;
};

namespace gb {
    class Gameboy;
}

namespace gb::graphics {
    class Ppu {
    public:
        Ppu(Gameboy &pGB, std::shared_ptr<cpu::Interrupts> interrupts);

        void    step(int cycles);
        void    reset();

        uint8_t read(uint16_t addr);
        void    send(uint16_t addr, uint8_t val);

        uint8_t read_vram(uint16_t addr);
        void write_vram(uint16_t addr, uint8_t val);
        uint8_t read_oam(uint16_t addr) { return mOAM[addr]; }
        void    write_oam(uint16_t addr, uint8_t val) { mOAM[addr] = val; }

        void    toggle_bg() { mBGEnable = !mBGEnable; }
        void    toggle_sprites() { mSpriteEnable = !mSpriteEnable; }
        void    toggle_window() { mWindowEnable = !mWindowEnable; }

        uint32_t*   get_screen() { return m_Screen; }
    private:
        std::vector<Tile> tileset;
        std::vector<Tile> tileset_bank1;

        struct Sprite_pixel {
            uint8_t color;
            uint8_t palette;
            uint8_t priority;
        };

        struct Tile_pixel {
            uint8_t color;
            uint8_t priority;
            uint8_t palette;

            Tile_pixel(uint8_t c) {
                color = c;
                priority = 0;
                palette = 0;
            }

            Tile_pixel(uint8_t c, uint8_t p, uint8_t pal) {
                color = c;
                priority = p;
                palette = pal;
            }
        };

        class Pixel_fetcher {
        public:
            explicit Pixel_fetcher(Ppu &ppu);

            void step();
            void reset(uint8_t x, uint8_t y, bool r_window);
            void start_sprite_fetch(sprite &s, uint8_t y);
            [[nodiscard]] bool is_rendering_sprites() const { return rendering_sprites; }
        private:
            Ppu &g;
            sprite spr;
            cgb_tile_attributes_t bg_tile_attributes_;

            enum class Pixel_fetcher_state {
                get_tile, get_tile_data_low, get_tile_data_high, sleep, push
            } fetcher_state_;

            uint16_t tile_row_index_;
            int tile_index_;
            uint16_t tile_row_addr_;
            uint8_t tile_y;
            int scroll_pixels;

            uint8_t sprite_tile_index_;
            uint8_t sprite_tile_y;
            bool rendering_sprites;

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
        void render_pixel();

        Gameboy& m_GB;
        std::shared_ptr<cpu::Interrupts> interrupts_;
        std::vector<sprite> sprites;

        uint32_t m_Screen[160 * 144];
        uint32_t m_BgPaletteColors[4], m_Obj0PaletteColors[4], m_Obj1PaletteColors[4];
        enum class Ppu_state: uint8_t {
            hblank, vblank, oam_search, pixel_transfer
        } state_;

        union {
            struct __attribute__((packed)) {
                uint8_t bg_window_enable_priority : 1;
                uint8_t obj_enable : 1;
                uint8_t obj_size : 1;
                uint8_t bg_tile_map : 1;
                uint8_t bg_window_tile_data : 1;
                uint8_t window_enable : 1;
                uint8_t window_tile_map : 1;
                uint8_t lcd_enable : 1;
            };
            uint8_t val;
        } mLCDC;

        uint8_t mLCDStat;
        uint8_t mScrollX;
        uint8_t mScrollY;
        uint8_t m_LY;
        uint8_t mLYC;
        uint8_t mWindowX;
        uint8_t mWindowY;
        uint8_t mBGPalette;
        uint8_t mObjPal0;
        uint8_t mObjPal1;
        uint8_t mDMASrc;
        uint8_t mVRAMBank;
        bool rendering_window = false;
        uint8_t internal_window_counter = 0;
        std::queue<Tile_pixel> bg_fifo;
        std::deque<Sprite_pixel> spr_fifo;
        Pixel_fetcher pxl_fetcher;

        int mScanlineCounter;
        uint8_t mHDMASourceLo;
        uint8_t mHDMASourceHi;
        uint8_t mHDMADestLo;
        uint8_t mHDMADestHi;
        union {
            struct {
                uint8_t len : 7;
                uint8_t type : 1;
            };
            uint8_t val;
        } mHDMALenMode;
        bool mHDMARunning;

        union {
            struct {
                uint8_t index : 6;
                uint8_t unused : 1;
                uint8_t auto_increment : 1;
            };
            uint8_t val;
        } mBCPS;

        CGBPalette mBCPD;

        union {
            struct {
                uint8_t index : 6;
                uint8_t unused : 1;
                uint8_t auto_increment : 1;
            };
            uint8_t val;
        } mOCPS;

        CGBPalette m_OCPD;

        memory::Address_space mOAM;
        memory::Address_space mVRAM;

        bool mWindowEnable;
        bool mBGEnable;
        bool mSpriteEnable;

        void update_bg_palette();
        void update_obj0_palette();
        void update_obj1_palette();

        void launch_gp_hdma();
        void step_hdma();
        void update_state(Ppu_state new_state);

        int current_pixel_ = 0;
        bool is_window_visible() {
            return (mLCDC.window_enable)
                   && (mWindowY <= m_LY)
                   && mWindowY <= 143
                   && mWindowX <= 166
                   && current_pixel_ >= (mWindowX - 7);
        }
    };
}

#endif //OHBOI_PPU_H
