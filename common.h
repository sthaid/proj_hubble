#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include <util_misc.h>

#define M_PER_KM     1000.               // meters per kilometer
#define M_PER_MPC    3.086e22            // meters per megaparsec
#define S_PER_YR     3.156e7             // seconds per year (365.25 days)
#define S_PER_BYR    (S_PER_YR * 1e9)
#define S_PER_MYR    (S_PER_YR * 1e6)
#define M_PER_LYR    9.461e15
#define M_PER_BLYR   (M_PER_LYR * 1e9)

#define c_si 3e8

#define DELTA_T_SECS (10000 * S_PER_YR)  // 10000 years


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))  // xxx in misc

// sf.c
void sf_init(void);
double get_sf(double t);
double get_h(double t);
double get_hsi(double t_sec);
double get_diameter(double t_backtrack_start, double *d_backtrack_end);

#endif
