/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#include "config.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define FPS            10
#define FRAME_DELAY_MS (1000 / FPS)

/* Duration for one full rainbow cycle in seconds. */
#define RAINBOW_PERIOD_SEC 17

/* Background color for dead cells (RGB565). */
#define COLOR_BG 0x0000

/* Game-of-Life reset / boredom detection thresholds.
 * Tuned for the 80x8 display; adjust here if behavior needs tweaking.
 */
#define FREEZE_SAME_PATTERN_THRESHOLD 8   /* consecutive identical patterns */
#define FREEZE_STABLE_CELLS_THRESHOLD 40  /* same live cell count for N generations */
#define FREEZE_MAX_GENERATIONS        240 /* hard cap on generations per game */
#define DEAD_RESPAWN_THRESHOLD        4   /* consecutive empty generations before respawn */

/* Simple RGB565 helpers for rainbow coloring. */
static unsigned short make_rgb565(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned short)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
}

/*
 * Horizontal rainbow across WIDTH with time-based scroll.
 * The rainbow completes one full cycle every RAINBOW_PERIOD_SEC seconds.
 * Uses a global frame counter to create a smooth phase shift.
 */
static unsigned int rainbow_offset = 0;

static unsigned short rainbow_color_for_x(int x) {
    if (WIDTH <= 1) {
        return make_rgb565(255, 0, 0);
    }

    /* One full cycle per RAINBOW_PERIOD_SEC seconds:
     * period_frames = RAINBOW_PERIOD_SEC * FPS.
     */
    const float period_frames = (float)(RAINBOW_PERIOD_SEC * FPS);

    /* Base position for this column. */
    float base = (float)x / (float)(WIDTH - 1);

    /* Phase offset derived from global frame counter.
     * Use negative direction so colors move left-to-right on screen.
     */
    float phase = (float)rainbow_offset / period_frames;

    float t = base - phase;

    /* Wrap t into [0,1) without libm to avoid -lm linkage. */
    if (t >= 1.0f) {
        t -= (int)t;
    } else if (t < 0.0f) {
        int k = (int)(-t) + 1;
        t += (float)k;
    }

    float r, g, b;

    if (t < 1.0f / 6.0f) {
        float u = t * 6.0f;
        r = 1.0f;
        g = u;
        b = 0.0f;
    } else if (t < 2.0f / 6.0f) {
        float u = (t - 1.0f / 6.0f) * 6.0f;
        r = 1.0f - u;
        g = 1.0f;
        b = 0.0f;
    } else if (t < 3.0f / 6.0f) {
        float u = (t - 2.0f / 6.0f) * 6.0f;
        r = 0.0f;
        g = 1.0f;
        b = u;
    } else if (t < 4.0f / 6.0f) {
        float u = (t - 3.0f / 6.0f) * 6.0f;
        r = 0.0f;
        g = 1.0f - u;
        b = 1.0f;
    } else if (t < 5.0f / 6.0f) {
        float u = (t - 4.0f / 6.0f) * 6.0f;
        r = u;
        g = 0.0f;
        b = 1.0f;
    } else {
        float u = (t - 5.0f / 6.0f) * 6.0f;
        r = 1.0f;
        g = 0.0f;
        b = 1.0f - u;
    }

    unsigned char R = (unsigned char)(r * 255.0f);
    unsigned char G = (unsigned char)(g * 255.0f);
    unsigned char B = (unsigned char)(b * 255.0f);

    return make_rgb565(R, G, B);
}

static void msleep(unsigned int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000U) * 1000000L;
    while (nanosleep(&ts, &ts) < 0 && errno == EINTR) {
        // retry if interrupted
    }
}

static void clear_field(unsigned char *field) {
    memset(field, 0, WIDTH * HEIGHT);
}

static void randomize_field(unsigned char *field) {
    int alive = 0;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            unsigned char v = (rand() % 4 == 0) ? 1 : 0;
            field[y * WIDTH + x] = v;
            if (v) {
                alive++;
            }
        }
    }

    printf("[GoL] Seeded new game: %d alive cells\n", alive);
}

static int count_neighbors(const unsigned char *field, int x, int y) {
    int count = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) {
                continue;
            }

            /* Wrap around edges (toroidal field). */
            int nx = x + dx;
            int ny = y + dy;

            if (nx < 0) {
                nx = WIDTH - 1;
            } else if (nx >= WIDTH) {
                nx = 0;
            }

            if (ny < 0) {
                ny = HEIGHT - 1;
            } else if (ny >= HEIGHT) {
                ny = 0;
            }

            if (field[ny * WIDTH + nx]) {
                count++;
            }
        }
    }

    return count;
}

static void step_game_of_life(const unsigned char *current, unsigned char *next) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int neighbors = count_neighbors(current, x, y);
            int alive = current[y * WIDTH + x] != 0;

            if (alive) {
                next[y * WIDTH + x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                next[y * WIDTH + x] = (neighbors == 3) ? 1 : 0;
            }
        }
    }
}

static void field_to_rgb565_frame(const unsigned char *field, unsigned char *frame) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int idx = y * WIDTH + x;
            unsigned short color;

            if (field[idx]) {
                /* Alive cell: horizontal rainbow color based on x and time (scrolling). */
                color = rainbow_color_for_x(x);
            } else {
                color = COLOR_BG;
            }

            frame[idx * 2 + 0] = (unsigned char)((color >> 8) & 0xFF);
            frame[idx * 2 + 1] = (unsigned char)(color & 0xFF);
        }
    }
}

static void reset_game_stats(unsigned int *deadcounter,
                             unsigned int *samecounter,
                             unsigned int *stablecounter,
                             unsigned long *game_generations,
                             unsigned long *game_born,
                             unsigned long *game_died) {
    *deadcounter = 0;
    *samecounter = 0;
    *stablecounter = 0;
    *game_generations = 0;
    *game_born = 0;
    *game_died = 0;
}

int main(void) {
    const char *group = MC_GROUP;
    int port = MC_PORT;

    printf("Game of Life multicast test sender\n");
    printf("Target: %s:%d\n", group, port);
    printf("Resolution: %dx%d, frame size %d bytes\n", WIDTH, HEIGHT, MC_EXPECTED_SIZE);
    printf("Sending at %d FPS\n", FPS);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    /*
     * Configure multicast sending:
     * - Use default interface (no explicit IP_MULTICAST_IF), so it behaves like a normal sender.
     * - Use TTL = 0 so traffic is kept on the local host only.
     */
    unsigned char ttl = 0;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("setsockopt(IP_MULTICAST_TTL)");
        close(sock);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(group, &addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid multicast group %s\n", group);
        close(sock);
        return 1;
    }

    unsigned char field_a[WIDTH * HEIGHT];
    unsigned char field_b[WIDTH * HEIGHT];
    unsigned char *cur = field_a;
    unsigned char *next = field_b;

    unsigned char frame[MC_EXPECTED_SIZE];

    srand((unsigned int)time(NULL));
    clear_field(cur);
    randomize_field(cur);

    /* Detect dead/frozen patterns and re-randomize when needed. */
    unsigned int deadcounter = 0;
    unsigned int stablecounter = 0;
    unsigned int samecounter = 0;

    /* Per-game statistics. */
    unsigned long game_generations = 0;
    unsigned long game_born = 0;
    unsigned long game_died = 0;

    for (;;) {
        /* Compute next generation first. */
        step_game_of_life(cur, next);

        /* Analyze current vs next to detect death / stability / cycles + stats. */
        unsigned int cur_alive = 0;
        unsigned int next_alive = 0;
        int same = 1;

        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            unsigned char c = cur[i];
            unsigned char n = next[i];

            if (c) {
                cur_alive++;
            }
            if (n) {
                next_alive++;
            }
            if (c != n) {
                same = 0;
                if (!c && n) {
                    game_born++;
                } else if (c && !n) {
                    game_died++;
                }
            }
        }

        if (next_alive == 0) {
            deadcounter++;
        } else if (same) {
            samecounter++;
        } else if (next_alive == cur_alive) {
            stablecounter++;
        } else {
            deadcounter = 0;
            samecounter = 0;
            stablecounter = 0;
        }

        game_generations++;

        /*
         * Reset conditions (tuned for this 80x8 display):
         * - same pattern for a while
         * - same number of live cells for a while
         * - or just too many cycles without "interesting" change
         * Thresholds are defined at the top of this file.
         */

        if (samecounter > FREEZE_SAME_PATTERN_THRESHOLD ||
            stablecounter > FREEZE_STABLE_CELLS_THRESHOLD ||
            game_generations > FREEZE_MAX_GENERATIONS) {
            printf("[GoL] Frozen/boring game ended: generations=%lu born=%lu died=%lu\n",
                   game_generations,
                   game_born,
                   game_died);

            memset(next, 0, WIDTH * HEIGHT);

            reset_game_stats(&deadcounter,
                             &samecounter,
                             &stablecounter,
                             &game_generations,
                             &game_born,
                             &game_died);
        }

        /* If dead for multiple consecutive generations, randomize anew. */
        if (deadcounter >= DEAD_RESPAWN_THRESHOLD) {
            printf("[GoL] Dead game detected after %u checks, respawning...\n", deadcounter);

            randomize_field(next);

            reset_game_stats(&deadcounter,
                             &samecounter,
                             &stablecounter,
                             &game_generations,
                             &game_born,
                             &game_died);
        }

        /* Swap: next becomes current for rendering and next iteration. */
        unsigned char *tmp = cur;
        cur = next;
        next = tmp;

        /* Now render the (new) current field. */
        field_to_rgb565_frame(cur, frame);

        ssize_t sent = sendto(sock, frame, sizeof(frame), 0, (struct sockaddr *)&addr, sizeof(addr));
        if (sent < 0) {
            perror("sendto");
            close(sock);
            return 1;
        } else if (sent != (ssize_t)sizeof(frame)) {
            fprintf(stderr, "Partial send: %zd/%zu bytes\n", sent, sizeof(frame));
        }

        msleep(FRAME_DELAY_MS);

        /* Advance rainbow phase; full cycle is RAINBOW_PERIOD_SEC seconds. */
        rainbow_offset++;
    }

    /* Not reached in normal operation: this is an intentional infinite test sender loop.
     * Close kept for completeness in case the loop is ever refactored to exit.
     */
    close(sock);
    return 0;
}