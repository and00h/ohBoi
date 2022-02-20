//
// Created by antonio on 30/07/20.
//

#include "Core/cpu/Cpu.h"
#include "Core/cpu/Registers.h"
#include "Core/Gameboy.h"
#include "Core/cpu/Interrupts.h"

using gb::cpu::Cpu;

namespace {
    inline uint8_t set(int bit, uint8_t val) {
        bit = 1 << bit;
        val |= bit;
        return val;
    }

    inline uint8_t res(int bit, uint8_t val) {
        bit = 1 << bit;
        val &= ~bit;
        return val;
    }
}

union gb::cpu::Instr_argument {
    struct {
        uint8_t lsb;
        uint8_t msb;
    };
    uint16_t word;
};

Cpu::Cpu(Gameboy &gb, std::shared_ptr<Interrupts> interrupts, std::shared_ptr<Joypad> joypad)
        : gb_(gb),
          regs_(gb.is_cgb_),
          interrupts_(std::move(interrupts)),
          joypad_(std::move(joypad)),
          pc_(0x100),
          sp_(0xFFFE),
          tima_(0),
          tma_(0),
          tac_(0),
          timer_counter_(1024),
          div_reg_(0),
          div_counter_(0),
          booting_(false),
          debug_(false),
          ei_last_instruction_(false),
          halted_(false),
          halt_bug_triggered_(false),
          timer_overflow_(false),
          double_speed_(false),
          cycles_(0),
          alu_functions_{
                  &Cpu::add, &Cpu::adc, &Cpu::sub, &Cpu::sbc,
                  &Cpu::and_l, &Cpu::xor_l, &Cpu::or_l, &Cpu::cp
          },
          rotate_instructions_{
                  &Cpu::rlc, &Cpu::rrc, &Cpu::rl, &Cpu::rr,
                  &Cpu::sla, &Cpu::sra, &Cpu::swap, &Cpu::srl
          }
{
    debug_ = false;
    //reset();
}

void Cpu::reset() {
    regs_.load_short(AF, gb_.is_cgb_ ? 0x11B0 : 0x01B0);
    regs_.load_short(BC, 0x0013);
    regs_.load_short(DE, 0x00D8);
    regs_.load_short(HL, 0x014D);

    sp_ = 0xFFFE;
    pc_ = 0x0100;

    ei_last_instruction_ = false;
    tima_ = 0;
    tma_ = 0;
    tac_ = 0;
    halted_ = false;
    halt_bug_triggered_ = false;

    interrupts_->set_ime(false);
    interrupts_->set_if(0xE1);
    interrupts_->set_ie(0);

    div_counter_ = 0;
    timer_counter_ = 1024;
    timer_overflow_ = false;
    double_speed_ = false;
}

void Cpu::step() {
    if ( ei_last_instruction_ ) {
        ei_last_instruction_ = false;
        interrupts_->set_ime(true);
    }
    if ( timer_overflow_ ) {
        timer_overflow_ = false;
        tima_ = tma_;
        interrupts_->request(Interrupts::timer);
    }
    cycles_ += halted_ ? 4 : fetch();
    if ( halted_ )
        gb_.clock(4);
    update_buttons();
    if (!interrupts_->ime() && interrupts_->interrupts_pending() && halted_ ) {
        halted_ = false;
    } else
        service_interrupts();
}

unsigned int Cpu::fetch() {
    uint8_t opcode = read_memory(pc_);
    if ( halt_bug_triggered_ )
        halt_bug_triggered_ = false;
    else
        pc_++;

    Instr_argument arg{.word = 0};

    if ( opcodes[opcode].operands >= 1 )
        arg.lsb = read_memory(pc_++);
    if ( opcodes[opcode].operands == 2 )
        arg.msb = read_memory(pc_++);
    if ( debug_ ) {
        switch ( opcodes[opcode].operands ) {
            case 0:
                printf("%s", instr[opcode].c_str());
                break;
            case 1:
                printf(instr[opcode].c_str(), arg.lsb);
                break;
            case 2:
                printf(instr[opcode].c_str(), arg.word);
                break;
        }
        printf("\n");
    }

    decode_n_xecute(opcode, arg);
    return opcode != 0xCB ? opcodes[opcode].cycles : cb_instructions_cycles[arg.lsb];
}

void Cpu::decode_n_xecute(uint8_t opcode, Instr_argument arg) {
    uint8_t temp_b;
    uint16_t temp_w;
    switch ( opcode ) {
        case 0x00:
            break;
        case 0x01:
            regs_.load_short(BC, arg.word);
            break;
        case 0x02:
            write_memory(regs_.read_short(BC), regs_.read_byte(REG_A));
            break;
        case 0x03:
            inc16(BC);
            break;
        case 0x04:
            inc8(REG_B);
            break;
        case 0x05:
            dec8(REG_B);
            break;
        case 0x06:
            regs_.load_byte(REG_B, arg.lsb);
            break;
        case 0x07:
            rlca();
            break;
        case 0x08:
            write_memory_short(arg.word, sp_);
            break;
        case 0x09:
            add_hl(regs_.read_short(BC));
            break;
        case 0x0A:
            regs_.load_byte(REG_A, read_memory(regs_.read_short(BC)));
            break;
        case 0x0B:
            dec16(BC);
            break;
        case 0x0C:
            inc8(REG_C);
            break;
        case 0x0D:
            dec8(REG_C);
            break;
        case 0x0E:
            regs_.load_byte(REG_C, arg.lsb);
            break;
        case 0x0F:
            rrca();
            break;
        case 0x10:
            stop();
            break;
        case 0x11:
            regs_.load_short(DE, arg.word);
            break;
        case 0x12:
            write_memory(regs_.read_short(DE), regs_.read_byte(REG_A));
            break;
        case 0x13:
            inc16(DE);
            break;
        case 0x14:
            inc8(REG_D);
            break;
        case 0x15:
            dec8(REG_D);
            break;
        case 0x16:
            regs_.load_byte(REG_D, arg.lsb);
            break;
        case 0x17:
            rla();
            break;
        case 0x18:
            jr((int8_t) arg.lsb);
            break;
        case 0x19:
            add_hl(regs_.read_short(DE));
            break;
        case 0x1A:
            regs_.load_byte(REG_A, read_memory(regs_.read_short(DE)));
            break;
        case 0x1B:
            dec16(DE);
            break;
        case 0x1C:
            inc8(REG_E);
            break;
        case 0x1D:
            dec8(REG_E);
            break;
        case 0x1E:
            regs_.load_byte(REG_E, arg.lsb);
            break;
        case 0x1F:
            rra();
            break;
        case 0x20:
            if ( !( regs_.zero() ) )
                jr((int8_t) arg.lsb);
            break;
        case 0x21:
            regs_.load_short(HL, arg.word);
            break;
        case 0x22:
            temp_w = regs_.read_short(HL);
            write_memory(temp_w, regs_.read_byte(REG_A));
            regs_.load_short(HL, temp_w + 1);
            break;
        case 0x23:
            inc16(HL);
            break;
        case 0x24:
            inc8(REG_H);
            break;
        case 0x25:
            dec8(REG_H);
            break;
        case 0x26:
            regs_.load_byte(REG_H, arg.lsb);
            break;
        case 0x27:
            daa();
            break;
        case 0x28:
            if ( regs_.zero() )
                jr((int8_t) arg.lsb);
            break;
        case 0x29:
            add_hl(regs_.read_short(HL));
            break;
        case 0x2A:
            temp_w = regs_.read_short(HL);
            regs_.load_byte(REG_A, read_memory(temp_w));
            regs_.load_short(HL, temp_w + 1);
            break;
        case 0x2B:
            dec16(HL);
            break;
        case 0x2C:
            inc8(REG_L);
            break;
        case 0x2D:
            dec8(REG_L);
            break;
        case 0x2E:
            regs_.load_byte(REG_L, arg.lsb);
            break;
        case 0x2F:
            cpl();
            break;
        case 0x30:
            if ( !regs_.carry() )
                jr((int8_t) arg.lsb);
            break;
        case 0x31:
            sp_ = arg.word;
            break;
        case 0x32:
            temp_w = regs_.read_short(HL);
            write_memory(temp_w, regs_.read_byte(REG_A));
            regs_.load_short(HL, temp_w - 1);
            break;
        case 0x33:
            inc16(SP);
            break;
        case 0x34:
            temp_b = read_memory(regs_.read_short(HL));
            regs_.set_half_carry((temp_b & 0xF) == 0xF);
            ++temp_b;
            regs_.set_zero(temp_b == 0);
            regs_.set_sub(false);
            write_memory(regs_.read_short(HL), temp_b);
            break;
        case 0x35:
            temp_b = read_memory(regs_.read_short(HL));
            regs_.set_half_carry(!(temp_b & 0xF));
            --temp_b;
            regs_.set_zero(temp_b == 0);
            regs_.set_sub(true);
            write_memory(regs_.read_short(HL), temp_b);
            break;
        case 0x36:
            write_memory(regs_.read_short(HL), arg.lsb);
            break;
        case 0x37:
            scf();
            break;
        case 0x38:
            if ( regs_.carry() )
                jr((int8_t) arg.lsb);
            break;
        case 0x39:
            add_hl(sp_);
            break;
        case 0x3A:
            temp_w = regs_.read_short(HL);
            regs_.load_byte(REG_A, read_memory(temp_w));
            regs_.load_short(HL, temp_w - 1);
            break;
        case 0x3B:
            dec16(SP);
            break;
        case 0x3C:
            inc8(REG_A);
            break;
        case 0x3D:
            dec8(REG_A);
            break;
        case 0x3E:
            regs_.load_byte(REG_A, arg.lsb);
            break;
        case 0x3F:
            ccf();
            break;
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
        case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
        case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            if ( ( opcode & 0x7 ) != 6 )
                regs_.load(( opcode >> 3 ) & 0x7, opcode & 0x7);
            else {
                temp_w = regs_.read_short(HL);
                regs_.load_byte((opcode >> 3) & 0x7, read_memory(temp_w));
            }
            break;
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
            write_memory(regs_.read_short(HL), regs_.read_byte(opcode & 0x7));
            break;
        case 0x76:
            halt();
            break;
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
        case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
        case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
        case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            (this->*alu_functions_[( opcode >> 3 ) & 0x7] )
                    (( ( opcode & 0x7 ) != 6 ) ? regs_.read_byte(opcode & 0x7) : read_memory(regs_.read_short(HL)));
            break;
        case 0xC0:
            gb_.clock(4);
            if ( !regs_.zero() )
                ret();
            break;
        case 0xC1:
            regs_.load_short(BC, stack_pop());
            break;
        case 0xC2:
            if ( !regs_.zero() )
                jp(arg.word);
            break;
        case 0xC3:
            jp(arg.word);
            break;
        case 0xC4:
            if ( !regs_.zero() )
                call(arg.word);
            break;
        case 0xC5:
            gb_.clock(4);
            stack_push(regs_.read_short(BC));
            break;
        case 0xC6:
            add(arg.lsb);
            break;
        case 0xC7:
            call(0x0000);
            break;
        case 0xC8:
            gb_.clock(4);
            if ( regs_.zero() )
                ret();
            break;
        case 0xC9:
            ret();
            break;
        case 0xCA:
            if ( regs_.zero() )
                jp(arg.word);
            break;
        case 0xCB:
            cb(arg.lsb);
            break;
        case 0xCC:
            if ( regs_.zero() )
                call(arg.word);
            break;
        case 0xCD:
            call(arg.word);
            break;
        case 0xCE:
            adc(arg.lsb);
            break;
        case 0xCF:
            call(0x0008);
            break;
        case 0xD0:
            gb_.clock(4);
            if ( !regs_.carry() )
                ret();
            break;
        case 0xD1:
            regs_.load_short(DE, stack_pop());
            break;
        case 0xD2:
            if ( !regs_.carry() )
                jp(arg.word);
            break;
        case 0xD4:
            if ( !regs_.carry() )
                call(arg.word);
            break;
        case 0xD5:
            gb_.clock(4);
            stack_push(regs_.read_short(DE));
            break;
        case 0xD6:
            sub(arg.lsb);
            break;
        case 0xD7:
            call(0x0010);
            break;
        case 0xD8:
            gb_.clock(4);
            if ( regs_.carry() )
                ret();
            break;
        case 0xD9:
            reti();
            break;
        case 0xDA:
            if ( regs_.carry() )
                jp(arg.word);
            break;
        case 0xDC:
            if ( regs_.carry() )
                call(arg.word);
            break;
        case 0xDE:
            sbc(arg.lsb);
            break;
        case 0xDF:
            call(0x0018);
            break;
        case 0xE0:
            write_memory(0xFF00 + arg.lsb, regs_.read_byte(REG_A));
            break;
        case 0xE1:
            regs_.load_short(HL, stack_pop());
            break;
        case 0xE2:
            write_memory(0xFF00 + regs_.read_byte(REG_C), regs_.read_byte(REG_A));
            break;
        case 0xE5:
            gb_.clock(4);
            stack_push(regs_.read_short(HL));
            break;
        case 0xE6:
            and_l(arg.lsb);
            break;
        case 0xE7:
            call(0x0020);
            break;
        case 0xE8:
            add_sp_n((int8_t) arg.lsb);
            break;
        case 0xE9:
            pc_ = regs_.read_short(HL);
            break;
        case 0xEA:
            write_memory(arg.word, regs_.read_byte(REG_A));
            break;
        case 0xEE:
            xor_l(arg.lsb);
            break;
        case 0xEF:
            call(0x0028);
            break;
        case 0xF0:
            regs_.load_byte(REG_A, read_memory(0xFF00 + arg.lsb));
            break;
        case 0xF1:
            regs_.load_short(AF, stack_pop());
            break;
        case 0xF2:
            regs_.load_byte(REG_A, read_memory(0xFF00 + regs_.read_byte(REG_C)));
            break;
        case 0xF3:
            interrupts_->set_ime(false);
            break;
        case 0xF5:
            gb_.clock(4);
            stack_push(regs_.read_short(AF));
            break;
        case 0xF6:
            or_l(arg.lsb);
            break;
        case 0xF7:
            call(0x0030);
            break;
        case 0xF8:
            ldhl_sp_n((int8_t) arg.lsb);
            break;
        case 0xF9:
            gb_.clock(4);
            sp_ = regs_.read_short(HL);
            break;
        case 0xFA:
            regs_.load_byte(REG_A, read_memory(arg.word));
            break;
        case 0xFB:
            ei_last_instruction_ = true;
            break;
        case 0xFE:
            cp(arg.lsb);
            break;
        case 0xFF:
            call(0x0038);
            break;
        default:
            break;
    }
}

void Cpu::update_timers(unsigned int cycles) {
    this->div_counter_ += cycles << (double_speed_ ? 1 : 0);
    if (this->div_counter_ >= 0xFF ) {
        this->div_counter_ -= 0xFF;
        this->div_reg_++;
    }
    if ( this->tac_.test(2) ) {
        this->timer_counter_ -= cycles;
        while (this->timer_counter_ <= 0 ) {
            static int timer_counter_increments[4] = {
                    1024, 16, 64, 256
            };
            timer_counter_ += timer_counter_increments[tac_.to_ulong() & 0x3];
            if ( ++tima_ == 0 ) {
                timer_overflow_ = true;
            }
        }
    }
}

void Cpu::update_buttons() {
    if ( (joypad_->buttons_enabled() && joypad_->buttons_pressed())
        || (joypad_->direction_enabled() && joypad_->direction_pressed()) ) {
            interrupts_->request(Interrupts::jpad);
    }
}

void Cpu::service_interrupts() {
    if ( interrupts_->ime() && interrupts_->interrupts_requested() ) {
        if ( interrupts_->interrupts_pending() ) {
            if (halted_)
                halted_ = false;
            interrupts_->set_ime(false);
        }
        static std::vector<int> interrupt_vectors { 0x40, 0x48, 0x50, 0x58, 0x60 };
        static std::vector<int> interrupts {Interrupts::jpad, Interrupts::serial, Interrupts::timer, Interrupts::lcd, Interrupts::v_blank };

        // Even if it seems that interrupts are traversed and serviced in reverse order, call() pushes the PC on the stack, so if say
        // joypad and VBlank interrupts are requested, the first call will place the Joypad interrupt vector in the PC
        // and the second call will push it on the stack and place the VBlank interrupt vector in the PC
        std::for_each(interrupts.begin(), interrupts.end(), [this](int i){
            if ( interrupts_->is_enabled(i) && interrupts_->is_requested(i) ) {
                interrupts_->unrequest(i);
                call(interrupt_vectors[i]);
                gb_.clock(8);
            }
        });
    }
}

inline void Cpu::stack_push(uint16_t val) {
    sp_ -= 2;
    write_memory_short(sp_, val);
}

inline uint16_t Cpu::stack_pop() {
    uint16_t val = read_memory_short(sp_);
    sp_ += 2;
    return val;
}

void Cpu::add(uint8_t val) {
    uint8_t a = regs_.read_byte(REG_A);
    uint8_t result = a + val;

    regs_.set_zero(result == 0);
    regs_.set_half_carry((a & 0xF) + (val & 0xF) > 0xF);
    regs_.set_carry(result < a);
    regs_.set_sub(false);

    regs_.load_byte(REG_A, result);
}

void Cpu::adc(uint8_t val) {
    uint8_t a = regs_.read_byte(REG_A);
    uint8_t carry = regs_.carry() ? 1 : 0;

    regs_.set_carry((uint16_t) a + val + carry > 0x00FF);
    regs_.set_half_carry((a & 0xF) + (val & 0xF) + carry > 0xF);
    a += ( val + carry );
    regs_.set_zero(a == 0);
    regs_.set_sub(false);

    regs_.load_byte(REG_A, a);
}

void Cpu::sub(uint8_t val) {
    uint8_t a = regs_.read_byte(REG_A);

    regs_.set_zero(a == val);
    regs_.set_half_carry((val & 0xF) > (a & 0xF));
    regs_.set_carry(val > a);
    regs_.set_sub(true);

    regs_.load_byte(REG_A, a - val);
}

void Cpu::sbc(uint8_t val) {
    uint8_t a = regs_.read_byte(REG_A);
    uint8_t carry = regs_.carry() ? 1 : 0;

    regs_.set_half_carry((a & 0xF) < (val & 0xF) + carry);
    regs_.set_carry((uint16_t) a < (uint16_t) val + carry);
    a -= ( val + carry );
    regs_.set_zero(a == 0);
    regs_.set_sub(true);

    regs_.load_byte(REG_A, a);
}

void Cpu::and_l(uint8_t val) {
    uint8_t res = regs_.read_byte(REG_A) & val;

    regs_.set_zero(res == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(true);
    regs_.set_carry(false);

    regs_.load_byte(REG_A, res);
}

void Cpu::or_l(uint8_t val) {
    uint8_t res = regs_.read_byte(REG_A) | val;

    regs_.set_zero(res == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
    regs_.set_carry(false);

    regs_.load_byte(REG_A, res);
}

void Cpu::xor_l(uint8_t val) {
    uint8_t res = regs_.read_byte(REG_A) ^ val;

    regs_.set_zero(res == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
    regs_.set_carry(false);

    regs_.load_byte(REG_A, res);
}

void Cpu::cp(uint8_t val) {
    uint8_t a = regs_.read_byte(REG_A);

    regs_.set_zero(a == val);
    regs_.set_sub(true);
    regs_.set_half_carry((a & 0xF) < (val & 0xF));
    regs_.set_carry(a < val);
}

void Cpu::add_hl(uint16_t val) {
    uint16_t hl = regs_.read_short(HL);
    regs_.set_half_carry((hl & 0x0FFF) > (0x0FFF - (val & 0x0FFF)));
    regs_.set_carry(hl > (0xFFFF - val ));
    regs_.set_sub(false);
    regs_.load_short(HL, hl + val);
    gb_.clock(4);
}

void Cpu::inc8(int r) {
    uint8_t reg = regs_.read_byte(r);
    regs_.set_half_carry((reg & 0xF) == 0xF);
    ++reg;
    regs_.set_zero(reg == 0);
    regs_.set_sub(false);
    regs_.load_byte(r, reg);
}

void Cpu::dec8(int r) {
    uint8_t reg = regs_.read_byte(r);
    regs_.set_half_carry((reg & 0xF) == 0);
    --reg;
    regs_.set_zero(reg == 0);
    regs_.set_sub(true);
    regs_.load_byte(r, reg);
}

void Cpu::inc16(int r) {
    gb_.clock(4);
    if ( r < 3 )
        regs_.load_short(r, regs_.read_short(r) + 1);
    else
        sp_++;
}

void Cpu::dec16(int r) {
    gb_.clock(4);
    if ( r < 3 )
        regs_.load_short(r, regs_.read_short(r) - 1);
    else
        sp_--;
}

inline void Cpu::jp(uint16_t addr) {
    pc_ = addr;
    gb_.clock(4);
}
inline void Cpu::jr(int8_t offset) {
    pc_ += (int8_t) offset;
    gb_.clock(4);
}

void Cpu::ldhl_sp_n(int8_t n) {
    gb_.clock(4);
    uint16_t result = sp_ + n;
    if ( n >= 0 ) {
        regs_.set_carry((sp_ & 0xFF ) + n > 0xFF);
        regs_.set_half_carry((sp_ & 0xF) + (n & 0xF) > 0xF);
    } else {
        regs_.set_carry((result & 0xFF ) <= (sp_ & 0xFF ));
        regs_.set_half_carry((result & 0xF) <= (sp_ & 0xF));
    }

    regs_.set_zero(false);
    regs_.set_sub(false);

    regs_.load_short(HL, result);
}

void Cpu::add_sp_n(int8_t offset) {
    uint8_t tmp = (uint8_t) offset;
    regs_.set_half_carry((sp_ & 0xF) > (0xF - (tmp & 0xF)));
    regs_.set_carry((sp_ & 0xFF ) > (0xFF - tmp ));

    sp_ += offset;
    regs_.set_zero(false);
    regs_.set_sub(false);
    gb_.clock(8);
}

void Cpu::daa() {
    unsigned int binary_val = regs_.read_byte(REG_A);

    if ( regs_.sub() ) {
        if (regs_.half_carry() )
            binary_val = ( binary_val - 6 ) & 0xFF;
        if ( regs_.carry() )
            binary_val = ( binary_val - 0x60 ) & 0xFF;
    } else {
        if (regs_.half_carry() || (binary_val & 0xF ) > 9 )
            binary_val += 0x06;
        if (regs_.carry() || binary_val > 0x9F )
            binary_val += 0x60;
    }

    regs_.set_half_carry(false);
    regs_.set_zero(false);

    if ( binary_val > 0xFF )
        regs_.set_carry(true);
    binary_val &= 0xFF;
    if ( binary_val == 0 )
        regs_.set_zero(true);

    regs_.load_byte(REG_A, (uint8_t) binary_val);
}

void Cpu::cpl() {
    regs_.set_sub(true);
    regs_.set_half_carry(true);
    regs_.load_byte(REG_A, ~(regs_.read_byte(REG_A)));
}

void Cpu::halt() {
    if ( interrupts_->ime() ) {
        halted_ = true;
    } else {
        if ( interrupts_->interrupts_pending() )
            halt_bug_triggered_ = true;
        else
            halted_ = true;
    }
}

void Cpu::stop() {
    // TODO
}

void Cpu::rlca() {
    uint8_t a = regs_.read_byte(REG_A);
    uint8_t b7 = a >> 7;
    regs_.load_byte(REG_A, (a << 1) | b7);

    regs_.set_zero(false);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
    regs_.set_carry(b7);
}

void Cpu::rla() {
    bool c = regs_.carry();
    uint8_t a = regs_.read_byte(REG_A);

    regs_.set_carry((a & 0x80 ) == 0x80);
    a <<= 1;
    a |= c;

    regs_.load_byte(REG_A, a);

    regs_.set_zero(false);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
}

void Cpu::rrca() {
    uint8_t a = regs_.read_byte(REG_A);
    uint8_t b0 = a & 1;
    regs_.load_byte(REG_A, (a >> 1) | (b0 << 7));

    regs_.set_zero(false);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
    regs_.set_carry(b0);
}

void Cpu::rra() {
    uint8_t carry = regs_.carry() << 7;
    uint8_t a = regs_.read_byte(REG_A);

    regs_.set_carry(a & 1);
    a >>= 1;
    a |= carry;

    regs_.load_byte(REG_A, a);

    regs_.set_zero(false);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
}

void Cpu::scf() {
    regs_.set_carry(true);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
}

void Cpu::call(uint16_t addr) {
    gb_.clock(4);
    stack_push(pc_);
    pc_ = addr;
}

void Cpu::ccf() {
    regs_.set_carry(!regs_.carry());
    regs_.set_sub(false);
    regs_.set_half_carry(false);
}

inline void Cpu::ret() {
    pc_ = stack_pop();
    gb_.clock(4);
}

void Cpu::reti() {
    gb_.clock(4);
    pc_ = stack_pop();
    interrupts_->set_ime(true);
}

void Cpu::cb(uint8_t opcode) {
    switch ( ( opcode & 0xC0 ) >> 6 ) {
        case 0:
            if ( ( opcode & 0x7 ) != 6 )
                if ( ( ( opcode >> 3 ) & 0x7 ) == 0x7 && ( opcode & 0x7 ) == 7 )
                    srl_a();
                else {
                    uint8_t val = regs_.read_byte(opcode & 0x7);
                    uint8_t res = (this->*rotate_instructions_[( opcode >> 3 ) & 0x7] )(val);
                    regs_.load_byte(opcode & 0x7, res);
                }
            else {
                uint8_t res = read_memory(regs_.read_short(HL));
                res = (this->*rotate_instructions_[( opcode >> 3 ) & 0x7] )(res);
                write_memory(regs_.read_short(HL), res);
            }
            break;
        case 1:
            bit(( opcode >> 3 ) & 0x7,
                ( opcode & 0x7 ) != 6 ? regs_.read_byte(opcode & 0x7)
                                          : read_memory(regs_.read_short(HL)));
            break;
        case 2:
            if ( ( opcode & 0x7 ) != 6 )
                regs_.load_byte(opcode & 0x7, res((opcode >> 3) & 0x7, regs_.read_byte(opcode & 0x7)));
            else
                write_memory(regs_.read_short(HL), res(( opcode >> 3 ) & 0x7, read_memory(regs_.read_short(HL))));
            break;
        case 3:
            if ( ( opcode & 0x7 ) != 6 )
                regs_.load_byte(opcode & 0x7, set((opcode >> 3) & 0x7, regs_.read_byte(opcode & 0x7)));
            else
                write_memory(regs_.read_short(HL), set(( opcode >> 3 ) & 0x7, read_memory(regs_.read_short(HL))));
            break;
    }
}

uint8_t Cpu::rlc(uint8_t val) {
    uint8_t carry = ( val & 0x80 ) >> 7;
    regs_.set_carry(val & 0x80);

    val <<= 1;
    val += carry;

    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

uint8_t Cpu::rrc(uint8_t val) {
    uint8_t carry = val & 1;
    val >>= 1;
    val |= ( carry << 7 );

    regs_.set_carry(carry);
    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

uint8_t Cpu::rl(uint8_t val) {
    bool carry = regs_.carry();
    regs_.set_carry((val & 0x80 ) == 0x80);
    val <<= 1;
    val += carry;

    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

uint8_t Cpu::rr(uint8_t val) {
    uint8_t carry = regs_.carry() << 7;

    regs_.set_carry(val & 1);
    val >>= 1;
    val += carry;

    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

uint8_t Cpu::sla(uint8_t val) {
    regs_.set_carry(val & 0x80);
    val <<= 1;
    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

uint8_t Cpu::sra(uint8_t val) {
    regs_.set_carry(val & 1);
    val = ( val & 0x80 ) | ( val >> 1 );

    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

uint8_t Cpu::swap(uint8_t val) {
    val = ( ( val & 0xF ) << 4 ) | ( ( val & 0xF0 ) >> 4 );
    regs_.set_zero(val == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
    regs_.set_carry(false);

    return val;
}

uint8_t Cpu::srl(uint8_t val) {
    regs_.set_carry(val & 1);
    val >>= 1;

    regs_.set_zero(val == 0);

    regs_.set_sub(false);
    regs_.set_half_carry(false);

    return val;
}

void Cpu::bit(int n, uint8_t val) {
    regs_.set_zero(!(val & (1 << n)));
    regs_.set_sub(false);
    regs_.set_half_carry(true);
}

void Cpu::srl_a() {
    uint8_t a = regs_.read_byte(REG_A);
    regs_.set_carry(a & 1);
    a >>= 1;
    regs_.set_zero(a == 0);
    regs_.set_sub(false);
    regs_.set_half_carry(false);
    regs_.load_byte(REG_A, a);
}

void Cpu::write_memory(unsigned int addr, uint8_t val) {
    gb_.clock(4);
    gb_.mmu_->write(addr, val);
}

uint8_t Cpu::read_memory(unsigned int addr) {
    gb_.clock(4);
    uint8_t r = gb_.mmu_->read(addr);
    return r;
}

void Cpu::write_memory_short(uint16_t addr, uint16_t val) {
    write_memory(addr, val & 0xFF);
    write_memory(addr + 1, ( val & 0xFF00 ) >> 8);
}

uint16_t Cpu::read_memory_short(uint16_t addr) {
    return (read_memory(addr) | ((unsigned short) (read_memory(addr + 1)) << 8));
}