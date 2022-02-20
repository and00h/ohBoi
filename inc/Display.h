//
// Created by antonio on 02/08/20.
//

#ifndef OHBOI_DISPLAY_H
#define OHBOI_DISPLAY_H

#include <string>
#include <SDL2/SDL.h>

class Display {
public:
    Display();
    ~Display();

    void update_display(Uint32 *framebuffer);
    void clear();
    void set_title(const char *new_title) { if ( window != nullptr) SDL_SetWindowTitle(window, new_title); }
    std::string get_title() { return window != nullptr ? SDL_GetWindowTitle(window) : ""; };

private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen;

};


#endif //OHBOI_DISPLAY_H
