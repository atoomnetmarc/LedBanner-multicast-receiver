/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#include "config.h"
#include "display.h"
#include "events.h"
#include "multicast.h"
#include "stats.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MC_BUF_SIZE 2048

static void
receive_and_render_loop(SDL_Renderer *renderer, int mc_sock) {
    bool running = true;
    unsigned char mc_buf[MC_BUF_SIZE];
    StatsState stats = {0};

    while (running) {
        if (!handle_sdl_events(&running)) {
            break;
        }

        if (mc_sock >= 0) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(mc_sock, &rfds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0; // poll, no block

            int ret = select(mc_sock + 1, &rfds, NULL, NULL, &tv);
            if (ret > 0 && FD_ISSET(mc_sock, &rfds)) {
                ssize_t n = recv(mc_sock, mc_buf, sizeof(mc_buf), 0);
                if (n > 0) {
                    update_stats_and_log(&stats, n);

                    if ((size_t)n == MC_EXPECTED_SIZE) {
                        draw_pixels_from_buffer(renderer, mc_buf, (size_t)n);
                    } else {
                        fprintf(stderr,
                                "Warning: received unexpected frame size: %zd bytes (expected %d), frame ignored\n",
                                n,
                                MC_EXPECTED_SIZE);
                    }
                }
            }
        }

        SDL_Delay(10);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const AppConfig config = DEFAULT_APPCONFIG;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!init_sdl(&config, &window, &renderer)) {
        return 1;
    }

    int mc_sock = setup_multicast_socket(&config);
    if (mc_sock < 0) {
        fprintf(stderr, "Warning: multicast setup failed, continuing without UDP\n");
    }

    receive_and_render_loop(renderer, mc_sock);

    if (mc_sock >= 0) {
        close(mc_sock);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}