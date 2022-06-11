#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "util_misc.h"

// -----------------  LOGMSG  --------------------------------------------

bool debug_enabled = false;

void logmsg(char *lvl, const char *func, char *fmt, ...) 
{
    va_list ap;
    char    msg[1000];
    int     len;

    // construct msg
    va_start(ap, fmt);
    len = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // log to stderr 
    fprintf(stderr, "%s %s: %s\n", lvl, func, msg);
}

// -----------------  TIME UTILS  -----------------------------------------

uint64_t microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
}

uint64_t get_real_time_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
}

char * time2str(char * str, int64_t us, bool gmt, bool display_ms, bool display_date) 
{
    struct tm tm;
    time_t secs;
    int32_t cnt;
    char * s = str;

    secs = us / 1000000;

    if (gmt) {
        gmtime_r(&secs, &tm);
    } else {
        localtime_r(&secs, &tm);
    }

    if (display_date) {
        cnt = sprintf(s, "%2.2d/%2.2d/%2.2d ",
                         tm.tm_mon+1, tm.tm_mday, tm.tm_year%100);
        s += cnt;
    }

    cnt = sprintf(s, "%2.2d:%2.2d:%2.2d",
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    s += cnt;

    if (display_ms) {
        cnt = sprintf(s, ".%3.3d", (int)((us % 1000000) / 1000));
        s += cnt;
    }

    if (gmt) {
        strcpy(s, " GMT");
    }

    return str;
}

// -----------------  RANDOM NUMBERS  -------------------------------------

// return uniformly distributed random numbers in range min to max inclusive
double random_range(double min, double max)
{
    return ((double)random() / RAND_MAX) * (max - min) + min;
}

// return triangular distributed random numbers in range min to max inclusive
// Refer to:
// - http://en.wikipedia.org/wiki/Triangular_distribution
// - http://stackoverflow.com/questions/3510475/generate-random-numbers-according-to-distributions
double random_triangular(double min, double max)
{
    double range = max - min;
    double range_squared_div_2 = range * range / 2;
    double U = (double)random() / RAND_MAX;   // 0 - 1 uniform

    if (U <= 0.5) {
        return min + sqrt(U * range_squared_div_2);
    } else {
        return max - sqrt((1.f - U) * range_squared_div_2);
    }
}

