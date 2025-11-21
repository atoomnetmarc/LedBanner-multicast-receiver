/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#ifndef STATS_H
#define STATS_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#define FPS_AVERAGE_FRAMES 42

typedef struct StatsState {
    struct timespec last_ts;
    int have_last_ts;
    size_t bytes_since_last;
    struct timespec frame_timestamps[FPS_AVERAGE_FRAMES];
    int frame_count;
    int frame_index;
} StatsState;

void print_timestamp_size_fps_kbps(ssize_t n, double fps, double averaged_fps, double kbps);
void update_stats_and_log(StatsState *stats, ssize_t n);

#endif // STATS_H