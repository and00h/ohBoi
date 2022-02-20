//
// Created by antonio on 02/08/20.
//

#include <Display.h>

Display::Display() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("OhBoi",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            160, 144,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_GL_SetSwapInterval(0);
}

Display::~Display() {
    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

void Display::update_display(Uint32 *framebuffer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(renderer);

    int pitch;
    Uint32 *pixels;
    SDL_LockTexture(screen, nullptr, (void **) &pixels, &pitch);
    memcpy((void *) pixels, (void *) framebuffer, 144 * pitch);
    SDL_UnlockTexture(screen);

    SDL_RenderCopy(renderer, screen, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Display::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}
