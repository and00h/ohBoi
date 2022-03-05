//
// Created by antonio on 31/07/20.
//

#include <Core/Graphics/Ppu.h>
#include <Core/Gameboy.h>
#include <map>
#include <ranges>
#include <span>

#include "Core/Memory/Address_space.h"
#include "Core/Cpu/Interrupts.h"
#include "Tile.h"

namespace {
    constexpr uint16_t vram_bank_size = 0x2000;
    constexpr uint16_t oam_size = 0xA0;
    constexpr uint32_t mono_palette[] { 0xFFFFFFFF, 0xFFCCCCCC, 0xFF777777, 0xFF000000 };
    constexpr uint8_t lcd_stat_lyc_flag = 4;
    
    enum Lcd_status_int_masks: uint8_t {
        hblank = 0x8,
        vblank = 0x10,
        oam = 0x20
    };

    void update_palette_colors_gb(std::span<uint32_t> colors, uint8_t palette) {
        for ( auto& color : colors ) {
            color = mono_palette[palette & 0x3];
            palette >>= 2;
        }
    }
}
// Public methods
gb::graphics::Ppu::Ppu(gb::Gameboy &pGB, std::shared_ptr<cpu::Interrupts> interrupts)
        : state_(Ppu_state::oam_search), gb_(pGB), interrupts_(std::move(interrupts)), pixel_fetcher_(*this), bg_fifo_{},
          spr_fifo_{}, oam_(oam_size), vram_(vram_bank_size << (pGB.is_cgb_ ? 1 : 0)), hdma_ctrl_{pGB} {
    reset();
    tileset_.reserve(384);
    tileset_bank1_.reserve(384);
}

void gb::graphics::Ppu::reset() {
    lcdc_.val = 0x91;
    lcd_stat_ = 0x81;
    scroll_x_ = 0;
    scroll_y_ = 0;
    ly_ = 0;
    lyc_ = 0;
    bg_pal_ = 0xFC;
    obj0_pal_ = 0xFF;
    obj1_pal_ = 0xFF;
    window_x_ = 0;
    window_y_ = 0;
    scanline_counter_ = 0;
    vram_bank_ = 0;
    enable_bg_ = enable_window_ = true;
    enable_sprites_ = true;
    opri_ = gb_.is_cgb_ ? 0 : 1;
    update_palette_colors_gb(bg_pal_colors_, bg_pal_);
    update_palette_colors_gb(obj0_pal_colors_, obj0_pal_);
    update_palette_colors_gb(obj1_pal_colors_, obj1_pal_);
//    if ( gb_.is_cgb_ ) {
//
//    }
}
uint8_t gb::graphics::Ppu::read(uint16_t addr) {
    switch (addr) {
        case Gpu_reg_location::lcd_control:   return lcdc_.val;
        case Gpu_reg_location::lcd_status:    return lcd_stat_;
        case Gpu_reg_location::scroll_y:      return scroll_y_;
        case Gpu_reg_location::scroll_x:      return scroll_x_;
        case Gpu_reg_location::ly:            return ly_;
        case Gpu_reg_location::lyc:           return lyc_;
        case Gpu_reg_location::bg_palette:    return gb_.is_cgb_ ? 0xFF : bg_pal_;
        case Gpu_reg_location::obj_pal0:      return gb_.is_cgb_ ? 0xFF : obj0_pal_;
        case Gpu_reg_location::obj_pal1:      return gb_.is_cgb_ ? 0xFF : obj1_pal_;
        case Gpu_reg_location::window_y:      return window_y_;
        case Gpu_reg_location::window_x:      return window_x_;
        case Gpu_reg_location::vram_bank_sel: return vram_bank_ | 0xFE;
        case Gpu_reg_location::hdma_src_msb:  return hdma_ctrl_.hdma_src_.msb;
        case Gpu_reg_location::hdma_src_lsb:  return hdma_ctrl_.hdma_src_.lsb;
        case Gpu_reg_location::hdma_dst_msb:  return hdma_ctrl_.hdma_dst_.msb;
        case Gpu_reg_location::hdma_dst_lsb:  return hdma_ctrl_.hdma_dst_.lsb;
        case Gpu_reg_location::hdma_len:      return hdma_ctrl_.get_length();
        case Gpu_reg_location::bcps:          return bcps_.val;
        case Gpu_reg_location::bcpd:          return bcpd_.get_byte(bcps_.index);
        case Gpu_reg_location::ocps:          return ocps_.val;
        case Gpu_reg_location::ocpd:          return ocpd_.get_byte(ocps_.index);
        case Gpu_reg_location::opri:          return opri_;
        default:
            return 0xFF;
    }
}

void gb::graphics::Ppu::send(uint16_t addr, uint8_t val) {
    switch(addr) {
        case Gpu_reg_location::lcd_control:
            lcdc_.val = val;
            if ( !lcdc_.lcd_enable )
                disable_lcd();
            break;
        case Gpu_reg_location::lcd_status:
            lcd_stat_ = 0x80 | val;
            break;
        case Gpu_reg_location::scroll_y:
            scroll_y_ = val;
            break;
        case Gpu_reg_location::scroll_x:
            scroll_x_ = val;
            break;
        case Gpu_reg_location::ly:
            ly_ = 0;
            break;
        case Gpu_reg_location::lyc:
            lyc_ = val;
            break;
        case Gpu_reg_location::bg_palette:
            if ( !gb_.is_cgb_ ) {
                bg_pal_ = val;
                update_palette_colors_gb(bg_pal_colors_, bg_pal_);
            }
            break;
        case Gpu_reg_location::obj_pal0:
            if ( !gb_.is_cgb_ ) {
                obj0_pal_ = val;
                update_palette_colors_gb(obj0_pal_colors_, obj0_pal_);
            }
            break;
        case Gpu_reg_location::obj_pal1:
            if ( !gb_.is_cgb_ ) {
                obj1_pal_ = val;
                update_palette_colors_gb(obj1_pal_colors_, obj1_pal_);
            }
            break;
        case Gpu_reg_location::window_y:
            window_y_ = val;
            break;
        case Gpu_reg_location::window_x:
            window_x_ = val;
            break;
        case Gpu_reg_location::vram_bank_sel:
            vram_bank_ = val & 1;
            break;
        case Gpu_reg_location::hdma_src_msb:
            hdma_ctrl_.hdma_src_.msb = val;
            break;
        case Gpu_reg_location::hdma_src_lsb:
            hdma_ctrl_.hdma_src_.lsb = val;
            break;
        case Gpu_reg_location::hdma_dst_msb:
            hdma_ctrl_.hdma_dst_.msb = val;
            break;
        case Gpu_reg_location::hdma_dst_lsb:
            hdma_ctrl_.hdma_dst_.lsb = val;
            break;
        case Gpu_reg_location::hdma_len:
            hdma_ctrl_.set_length(val);
            break;
        case Gpu_reg_location::bcps:
            bcps_.val = val;
            break;
        case Gpu_reg_location::bcpd:
            bcpd_.set_byte(val, bcps_.index);
            bcpd_.update(bcps_.index >> 3);
            if ( bcps_.auto_increment )
                bcps_.index = (bcps_.index + 1) & 0x3F;
            break;
        case Gpu_reg_location::ocps:
            ocps_.val = val;
            break;
        case Gpu_reg_location::ocpd:
            ocpd_.set_byte(val, ocps_.index);
            ocpd_.update(ocps_.index >> 3);
            if ( ocps_.auto_increment )
                ocps_.index = (ocps_.index + 1) & 0x3F;
            break;
        case Gpu_reg_location::opri:
            opri_ = val;
            break;
        default:
            std::ostringstream s("Write to unknown Ppu register at $");
            s << std::hex << addr;
            s << " value 0x" << std::hex << val;
            std::cout << s.str() << std::endl;
            break;
    }
}

void gb::graphics::Ppu::render_pixel() {
    if ( pixel_fetcher_.is_rendering_sprites() )
        return;

    if ( lcdc_.obj_enable && enable_sprites_ ) {
        auto sprite = std::find_if(sprites_.begin(), sprites_.end(), [this](Sprite a) {
            return current_pixel_ >= a.x - 8 && current_pixel_ < a.x && !a._removed;
        });
        if ( sprite != sprites_.end() && bg_fifo_.size() >= 8 ) {
            pixel_fetcher_.start_sprite_fetch(*sprite, ly_);
            sprite->_removed = true;
            return;
        }
    }

    if ( !rendering_window_ && is_window_visible() && enable_window_ ) {
        rendering_window_ = true;

        uint8_t x_ = current_pixel_ - (window_x_ - 7);
        uint8_t y_ = internal_window_counter_;
        pixel_fetcher_.reset(x_, y_, true);
        while ( !bg_fifo_.empty() )
            bg_fifo_.pop();
        return;
    }

    if ( bg_fifo_.size() >= 8 ) {
        Tile_pixel bg_pixel = (gb_.is_cgb_ || lcdc_.bg_window_enable_priority) && enable_bg_ ? bg_fifo_.front() : 0;
        uint8_t color_ = bg_pixel.color_;
        uint32_t *pal = gb_.is_cgb_ ? bcpd_.get_palette(bg_pixel.palette_) : bg_pal_colors_;
        if ( !spr_fifo_.empty() ) {
            Sprite_pixel spr_pixel = spr_fifo_.front();
            if ( spr_pixel.color_ != 0 ) {
                if ( gb_.is_cgb_ ) {
                    if (!lcdc_.bg_window_enable_priority || (!bg_pixel.priority_ && !spr_pixel.priority_) || bg_pixel.color_ == 0 ) {
                        color_ = spr_pixel.color_;
                        pal = ocpd_.get_palette(spr_pixel.palette_);
                    }
                } else {
                    if (!lcdc_.bg_window_enable_priority || !spr_pixel.priority_ || bg_pixel.color_ == 0 ) {
                        color_ = spr_pixel.color_;
                        pal = spr_pixel.palette_ ? obj1_pal_colors_ : obj0_pal_colors_;
                    }
                }
            }
            spr_fifo_.pop_front();
        }

        screen_[ly_ * 160 + current_pixel_] = pal[color_];
        bg_fifo_.pop();
        current_pixel_++;
        if (current_pixel_ == 160) {
            current_pixel_ = 0;
            if (rendering_window_)
                internal_window_counter_++;
            update_state(Ppu_state::hblank);
        }
    }
}

void gb::graphics::Ppu::step(unsigned int cycles) {
    if ( !lcdc_.lcd_enable ) {
        return;
    }

    while ( cycles-- > 0 ) {
        switch (state_) {
            case Ppu_state::hblank:
                if ( advance_scanline_counter() == 0 ) {
                    if (hdma_ctrl_.is_running())
                        hdma_ctrl_.step();

                    if ( advance_scanline() == 144 ) {
                        update_state(Ppu_state::vblank);
                        interrupts_->request(cpu::Interrupts::v_blank);
                    } else {
                        update_state(Ppu_state::oam_search);
                    }
                }
                break;
            case Ppu_state::vblank:
                if ( advance_scanline_counter() == 0 ) {
                    if ( advance_scanline() == 0 ) {
                        internal_window_counter_ = 0;
                        update_state(Ppu_state::oam_search);
                    }
                }
                break;
            case Ppu_state::pixel_transfer:
                render_pixel();
                pixel_fetcher_.step();
                scanline_counter_++;
                break;
            case Ppu_state::oam_search:
                if (advance_scanline_counter() == 80 ) {
                    sprites_.clear();
                    for (uint8_t i = 0; i < oam_size && sprites_.size() < 10; i += 4 ) {
                        Sprite x{i, oam_[i], oam_[i + 1], oam_[i + 2], oam_[i + 3], false};
                        if ((ly_ + 16 >= x.y) && (ly_ + 16 < (x.y + (lcdc_.obj_size ? 16 : 8))) ) {
                            sprites_.push_back(x);
                        }
                    }
                    std::stable_sort(sprites_.begin(), sprites_.end(), [](Sprite a, Sprite b) {
                        return a.x <= b.x;
                    });

                    uint8_t x_, y_;
                    x_ = scroll_x_;
                    y_ = ly_ + scroll_y_;
                    rendering_window_ = false;
                    pixel_fetcher_.reset(x_, y_, false);
                    while ( !bg_fifo_.empty() ) bg_fifo_.pop();
                    while ( !spr_fifo_.empty() ) spr_fifo_.pop_front();
                    update_state(Ppu_state::pixel_transfer);
                }
                break;
        }
    }
}

uint8_t gb::graphics::Ppu::read_vram(uint16_t addr) {
//    if ( state_ == Ppu_state::pixel_transfer ) {
//        return 0xFF;
//    }
    uint16_t a = 0x2000 * (vram_bank_ & 1) + addr;
    return vram_[a];
}

void gb::graphics::Ppu::write_vram(uint16_t addr, uint8_t val) {
    if ( state_ == Ppu_state::pixel_transfer )
        return;
    uint16_t a = 0x2000 * (vram_bank_ & 1) + addr;
    vram_[a] = val;
    // Update tileset_ if a tile is updated
    if ( addr <= 0x17FF ) {
        if (vram_bank_ == 0 )
            tileset_[addr >> 4].update_byte(addr & 0xF, val);
        else
            tileset_bank1_[addr >> 4].update_byte(addr & 0xF, val);
    }
}

void gb::graphics::Ppu::update_state(Ppu_state new_state) {
    static uint8_t interrupt_masks[] {
            Lcd_status_int_masks::hblank,
            Lcd_status_int_masks::vblank,
            Lcd_status_int_masks::oam,
    };

    state_ = new_state;

    lcd_stat_ &= 0xFC;
    lcd_stat_ |= new_state;
    if (lcd_stat_ & interrupt_masks[new_state] ) {
        interrupts_->request(cpu::Interrupts::lcd);
    }
}

uint8_t gb::graphics::Ppu::advance_scanline() {
    ly_ = (ly_ + 1) % 154;
    if (ly_ == lyc_ ) {
        lcd_stat_ |= lcd_stat_lyc_flag;
        interrupts_->request(cpu::Interrupts::lcd);
    } else {
        if (lcd_stat_ & lcd_stat_lyc_flag ) {
            lcd_stat_ &= ~lcd_stat_lyc_flag;
        }
    }

    return ly_;
}

uint16_t gb::graphics::Ppu::advance_scanline_counter() {
    scanline_counter_ = (scanline_counter_ + 1) % 456;
    return scanline_counter_;
}

void gb::graphics::Ppu::disable_lcd() {
    scanline_counter_ = 0;
    ly_ = 0;
    lcd_stat_ &= 0xFC;
    state_ = Ppu_state::vblank;
    return;
}
