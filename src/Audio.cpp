//
// Created by antonio on 07/09/20.
//

#include <SDL2/SDL_audio.h>
#include <Audio.h>

Audio::Audio() : buffer(SAMPLE_SIZE) {
    SDL_AudioSpec audioSpec, have;
    SDL_memset(&audioSpec, 0, sizeof(audioSpec));

    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_F32SYS;
    audioSpec.channels = 2;
    audioSpec.samples = SAMPLE_SIZE;
    audioSpec.callback = nullptr;
    audioSpec.userdata = nullptr;

    dev = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, &have,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    SDL_PauseAudioDevice(dev, 0);

    buffer_index = 0;
    std::fill(buffer.begin(), buffer.end(), 0);
}

Audio::~Audio() {
    SDL_CloseAudioDevice(dev);
}

void Audio::update(const apu::audio_output& output) {
    float buffer0 = 0.0;
    float buffer1 = 0.0;
    int left_volume = (output.left_volume << 7) / 7;
    int right_volume = (output.right_volume << 7) / 7;

    if ( output.ch1_output >= 0 ) {
        buffer1 = (float) output.ch1_output / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), left_volume);
    }
    if ( output.ch2_output >= 0 ) {
        buffer1 = (float) output.ch2_output / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), left_volume);
    }
    if ( output.wave_output >= 0 ) {
        buffer1 = (float) output.wave_output / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), left_volume);
    }
    if ( output.noise_output >= 0 ) {
        buffer1 = (float) output.noise_output / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), left_volume);
    }
    buffer[buffer_index++] = buffer0;
    buffer0 = 0;

    if ( output.ch1_output_right >= 0 ) {
        buffer1 = (float) output.ch1_output_right / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), right_volume);
    }
    if ( output.ch2_output_right >= 0 ) {
        buffer1 = (float) output.ch2_output_right / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), right_volume);
    }
    if ( output.wave_output_right >= 0 ) {
        buffer1 = (float) output.wave_output_right / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), right_volume);
    }
    if ( output.noise_output_right >= 0 ) {
        buffer1 = (float) output.noise_output_right / 100;
        SDL_MixAudioFormat((Uint8 *) ( &buffer0 ), (Uint8 *) ( &buffer1 ), AUDIO_F32SYS, sizeof(float), right_volume);
    }
    buffer[buffer_index++] = buffer0;
    if ( buffer_index >= SAMPLE_SIZE ) {
        buffer_index = 0;
        while (SDL_GetQueuedAudioSize(dev) > SAMPLE_SIZE * sizeof(float));
        SDL_QueueAudio(dev, (void *) buffer.data(), SAMPLE_SIZE * sizeof(float));
    }
}