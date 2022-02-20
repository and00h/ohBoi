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
        : m_GB(pGB), interrupts_(std::move(interrupts)), mOAM(oam_size), mVRAM(vram_bank_size << (pGB.is_cgb_ ? 1 : 0)),
        bg_fifo{}, spr_fifo{}, pxl_fetcher(*this), state_(Ppu_state::oam_search) {
    reset();
    tileset.reserve(384);
    tileset_bank1.reserve(384);
}

void gb::graphics::Ppu::reset() {
    mLCDC.val = 0x91;
    mLCDStat = 0x81;
    mScrollX = 0;
    mScrollY = 0;
    m_LY = 0;
    mLYC = 0;
    mBGPalette = 0xFC;
    mObjPal0 = 0xFF;
    mObjPal1 = 0xFF;
    mWindowX = 0;
    mWindowY = 0;
    mScanlineCounter = 0;
    mVRAMBank = 0;
    mBGEnable = mWindowEnable = mSpriteEnable = true;
    mHDMARunning = false;
    update_bg_palette();
    update_obj0_palette();
    update_obj1_palette();
    if ( m_GB.is_cgb_ ) {

    }
}
uint8_t gb::graphics::Ppu::read(uint16_t addr) {
    switch (addr) {
        case LCD_CONTROL:   return mLCDC.val;
        case LCD_STATUS:    return mLCDStat;
        case SCROLL_Y:      return mScrollY;
        case SCROLL_X:      return mScrollX;
        case LCDC_Y:        return m_LY;
        case LY_COMPARE:    return mLYC;
        case DMA_TRANSFER:  return mDMASrc;
        case BG_PALETTE:    return m_GB.is_cgb_ ? 0xFF : mBGPalette;
        case OBJ_PAL0:      return m_GB.is_cgb_ ? 0xFF : mObjPal0;
        case OBJ_PAL1:      return m_GB.is_cgb_ ? 0xFF : mObjPal1;
        case WINDOW_Y:      return mWindowY;
        case WINDOW_X:      return mWindowX;
    }
    if ( m_GB.is_cgb_ ) {
        switch (addr) {
            case VRAM_BANK_SEL:
                return mVRAMBank | 0xFE;
            case HDMA_SRC_HI:
                return mHDMASourceHi;
            case HDMA_SRC_LO:
                return mHDMASourceLo;
            case HDMA_DST_HI:
                return mHDMADestHi;
            case HDMA_DST_LO:
                return mHDMADestLo;
            case HDMA_LEN:
                return mHDMALenMode.val;
            case BCPS:
                return mBCPS.val;
            case BCPD:
                return mBCPD.get_byte(mBCPS.index);
            case OCPS:
                return mOCPS.val;
            case OCPD:
                return m_OCPD.get_byte(mOCPS.index);
        }
    }
    return 0xFF;
}

void gb::graphics::Ppu::send(uint16_t addr, uint8_t val) {
    switch(addr) {
        case LCD_CONTROL:
            mLCDC.val = val;
            break;
        case LCD_STATUS:
            mLCDStat = 0x80 | val;
            break;
        case SCROLL_Y:
            mScrollY = val;
            break;
        case SCROLL_X:
            mScrollX = val;
            break;
        case LCDC_Y:
            m_LY = 0;
            break;
        case LY_COMPARE:
            mLYC = val;
            break;
        case DMA_TRANSFER:
            mDMASrc = val;
            m_GB.mmu_->trigger_dma(val);
            break;
        case BG_PALETTE:
            if ( !m_GB.is_cgb_ ) {
                mBGPalette = val;
                update_bg_palette();
            }
            break;
        case OBJ_PAL0:
            if ( !m_GB.is_cgb_ ) {
                mObjPal0 = val;
                update_obj0_palette();
            }
            break;
        case OBJ_PAL1:
            if ( !m_GB.is_cgb_ ) {
                mObjPal1 = val;
                update_obj1_palette();
            }
            break;
        case WINDOW_Y:
            mWindowY = val;
            break;
        case WINDOW_X:
            mWindowX = val;
            break;
        case 0xFF4F:
            mVRAMBank = val & 1;
            break;
        case HDMA_SRC_HI:
            mHDMASourceHi = val;
            break;
        case HDMA_SRC_LO:
            mHDMASourceLo = val;
            break;
        case HDMA_DST_HI:
            mHDMADestHi = val;
            break;
        case HDMA_DST_LO:
            mHDMADestLo = val;
            break;
        case HDMA_LEN:
            if ( !mHDMARunning ) {
                mHDMALenMode.val = val;
                if ( mHDMALenMode.type == 0 )
                    launch_gp_hdma();
                else {
                    mHDMARunning = true;
                    mHDMALenMode.type = 0;
                }
            }
            break;
        case BCPS:
            mBCPS.val = val;
            break;
        case BCPD:
            mBCPD.set_byte(val, mBCPS.index);
            mBCPD.update(mBCPS.index >> 3);
            if ( mBCPS.auto_increment )
                mBCPS.index = (mBCPS.index + 1) & 0x3F;
            break;
        case OCPS:
            mOCPS.val = val;
            break;
        case OCPD:
            m_OCPD.set_byte(val, mOCPS.index);
            m_OCPD.update(mOCPS.index >> 3);
            if ( mOCPS.auto_increment )
                mOCPS.index = (mOCPS.index + 1) & 0x3F;
            break;
        default:
            std::ostringstream s("Writing to unknown Ppu register at $");
            s << std::hex << addr;
            s << " value 0x" << std::hex << val;
            break;
    }
}

void gb::graphics::Ppu::render_pixel() {
    if ( pxl_fetcher.is_rendering_sprites() )
        return;
    if ( !rendering_window && is_window_visible() ) {
        uint8_t x_, y_;
        x_ = current_pixel_ - (mWindowX - 7);
        y_ = internal_window_counter;
        pxl_fetcher.reset(x_, y_, true);
        while ( !bg_fifo.empty() )
            bg_fifo.pop();
        rendering_window = true;
        return;
    }
    if ( mLCDC.obj_enable ) {
        for ( auto& s : sprites ) {
            if ( current_pixel_ >= s.x - 8 ) {
                if ( s._removed )
                    continue;
                if ( bg_fifo.size() >= 8 ) {
                    pxl_fetcher.start_sprite_fetch(s, m_LY);
                    s._removed = true;
                }
                return;
            }
        }
    }
    if ( !bg_fifo.empty() ) {
        Tile_pixel bg_pixel = (m_GB.is_cgb_ || mLCDC.bg_window_enable_priority) ? bg_fifo.front() : 0;
        uint8_t color_ = bg_pixel.color;
        uint32_t *pal = m_GB.is_cgb_ ? mBCPD.get_palette(bg_pixel.palette) : m_BgPaletteColors;
        if ( !spr_fifo.empty() ) {
            Sprite_pixel spr_pixel = spr_fifo.front();
            if ( spr_pixel.color != 0 ) {
                if ( m_GB.is_cgb_ ) {
                    if ( !mLCDC.bg_window_enable_priority || (!bg_pixel.priority && !spr_pixel.priority) || bg_pixel.color == 0 ) {
                        color_ = spr_pixel.color;
                        pal = m_OCPD.get_palette(spr_pixel.palette);
                    }
                } else {
                    if ( !mLCDC.bg_window_enable_priority || !spr_pixel.priority || bg_pixel.color == 0 ) {
                        color_ = spr_pixel.color;
                        pal = spr_pixel.palette ? m_Obj1PaletteColors : m_Obj0PaletteColors;
                    }
                }
            }
            spr_fifo.pop_front();
        }

        m_Screen[m_LY * 160 + current_pixel_] = pal[color_];
        bg_fifo.pop();
        current_pixel_++;
        if (current_pixel_ == 160) {
            current_pixel_ = 0;
            if (rendering_window)
                internal_window_counter++;
            update_state(Ppu_state::hblank);
        }
    }
}

void gb::graphics::Ppu::step(int cycles) {
    if ( !mLCDC.lcd_enable ) {
        mScanlineCounter = 0;
        m_LY = 0;
        mLCDStat &= 0xFC;
        state_ = Ppu_state::vblank;
        return;
    }

    while ( cycles-- > 0 ) {
        switch (state_) {
            case Ppu_state::hblank:
                if ( ++mScanlineCounter == 456 ) {
                    if (mHDMARunning)
                        step_hdma();
                    mScanlineCounter = 0;
                    ++m_LY;
                    if ( m_LY == mLYC ) {
                        mLCDStat |= (1 << COINCIDENCE_FLAG);
                        interrupts_->request(cpu::Interrupts::lcd);
                    } else if ( m_LY != mLYC ) {
                        if ( mLCDStat & (1 << COINCIDENCE_FLAG) ) {
                            mLCDStat &= ~(1 << COINCIDENCE_FLAG);
                        }
                    }
                    if ( m_LY == 144 ) {
                        update_state(Ppu_state::vblank);
                        interrupts_->request(cpu::Interrupts::v_blank);
                    } else {
                        update_state(Ppu_state::oam_search);
                    }
                }
                break;
            case Ppu_state::vblank:
                if ( ++mScanlineCounter == 456 ) {
                    mScanlineCounter = 0;
                    m_LY++;
                    if ( m_LY == mLYC ) {
                        mLCDStat |= (1 << COINCIDENCE_FLAG);
                        interrupts_->request(cpu::Interrupts::lcd);
                    } else if ( m_LY != mLYC ) {
                        if ( mLCDStat & (1 << COINCIDENCE_FLAG) ) {
                            mLCDStat &= ~(1 << COINCIDENCE_FLAG);
                        }
                    }
                }
                if ( m_LY == 154 ) {
                    m_LY = 0;
                    internal_window_counter = 0;
                    update_state(Ppu_state::oam_search);
                }
                break;
            case Ppu_state::pixel_transfer:
                render_pixel();
                pxl_fetcher.step();
                mScanlineCounter++;
                break;
            case Ppu_state::oam_search:
                if ( ++mScanlineCounter == 80 ) {
                    sprites.clear();
                    for ( int i = 0; i < oam_size && sprites.size() < 10; i += 4 ) {
                        sprite x{static_cast<uint8_t>(i >> 2), mOAM[i], mOAM[i+1], mOAM[i+2], mOAM[i+3], false};
                        if ( (m_LY + 16 >= x.y) && (m_LY + 16 < (x.y + (mLCDC.obj_size ? 16 : 8))) ) {
                            sprites.push_back(x);
                        }
                    }
                    std::sort(sprites.begin(), sprites.end(), [](sprite a, sprite b) {
                        return a.x <= b.x && a.oam_offset < b.oam_offset;
                    });
                    uint8_t x_, y_;
                    x_ = mScrollX;
                    y_ = m_LY + mScrollY;
                    rendering_window = false;
                    pxl_fetcher.reset(x_, y_, false);
                    while ( !bg_fifo.empty() ) bg_fifo.pop();
                    while ( !spr_fifo.empty() ) spr_fifo.pop_front();
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
    uint16_t a = 0x2000 * (mVRAMBank & 1) + addr;
    return mVRAM[a];
}

void gb::graphics::Ppu::write_vram(uint16_t addr, uint8_t val) {
    if ( state_ == Ppu_state::pixel_transfer )
        return;
    uint16_t a = 0x2000 * (mVRAMBank & 1) + addr;
    mVRAM[a] = val;
    // Update tileset if a tile is updated
    if ( addr <= 0x17FF ) {
        if ( mVRAMBank == 0 )
            tileset[addr >> 4].update_byte(addr & 0xF, val);
        else
            tileset_bank1[addr >> 4].update_byte(addr & 0xF, val);
    }
}

void gb::graphics::Ppu::update_bg_palette() {
    for (int i = 0; i < 4; i++)
        m_BgPaletteColors[i] = monoPalette[(mBGPalette >> (i * 2)) & 0x3];
}

void gb::graphics::Ppu::update_obj0_palette() {
    for (int i = 0; i < 4; i++)
        m_Obj0PaletteColors[i] = monoPalette[(mObjPal0 >> (i * 2)) & 0x3];
}

void gb::graphics::Ppu::update_obj1_palette() {
    for (int i = 0; i < 4; i++)
        m_Obj1PaletteColors[i] = monoPalette[(mObjPal1 >> (i * 2)) & 0x3];
}

void gb::graphics::Ppu::launch_gp_hdma() {
    uint16_t src = (mHDMASourceHi << 8) | mHDMASourceLo;
    uint16_t dst = (mHDMADestHi << 8) | mHDMADestLo;
    unsigned int len = (mHDMALenMode.len + 1) << 4;
    for ( unsigned int i = 0; i < len; i++ )
        m_GB.mmu_->write(dst + i, m_GB.mmu_->read(src + i));
    mHDMALenMode.val = 0xFF;
}

void gb::graphics::Ppu::step_hdma() {
    uint16_t index = mHDMALenMode.len << 4;
    uint16_t src = ((mHDMASourceHi << 8) | mHDMASourceLo) + index;
    uint16_t dst = ((mHDMADestHi << 8) | mHDMADestLo) + index;
    for ( int i = 0; i < 0x10; i++ )
        m_GB.mmu_->write(dst + i, m_GB.mmu_->read(src + i));
    if ( --mHDMALenMode.val == 0xFF )
        mHDMARunning = false;
}

void gb::graphics::Ppu::update_state(Ppu_state new_state) {
    static uint8_t interrupt_masks[] {
            Lcd_status_int_masks::hblank,
            Lcd_status_int_masks::vblank,
            Lcd_status_int_masks::oam,
    };

    state_ = new_state;

    mLCDStat &= 0xFC;
    auto state_val = static_cast<std::underlying_type<Ppu_state>::type>(new_state);
    mLCDStat |= state_val;
    if ( mLCDStat & interrupt_masks[state_val] ) {
        interrupts_->request(cpu::Interrupts::lcd);
    }
}

