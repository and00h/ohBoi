#include <iostream>
#include <SDL2/SDL.h>

#include <Audio.h>
#include <Display.h>
#include <Core/Gameboy.h>
#include <Core/Joypad.h>
#include <Logger/Logger.h>
#include <filesystem>
#include <map>
#include <functional>
#include <thread>

namespace {
    void key_down(std::unique_ptr<gb::Gameboy> &gb, SDL_Keycode key) {
        if ( not gb )
            return;
        static std::map<SDL_Keycode, std::function<void(void)>> key_callbacks = {
                {SDLK_1, [&gb] { gb->disable_bg(); }},
                {SDLK_2, [&gb] { gb->disable_window(); }},
                {SDLK_3, [&gb] { gb->disable_sprites(); }},
                {SDLK_4, [&gb] { gb->toggle_ch1(); }},
                {SDLK_5, [&gb] { gb->toggle_ch2(); }},
                {SDLK_6, [&gb] { gb->toggle_wave(); }},
                {SDLK_7, [&gb] { gb->toggle_noise(); }},
                {SDLK_8, [&gb] { gb->set_speed(15); }},
                {SDLK_9, [&gb] { gb->set_speed(20); }},
                {SDLK_0, [&gb] { gb->set_speed(40); }},
                {SDLK_COMMA, [&gb] { gb->set_speed(1); }},
                {SDLK_p, [&gb] { gb->toggle_pause(); }},
                {SDLK_a, [&gb] { gb->press_key(Joypad::KEY_A); }},
                {SDLK_s, [&gb] { gb->press_key(Joypad::KEY_B); }},
                {SDLK_UP, [&gb] { gb->press_key(Joypad::KEY_UP); }},
                {SDLK_DOWN, [&gb] { gb->press_key(Joypad::KEY_DOWN); }},
                {SDLK_LEFT, [&gb] { gb->press_key(Joypad::KEY_LEFT); }},
                {SDLK_RIGHT, [&gb] { gb->press_key(Joypad::KEY_RIGHT); }},
                {SDLK_x, [&gb] { gb->press_key(Joypad::KEY_SELECT); }},
                {SDLK_z, [&gb] { gb->press_key(Joypad::KEY_START); }}
        };

        try {
            key_callbacks.at(key)();
        } catch (std::out_of_range &e) {
            Logger::warning("Input", "Unknown keycode");
        }
    }

    void key_up(std::unique_ptr<gb::Gameboy> &gb, SDL_Keycode key) {
        if (not gb)
            return;
        static std::map<SDL_Keycode, std::function<void(void)>> key_callbacks = {
                {SDLK_a, [&gb] { gb->release_key(Joypad::KEY_A); }},
                {SDLK_s, [&gb] { gb->release_key(Joypad::KEY_B); }},
                {SDLK_UP, [&gb] { gb->release_key(Joypad::KEY_UP); }},
                {SDLK_DOWN, [&gb] { gb->release_key(Joypad::KEY_DOWN); }},
                {SDLK_LEFT, [&gb] { gb->release_key(Joypad::KEY_LEFT); }},
                {SDLK_RIGHT, [&gb] { gb->release_key(Joypad::KEY_RIGHT); }},
                {SDLK_x, [&gb] { gb->release_key(Joypad::KEY_SELECT); }},
                {SDLK_z, [&gb] { gb->release_key(Joypad::KEY_START); }}
        };
        try {
            key_callbacks.at(key)();
        } catch (std::out_of_range &e) {
            Logger::warning("Input", "Unknown key");
        }
    }
}

int main() {
    static std::map<Uint32, std::function<void(void)>> event_callbacks;

    Logger::toggle_info();
    std::unique_ptr<gb::Gameboy> gb(nullptr);

    Display display{};
    Audio audio{};

    display.clear();
    std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

    bool close = false;
    while ( not close ) {
        SDL_Event e;
        while ( (not close) and SDL_PollEvent(&e) ) {
            if ( e.type == SDL_DROPFILE ) {
                std::filesystem::path game_path{e.drop.file};
                if ( gb ) {
                    display.clear();
                } else {
                    display.set_title("OhBoi");
                }
                gb = std::make_unique<gb::Gameboy>(game_path);
                event_callbacks.clear();
                event_callbacks = {
                        {SDL_QUIT, [&close] {
                            close = true;
                        }},
                        {SDL_KEYDOWN, [&gb, &e] {
                            ::key_down(gb, e.key.keysym.sym);
                        }},
                        { SDL_KEYUP, [&gb, &e] {
                            ::key_up(gb, e.key.keysym.sym);
                        }}
                };
                continue;
            } else {
                try {
                    event_callbacks.at(e.type)();
                } catch (std::out_of_range &e) {

                }
            }
        }
        if ( gb ) {
            a = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> work_time = a - b;

            if ( work_time.count() < (1000.0/60.0) ) {
                std::chrono::duration<double, std::milli> delta_ms((1000.0/60.0) - work_time.count());
                auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
            }

            b = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> sleep_time = b - a;

            gb->reset_cpu_cycle_counter();
            while ( gb->get_cpu_cycles() < (gb::cpu::clock_speed / 60) ) {
                gb->step();
                if ( gb->new_audio_available() ) {
                    gb->set_audio_reproduced();
                    audio.update(gb->get_audio_output());
                }
            }
            display.update_display(gb->get_screen());
        }
    }
    return 0;
}