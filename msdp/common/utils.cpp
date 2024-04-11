#include "utils.h"

int64_t getNowMs() {
    timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}