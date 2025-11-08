#include "events.h"
#include <SDL3/SDL.h>

bool handle_sdl_events(bool *running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            *running = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_ESCAPE) {
                *running = false;
            }
        }
    }
    return *running;
}