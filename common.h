#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <util_misc.h>
#include <util_sdl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))  // xxx in misc

#define M_PER_KM     1000.               // meters per kilometer
#define M_PER_MPC    3.086e22            // meters per megaparsec
#define S_PER_YR     3.156e7             // seconds per year (365.25 days)
#define S_PER_BYR    (S_PER_YR * 1e9)
#define S_PER_MYR    (S_PER_YR * 1e6)
#define M_PER_LYR    9.461e15
#define M_PER_BLYR   (M_PER_LYR * 1e9)
#define c_si         3e8

#define DELTA_T_SECS (10000 * S_PER_YR)  // 10000 years

#define T_START      0.000380   // time when the universe became transparent
#define T_MAX        200.       // max time supported by the code in sf.c
#define TEMP_START   3000.      // temperature of CMB at T_START

#define MB           0x100000
#define FONT_SZ      24
#define NO_VALUE     -999
#define PRECISION(x) ((x) == 0 ? 0 : (x) < .001 ? 6 : (x) < 1 ? 3 : (x) < 100 ? 1 : 0)

// sf.c
void sf_init(void);
double get_sf(double t);
double get_h(double t);
double get_hsi(double t_sec);
double get_diameter(double t_backtrack_start, double *d_backtrack_end, double *max_photon_distance);
double get_diameter_ex(double t_backtrack_start, double *d_backtrack_end, double *max_photon_distance);

// graph.c
#define MAX_GRAPH_POINTS 796

typedef struct {
    bool exists;
    double max_xval;
    double max_yval;
    double x_cursor;
    char title[100];
    struct {
        double x;
        double y;
    } p[MAX_GRAPH_POINTS];
    int n;
} graph_t;

int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);

#endif
