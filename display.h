/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>

bool init_sdl(const AppConfig *config, SDL_Window **out_window, SDL_Renderer **out_renderer);
void draw_pixels_from_buffer(SDL_Renderer *renderer, const unsigned char *buf, size_t len);

#endif // DISPLAY_H