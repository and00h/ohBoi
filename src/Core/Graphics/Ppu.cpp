//
// Created by antonio on 31/07/20.
//

#include <Core/Graphics/Ppu.h>
#include <Core/Gameboy.h>
#include <map>
#include <functional>
#include "Core/Memory/Address_space.h"
#include "Core/cpu/Interrupts.h"
#include "Tile.h"

namespace {
    constexpr uint16_t vram_bank_size = 0x2000;
    constexpr uint16_t oam_size = 0xA0;
    constexpr uint32_t monoPalette[] { 0xFFFFFFFF, 0xFFCCCCCC, 0xFF777777, 0xFF000000 };

    enum Lcd_status_int_masks: uint8_t {
        hblank = 0x8,
        vblank = 0x10,
        oam = 0x20
    };
}
// Public methods
gb::graphics::Ppu::Ppu(gb::Gameboy &pGB, std::shared_ptr<cpu::Interrupts> interrupts)
        : gb_(pGB), interrupts_(std::move(interrupts)), oam_(oam_size), vram_(vram_bank_size << (pGB.is_cgb_ ? 1 : 0)),
          bg_fifo_{}, spr_fifo_{}, pixel_fetcher_(*this), state_(Ppu_state::oam_search) {
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
    enable_bg_ = enable_window_ = enable_sprites_ = true;
    hdma_running_ = false;
    update_bg_palette();
    update_obj0_palette();
    update_obj1_palette();
    if ( gb_.is_cgb_ ) {

    }
}
uint8_t gb::graphics::Ppu::read(uint16_t addr) {
    switch (addr) {
        case LCD_CONTROL:   return lcdc_.val;
        case LCD_STATUS:    return lcd_stat_;
        case SCROLL_Y:      return scroll_y_;
        case SCROLL_X:      return scroll_x_;
        case LCDC_Y:        return ly_;
        case LY_COMPARE:    return lyc_;
        case BG_PALETTE:    return gb_.is_cgb_ ? 0xFF : bg_pal_;
        case OBJ_PAL0:      return gb_.is_cgb_ ? 0xFF : obj0_pal_;
        case OBJ_PAL1:      return gb_.is_cgb_ ? 0xFF : obj1_pal_;
        case WINDOW_Y:      return window_y_;
        case WINDOW_X:      return window_x_;
    }
    if ( gb_.is_cgb_ ) {
        switch (addr) {
            case VRAM_BANK_SEL:
                return vram_bank_ | 0xFE;
            case HDMA_SRC_HI:
                return hdma_source_msb_;
            case HDMA_SRC_LO:
                return hdma_source_lsb_;
            case HDMA_DST_HI:
                return hdma_dest_msb_;
            case HDMA_DST_LO:
                return hdma_dest_lsb_;
            case HDMA_LEN:
                return hdma_len_mode_.val;
            case BCPS:
                return bcps_.val;
            case BCPD:
                return bcpd_.get_byte(bcps_.index);
            case OCPS:
                return ocps_.val;
            case OCPD:
                return ocpd_.get_byte(ocps_.index);
        }
    }
    return 0xFF;
}

void gb::graphics::Ppu::send(uint16_t addr, uint8_t val) {
    switch(addr) {
        case LCD_CONTROL:
            lcdc_.val = val;
            break;
        case LCD_STATUS:
            lcd_stat_ = 0x80 | val;
            break;
        case SCROLL_Y:
            scroll_y_ = val;
            break;
        case SCROLL_X:
            scroll_x_ = val;
            break;
        case LCDC_Y:
            ly_ = 0;
            break;
        case LY_COMPARE:
            lyc_ = val;
            break;
        case BG_PALETTE:
            if ( !gb_.is_cgb_ ) {
                bg_pal_ = val;
                update_bg_palette();
            }
            break;
        case OBJ_PAL0:
            if ( !gb_.is_cgb_ ) {
                obj0_pal_ = val;
                update_obj0_palette();
            }
            break;
        case OBJ_PAL1:
            if ( !gb_.is_cgb_ ) {
                obj1_pal_ = val;
                update_obj1_palette();
            }
            break;
        case WINDOW_Y:
            window_y_ = val;
            break;
        case WINDOW_X:
            window_x_ = val;
            break;
        case 0xFF4F:
            vram_bank_ = val & 1;
            break;
        case HDMA_SRC_HI:
            hdma_source_msb_ = val;
            break;
        case HDMA_SRC_LO:
            hdma_source_lsb_ = val;
            break;
        case HDMA_DST_HI:
            hdma_dest_msb_ = val;
            break;
        case HDMA_DST_LO:
            hdma_dest_lsb_ = val;
            break;
        case HDMA_LEN:
            if ( !hdma_running_ ) {
                hdma_len_mode_.val = val;
                if (hdma_len_mode_.type == 0 )
                    launch_gp_hdma();
                else {
                    hdma_running_ = true;
                    hdma_len_mode_.type = 0;
                }
            }
            break;
        case BCPS:
            bcps_.val = val;
            break;
        case BCPD:
            bcpd_.set_byte(val, bcps_.index);
            bcpd_.update(bcps_.index >> 3);
            if ( bcps_.auto_increment )
                bcps_.index = (bcps_.index + 1) & 0x3F;
            break;
        case OCPS:
            ocps_.val = val;
            break;
        case OCPD:
            ocpd_.set_byte(val, ocps_.index);
            ocpd_.update(ocps_.index >> 3);
            if ( ocps_.auto_increment )
                ocps_.index = (ocps_.index + 1) & 0x3F;
            break;
        default:
            std::ostringstream s("Writing to unknown Ppu register at $");
            s << std::hex << addr;
            s << " value 0x" << std::hex << val;
            break;
    }
}

void gb::graphics::Ppu::render_pixel() {
    if ( pixel_fetcher_.is_rendering_sprites() )
        return;

    if ( !rendering_window_ && is_window_visible() ) {
        rendering_window_ = true;

        uint8_t x_ = current_pixel_ - (window_x_ - 7);
        uint8_t y_ = internal_window_counter_;
        pixel_fetcher_.reset(x_, y_, true);
        while ( !bg_fifo_.empty() )
            bg_fifo_.pop();
        return;
    }

    if ( lcdc_.obj_enable ) {
        for ( auto& s : sprites_ ) {
            if ( current_pixel_ >= s.x - 8 ) {
                if ( s._removed )
                    continue;
                if (bg_fifo_.size() >= 8 ) {
                    pixel_fetcher_.start_sprite_fetch(s, ly_);
                    s._removed = true;
                }
                return;
            }
        }
    }

    if ( !bg_fifo_.empty() ) {
        Tile_pixel bg_pixel = (gb_.is_cgb_ || lcdc_.bg_window_enable_priority) ? bg_fifo_.front() : 0;
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
        scanline_counter_ = 0;
        ly_ = 0;
        lcd_stat_ &= 0xFC;
        state_ = Ppu_state::vblank;
        return;
    }

    while ( cycles-- > 0 ) {
        switch (state_) {
            case Ppu_state::hblank:
                if (++scanline_counter_ == 456 ) {
                    if (hdma_running_)
                        step_hdma();
                    scanline_counter_ = 0;
                    ++ly_;
                    if (ly_ == lyc_ ) {
                        lcd_stat_ |= (1 << COINCIDENCE_FLAG);
                        interrupts_->request(cpu::Interrupts::lcd);
                    } else if (ly_ != lyc_ ) {
                        if (lcd_stat_ & (1 << COINCIDENCE_FLAG) ) {
                            lcd_stat_ &= ~(1 << COINCIDENCE_FLAG);
                        }
                    }
                    if (ly_ == 144 ) {
                        update_state(Ppu_state::vblank);
                        interrupts_->request(cpu::Interrupts::v_blank);
                    } else {
                        update_state(Ppu_state::oam_search);
                    }
                }
                break;
            case Ppu_state::vblank:
                if (++scanline_counter_ == 456 ) {
                    scanline_counter_ = 0;
                    ly_++;
                    if (ly_ == lyc_ ) {
                        lcd_stat_ |= (1 << COINCIDENCE_FLAG);
                        interrupts_->request(cpu::Interrupts::lcd);
                    } else if (ly_ != lyc_ ) {
                        if (lcd_stat_ & (1 << COINCIDENCE_FLAG) ) {
                            lcd_stat_ &= ~(1 << COINCIDENCE_FLAG);
                        }
                    }
                }
                if (ly_ == 154 ) {
                    ly_ = 0;
                    internal_window_counter_ = 0;
                    update_state(Ppu_state::oam_search);
                }
                break;
            case Ppu_state::pixel_transfer:
                render_pixel();
                pixel_fetcher_.step();
                scanline_counter_++;
                break;
            case Ppu_state::oam_search:
                if (++scanline_counter_ == 80 ) {
                    sprites_.clear();
                    for (int i = 0; i < oam_size && sprites_.size() < 10; i += 4 ) {
                        Sprite x{static_cast<uint8_t>(i >> 2), oam_[i], oam_[i + 1], oam_[i + 2], oam_[i + 3], false};
                        if ((ly_ + 16 >= x.y) && (ly_ + 16 < (x.y + (lcdc_.obj_size ? 16 : 8))) ) {
                            sprites_.push_back(x);
                        }
                    }
                    std::sort(sprites_.begin(), sprites_.end(), [](Sprite a, Sprite b) {
                        return a.x <= b.x && a.oam_offset < b.oam_offset;
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

void gb::graphics::Ppu::update_bg_palette() {
    for (int i = 0; i < 4; i++)
        bg_pal_colors_[i] = monoPalette[(bg_pal_ >> (i * 2)) & 0x3];
}

void gb::graphics::Ppu::update_obj0_palette() {
    for (int i = 0; i < 4; i++)
        obj0_pal_colors_[i] = monoPalette[(obj0_pal_ >> (i * 2)) & 0x3];
}

void gb::graphics::Ppu::update_obj1_palette() {
    for (int i = 0; i < 4; i++)
        obj1_pal_colors_[i] = monoPalette[(obj1_pal_ >> (i * 2)) & 0x3];
}

void gb::graphics::Ppu::launch_gp_hdma() {
    uint16_t src = (hdma_source_msb_ << 8) | hdma_source_lsb_;
    uint16_t dst = (hdma_dest_msb_ << 8) | hdma_dest_lsb_;
    unsigned int len = (hdma_len_mode_.len + 1) << 4;
    for ( unsigned int i = 0; i < len; i++ )
        gb_.mmu_->write(dst + i, gb_.mmu_->read(src + i));
    hdma_len_mode_.val = 0xFF;
}

void gb::graphics::Ppu::step_hdma() {
    uint16_t index = hdma_len_mode_.len << 4;
    uint16_t src = ((hdma_source_msb_ << 8) | hdma_source_lsb_) + index;
    uint16_t dst = ((hdma_dest_msb_ << 8) | hdma_dest_lsb_) + index;
    for ( int i = 0; i < 0x10; i++ )
        gb_.mmu_->write(dst + i, gb_.mmu_->read(src + i));
    if (--hdma_len_mode_.val == 0xFF )
        hdma_running_ = false;
}

void gb::graphics::Ppu::update_state(Ppu_state new_state) {
    static uint8_t interrupt_masks[] {
            Lcd_status_int_masks::hblank,
            Lcd_status_int_masks::vblank,
            Lcd_status_int_masks::oam,
    };

    state_ = new_state;

    lcd_stat_ &= 0xFC;
    auto state_val = static_cast<std::underlying_type<Ppu_state>::type>(new_state);
    lcd_stat_ |= state_val;
    if (lcd_stat_ & interrupt_masks[state_val] ) {
        interrupts_->request(cpu::Interrupts::lcd);
    }
}

