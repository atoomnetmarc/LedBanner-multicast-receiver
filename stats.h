#ifndef STATS_H
#define STATS_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

typedef struct StatsState {
    struct timespec last_ts;
    int have_last_ts;
    size_t bytes_since_last;
} StatsState;

void print_timestamp_size_fps_kbps(ssize_t n, double fps, double kbps);
void update_stats_and_log(StatsState *stats, ssize_t n);

#endif // STATS_H