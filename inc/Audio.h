//
// Created by antonio on 07/09/20.
//

#ifndef OHBOI_AUDIO_H
#define OHBOI_AUDIO_H

#include <Core/Audio/apu.h>
#include <SDL2/SDL_audio.h>
#include <vector>

#define SAMPLE_SIZE 2048

class Audio {
public:
    Audio();
    ~Audio();

    void update(const apu::audio_output& a);

    void reset() {
        SDL_PauseAudioDevice(dev, 0);

        buffer_index = 0;
        std::fill(buffer.begin(), buffer.end(), 0);
    };
private:
    SDL_AudioDeviceID dev;
    std::vector<float> buffer;
    int buffer_index;
};
#endif //OHBOI_AUDIO_H
