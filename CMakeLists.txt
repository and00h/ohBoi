cmake_minimum_required(VERSION 3.19)
project(ohBoi)
set(CMAKE_CXX_STANDARD 20)

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
set(LIBRARIES ${SDL2_LIBRARIES})
include_directories(${INCLUDE_DIR} ${SDL2_INCLUDE_DIRS})

include_directories(inc)
include_directories(inc/Core)
include_directories(inc/Core/Audio)
include_directories(inc/Core/Audio/utils)
include_directories(inc/Core/cpu)
include_directories(inc/Core/Graphics)
include_directories(inc/Core/Memory)
include_directories(inc/Core/Memory/MBC)
include_directories(inc/Logger)

add_executable(ohBoi
        inc/Core/Audio/utils/Envelope.h
        inc/Core/Audio/utils/Length_counter.h
        inc/Core/Audio/utils/Programmable_timer.h
        inc/Core/Audio/utils/Sweep.h
        inc/Core/Audio/apu.h
        inc/Core/Audio/audio_ch_1.h
        inc/Core/Audio/audio_ch_2.h
        inc/Core/Audio/noise_ch.h
        inc/Core/Audio/wave_ch.h
        inc/Core/cpu/Cpu.h
        inc/Core/cpu/Interrupts.h
        inc/Core/cpu/Registers.h
        inc/Core/Graphics/CGBPalette.h
        inc/Core/Graphics/Ppu.h
        inc/Core/Memory/MBC/Mbc.h
        inc/Core/Memory/MBC/Mbc1.h
        inc/Core/Memory/MBC/Mbc3.h
        inc/Core/Memory/MBC/Mbc5.h
        inc/Core/Memory/MBC/None.h
        inc/Core/Memory/MBC/RTC.h
        inc/Core/Memory/Address_space.h
        inc/Core/Memory/Ext_ram.h
        inc/Core/Memory/Memory.h
        inc/Core/Memory/Rom.h
        inc/Core/Memory/Wram.h
        inc/Core/Gameboy.h
        inc/Core/Joypad.h
        inc/Logger/Logger.h
        inc/Audio.h
        inc/Display.h
        inc/macro.h
        inc/types.h
        inc/util.h
        src/Core/Audio/apu.cpp
        src/Core/Audio/audio_ch_1.cpp
        src/Core/Audio/audio_ch_2.cpp
        src/Core/Audio/noise_ch.cpp
        src/Core/Audio/wave_ch.cpp
        src/Core/cpu/Cpu.cpp
        src/Core/cpu/CPUDefinitions.cpp
        src/Core/cpu/Registers.cpp
        src/Core/Graphics/CGBPalette.cpp
        src/Core/Graphics/Ppu.cpp
        src/Core/Memory/MBC/Mbc.cpp
        src/Core/Memory/MBC/Mbc1.cpp
        src/Core/Memory/MBC/Mbc3.cpp
        src/Core/Memory/MBC/Mbc5.cpp
        src/Core/Memory/MBC/None.cpp
        src/Core/Memory/MBC/RTC.cpp
        src/Core/Memory/Memory.cpp
        src/Core/Gameboy.cpp
        src/Core/Joypad.cpp
        src/Logger/Logger.cpp
        src/Audio.cpp
        src/Display.cpp
        src/main.cpp src/Core/Graphics/Tile.cpp inc/Core/Graphics/Tile.h src/Core/Graphics/Pixel_fetcher.cpp
        )

target_compile_options(ohBoi PRIVATE -O3 -Wall -Wextra)
target_link_libraries(ohBoi ${LIBRARIES})