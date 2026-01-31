#include "utils/Time.hpp"

#include <sys/time.h>

size_t now_ms()
{
    struct timeval tv;
    if (gettimeofday(&tv, 0) != 0)
        return 0;

    return static_cast<size_t>(tv.tv_sec) * 1000u
         + static_cast<size_t>(tv.tv_usec) / 1000u;
}

size_t elapsed_ms(size_t previous_ms)
{
    const size_t current_ms = now_ms();
    if (current_ms < previous_ms)
        return 0;
    return current_ms - previous_ms;
}
