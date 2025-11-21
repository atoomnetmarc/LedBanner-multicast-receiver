/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#ifndef CONFIG_H
#define CONFIG_H

#define WIDTH  80
#define HEIGHT 8

#define FPS_AVERAGE_FRAMES 42

#define MC_GROUP         "239.0.0.1"
#define MC_PORT          1565
#define MC_EXPECTED_SIZE (WIDTH * HEIGHT * 2)

typedef struct AppConfig {
    const char *title;
    int width;
    int height;
    int scale;
    const char *mc_group;
    int mc_port;
} AppConfig;

#define DEFAULT_APPCONFIG          \
    {                              \
        .title = "80x8 LedBanner", \
        .width = WIDTH,            \
        .height = HEIGHT,          \
        .scale = 8,                \
        .mc_group = MC_GROUP,      \
        .mc_port = MC_PORT,        \
    }

#endif // CONFIG_H