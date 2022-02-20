//
// Created by antonio on 30/07/20.
//

#ifndef OHBOI_CPU_H
#define OHBOI_CPU_H


#include <vector>
#include <memory>
#include <iostream>

#include "Registers.h"
#include "Interrupts.h"
#include "Core/Joypad.h"

#define CLOCK_SPEED         4194304

extern const std::vector<std::string> instr;
extern const uint8_t cb_instructions_cycles[0x100];
extern const struct opcode {
    int operands;
    int cycles;
} opcodes[0x100];
extern const int timerCounterValues[4];

namespace gb {
    class Gameboy;
    namespace debug {
        class Debugger;
        class Cpu_debugger;
    }
}

namespace gb::cpu {
    const unsigned int clock_speed = 4194304;

    union Instr_argument;

    class Cpu {
    public:
        Cpu(Gameboy &gb, std::shared_ptr<Interrupts> interrupts, std::shared_ptr<Joypad> joypad);
        ~Cpu() { std::cout << "Cpu destroyed" << std::endl; }
        void step();

        [[nodiscard]] unsigned int get_cycles() const { return cycles_; }
        [[nodiscard]] uint8_t get_div_reg() const { return div_reg_; }
        [[nodiscard]] uint8_t get_tac() const { return ((uint8_t) tac_.to_ulong() & 0xFF) | 0xF8; }
        [[nodiscard]] uint8_t get_tima() const { return tima_; }
        [[nodiscard]] uint8_t get_tma() const { return tma_; }

        void reset();
        void reset_cycle_counter() { cycles_ = 0; }
        void reset_div_reg() { this->div_reg_ = 0; }

        void set_tac(uint8_t tac_new) { tac_ = tac_new; }
        void set_tima(uint8_t tima_new) { this->tima_ = tima_new; }
        void set_tma(uint8_t tma_new) { this->tma_ = tma_new; }
        void update_timer_counter() { timer_counter_ = timerCounterValues[tac_.to_ulong() & 0b11]; }

        void set_double_speed(bool val) { double_speed_ = val; }
        [[nodiscard]] bool double_speed() const { return double_speed_; }

        void update_timers(unsigned int cycles);

    private:
        Gameboy& gb_;
        Registers regs_;
        std::shared_ptr<Interrupts> interrupts_;
        std::shared_ptr<Joypad> joypad_;
        uint16_t pc_;
        uint16_t sp_;

        uint8_t tima_;
        uint8_t tma_;
        std::bitset<8> tac_;
        int timer_counter_;

        uint8_t div_reg_;
        uint16_t div_counter_;

        bool booting_;
        bool debug_;
        bool ei_last_instruction_;
        bool halted_;
        bool halt_bug_triggered_;
        bool timer_overflow_;
        bool double_speed_;

        unsigned int cycles_;

        const std::vector<void (Cpu::*)(uint8_t)> alu_functions_;
        const std::vector<uint8_t (Cpu::*)(uint8_t)> rotate_instructions_;

        inline uint8_t read_memory(unsigned int addr);
        inline void write_memory(unsigned int addr, uint8_t val);

        void decode_n_xecute(uint8_t opcode, Instr_argument arg);
        unsigned int fetch();
        void service_interrupts();
        inline uint16_t stack_pop();
        inline void stack_push(uint16_t val);
        void update_buttons();

        inline void write_memory_short(uint16_t addr, uint16_t val);
        inline uint16_t read_memory_short(uint16_t addr);

        void adc(uint8_t val);
        void add(uint8_t val);
        void add_hl(uint16_t val);
        void add_sp_n(int8_t offset);
        void and_l(uint8_t val);
        inline void call(uint16_t addr);
        inline void ccf();
        void cp(uint8_t val);
        inline void cpl();
        void daa();
        void dec8(int r);
        inline void dec16(int r);
        void halt();
        void inc8(int r);
        inline void inc16(int r);
        inline void jp(uint16_t addr);
        inline void jr(int8_t offset);
        void ldhl_sp_n(int8_t n);
        void or_l(uint8_t val);
        inline void ret();
        inline void reti();
        void rla();
        void rlca();
        void rra();
        void rrca();
        void sbc(uint8_t val);
        inline void scf();
        void stop();
        void sub(uint8_t val);
        void xor_l(uint8_t val);

        // CB-prefixed instructions
        void cb(uint8_t opcode);
        void bit(int n, uint8_t val);
        [[nodiscard]] uint8_t rl(uint8_t val);
        [[nodiscard]] uint8_t rlc(uint8_t val);
        [[nodiscard]] uint8_t rr(uint8_t val);
        [[nodiscard]] uint8_t rrc(uint8_t val);
        [[nodiscard]] uint8_t sla(uint8_t val);
        [[nodiscard]] uint8_t sra(uint8_t val);
        [[nodiscard]] uint8_t srl(uint8_t val);
        void srl_a();
        [[nodiscard]] uint8_t swap(uint8_t val);
    };
}

#endif //OHBOI_CPU_H
