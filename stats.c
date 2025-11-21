/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#include "stats.h"

#include <stdio.h>
#include <time.h>

void print_timestamp_size_fps_kbps(ssize_t n, double fps, double averaged_fps, double kbps) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_local;
    localtime_r(&ts.tv_sec, &tm_local);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_local);

    printf("%s.%03ld: received %zd bytes, %6.2f FPS, %6.2f FPS (avg), %7.2f kB/s\n",
           buf,
           ts.tv_nsec / 1000000,
           n,
           fps,
           averaged_fps,
           kbps);
    fflush(stdout);
}

void update_stats_and_log(StatsState *stats, ssize_t n) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    stats->bytes_since_last += (size_t)n;

    double fps = 0.0;
    double averaged_fps = 0.0;
    double kbps = 0.0;

    // Store current frame timestamp
    stats->frame_timestamps[stats->frame_index] = now;
    stats->frame_count++;
    stats->frame_index = (stats->frame_index + 1) % FPS_AVERAGE_FRAMES;

    // Calculate current FPS (time since last frame)
    if (stats->have_last_ts) {
        double dt = (now.tv_sec - stats->last_ts.tv_sec) + (now.tv_nsec - stats->last_ts.tv_nsec) / 1e9;
        if (dt > 0.0) {
            fps = 1.0 / dt;
            kbps = (stats->bytes_since_last / 1024.0) / dt;
        }
    }

    // Calculate averaged FPS over the last N frames
    if (stats->frame_count >= 2) {
        int frames_to_average = (stats->frame_count < FPS_AVERAGE_FRAMES) ? stats->frame_count : FPS_AVERAGE_FRAMES;
        int start_index = (stats->frame_index - frames_to_average + FPS_AVERAGE_FRAMES) % FPS_AVERAGE_FRAMES;

        double total_time = 0.0;
        int i;

        for (i = 0; i < frames_to_average - 1; i++) {
            int idx1 = (start_index + i) % FPS_AVERAGE_FRAMES;
            int idx2 = (start_index + i + 1) % FPS_AVERAGE_FRAMES;
            total_time += (stats->frame_timestamps[idx2].tv_sec - stats->frame_timestamps[idx1].tv_sec) +
                          (stats->frame_timestamps[idx2].tv_nsec - stats->frame_timestamps[idx1].tv_nsec) / 1e9;
        }

        if (total_time > 0.0) {
            averaged_fps = (frames_to_average - 1) / total_time;
        }
    }

    stats->last_ts = now;
    stats->have_last_ts = 1;

    if (fps > 0.0) {
        print_timestamp_size_fps_kbps(n, fps, averaged_fps, kbps);
        stats->bytes_since_last = 0;
    } else {
        print_timestamp_size_fps_kbps(n, 0.0, 0.0, 0.0);
    }
}