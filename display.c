/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#include "display.h"
#include "config.h"

#include <SDL3/SDL.h>

bool init_sdl(const AppConfig *config, SDL_Window **out_window, SDL_Renderer **out_renderer) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }

    // Initial size based on logical 80x8 and initial scale.
    const int init_w = config->width * config->scale;
    const int init_h = config->height * config->scale;

    SDL_Window *window = SDL_CreateWindow(
        config->title,
        init_w,
        init_h,
        SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Quit();
        return false;
    }

#ifdef SDL_WINDOWPROP_MINIMUM_SIZE
    SDL_SetWindowMinimumSize(window, config->width, config->height);
#endif

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Clear background (black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw a clear "READY" pattern using diagonal green/black stripes
    // across the entire 80x8 logical matrix. This avoids text layout issues
    // and makes the ready state visually obvious until multicast data arrives.
    for (int y = 0; y < config->height; ++y) {
        for (int x = 0; x < config->width; ++x) {
            // Simple diagonal pattern: periodic stripes based on x+y.
            // Adjust modulus / threshold to tune density.
            if (((x + y) % 4) < 2) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
            }
            SDL_RenderPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);

    *out_window = window;
    *out_renderer = renderer;
    return true;
}

void draw_pixels_from_buffer(SDL_Renderer *renderer, const unsigned char *buf, size_t len) {
    if (!renderer || !buf) {
        return;
    }

    if (len != MC_EXPECTED_SIZE) {
        // Ignore frames with unexpected size
        return;
    }

    SDL_Window *window = SDL_GetRenderWindow(renderer);

    int win_w = 0;
    int win_h = 0;
    SDL_GetRenderOutputSize(renderer, &win_w, &win_h);

    if (win_w <= 0 || win_h <= 0) {
        return;
    }

    // Compute pixel size based on current window, preserving 80x8 aspect.
    float pixel_size = (float)win_w / (float)WIDTH;
    float max_pixel_from_height = (float)win_h / (float)HEIGHT;
    if (pixel_size * HEIGHT > win_h) {
        pixel_size = max_pixel_from_height;
    }

    if (pixel_size <= 0.1f) {
        return;
    }

    // Center the 80x8 area within the window.
    float used_w = pixel_size * (float)WIDTH;
    float used_h = pixel_size * (float)HEIGHT;
    float offset_x = ((float)win_w - used_w) * 0.5f;
    float offset_y = ((float)win_h - used_h) * 0.5f;

    // Update window title with current scale and window size.
    if (window) {
        char title[256];
        // pixel_size is in render-output pixels per logical LED.
        SDL_snprintf(title,
                     sizeof(title),
                     "80x8 LedBanner - scale %.2f - %dx%d",
                     pixel_size,
                     win_w,
                     win_h);
        SDL_SetWindowTitle(window, title);
    }

    // Each pixel: 2 bytes: RRRRRGGG GGGBBBBB  (5-6-5)
    // buf index: (y * WIDTH + x) * 2
    static_assert(MC_EXPECTED_SIZE == WIDTH * HEIGHT * 2,
                  "MC_EXPECTED_SIZE must equal WIDTH * HEIGHT * 2");

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            size_t idx = (size_t)(y * WIDTH + x) * 2;
            uint16_t raw = (uint16_t)((buf[idx] << 8) | buf[idx + 1]);

            uint8_t r5 = (raw >> 11) & 0x1F;
            uint8_t g6 = (raw >> 5) & 0x3F;
            uint8_t b5 = raw & 0x1F;

            // Scale 5-bit and 6-bit to 8-bit
            uint8_t r = (uint8_t)((r5 * 255 + 15) / 31);
            uint8_t g = (uint8_t)((g6 * 255 + 31) / 63);
            uint8_t b = (uint8_t)((b5 * 255 + 15) / 31);

            SDL_FRect rct;
            rct.x = offset_x + (float)x * pixel_size;
            rct.y = offset_y + (float)y * pixel_size;
            rct.w = pixel_size;
            rct.h = pixel_size;

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderFillRect(renderer, &rct);
        }
    }

    SDL_RenderPresent(renderer);
}