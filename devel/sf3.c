#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "common.h"

#define M_PER_KM   1000.               // meters per kilometer
#define M_PER_MPC  3.086e22            // meters per megaparsec
#define S_PER_YR   3.156e7             // seconds per year (365.25 days)
#define S_PER_BYR  (S_PER_YR * 1e9)
#define S_PER_MYR  (S_PER_YR * 1e6)
//#define LYR_PER_MPC  3.262e+6
//#define M_PER_LYR   9.461e15
//#define M_PER_BLYR   (M_PER_LYR * 1e9)

#define H_TO_SI (M_PER_KM / M_PER_MPC)

double get_h(double t);

double get_sf(double t);
void init_sf(void);
static double interpolate(double x, double x0, double x1, double y0, double y1);

// -----------------  MAIN --------------------------------------------------

int main(int argc, char **argv)
{
    //printf("H_TO_SI  %e\n", H_TO_SI);

    init_sf();

    static struct {
        double t;
        double h;
    } tbl[] = {
        { 9.8, 100 }, 
        { 10.9, 90 },
        { 12.3, 80 },
        { 14.0, 70 },
        { 16.3, 60 },
        { 19.6, 50 }, };
    for (int i = 0; i < 6; i++) {
        fprintf(stderr, "# t=%f  h=%f  h_exp=%f\n", 
            tbl[i].t, get_h(tbl[i].t), tbl[i].h);
    }

#if 1
    for (double t = .00038; t < 20; t += .001) {
        printf("%12.6f %12.6f\n", t, get_sf(t));
    }
#else
    for (double t = 2; t < 20; t += .001) {
        printf("%12.6f %12.6f\n", t, get_h(t));
    }
#endif

    return 0;
}

// -----------------  HUBBLE CONSTANT  --------------------------------------

double get_h(double t)
{
    double h, a, a1;
    double dt = t / 100000;

    a = get_sf(t);
    a1 = get_sf(t+dt);

    h = ((a1 - a) / dt) / a;
    h /= S_PER_BYR;
    h *= M_PER_MPC;
    h /= 1000;
    return h;
}

// -----------------  SCALE FACTOR  -----------------------------------------

double sf_tbl[20000];
double k1m, k2m;

double get_h_for_sf_init(double t);

#if 0
//#define GET_START   8.0  //okay
//#define GET_END    11.6
//#define GET_START   7.0  //okay
//#define GET_END    12.6
//#define GET_START   6.5  //okay  ??
//#define GET_END    13.0
//#define GET_START   6.0  //okay   GOOD
//#define GET_END    12.6
//#define GET_START   6.0  //okay   GOOD  <===========
//#define GET_END    12.6
#endif
#define GET_START   6.3  //okay   GOOD
#define GET_END    12.6

// t in byrs
double get_sf(double t)
{
    double a;

    if (t >= GET_START && t < GET_END) {
        a = get(t);
    } else if (t < 9.8) {  // t >= .000380 && t < 9.8
        a = k1m + k2m * pow(t, 2./3.);
    } else if (t < 19.6) {  // t >= 9.8 && t < 19.6
        int idx;
        double t0, t1, a0, a1;
        idx = t * 1000;
        t0 = .001 * idx;
        t1 = t0 + .001;
        a0 = sf_tbl[idx];
        a1 = sf_tbl[idx+1];
        a = interpolate(t, t0, t1, a0, a1);
    } else {  // t >= 19.6
        double h19_6, a19_6;
        h19_6 = 50 * H_TO_SI;
        a19_6 = sf_tbl[19600];
        a = exp(h19_6 * (t-19.6)*S_PER_BYR + log(a19_6));
    }
    return a;
}

void init_sf(void)
{
    int t;

    // init sf_tbl for time range 9.8 to 19.6 inclusive
    sf_tbl[13800] = 1;
    for (t = 13801; t <= 19600; t++) {
        sf_tbl[t] = sf_tbl[t-1] + (get_h_for_sf_init(t/1000.)*H_TO_SI) * sf_tbl[t-1] * S_PER_MYR;
    }
    for (t = 13799; t >= 5800; t--) {
        sf_tbl[t] = sf_tbl[t+1] - (get_h_for_sf_init(t/1000.)*H_TO_SI) * sf_tbl[t+1] * S_PER_MYR;
    }

    // init k1m and k2m
    double t0,t1,a0,a1;
    t0 = .000380;      // 380,000 yrs
    a0 = 2.7 / 3000;   // .0009
    t1 = 9.8;
    a1 = sf_tbl[(int)(9.8*1000)];
    k2m = (a1 - a0) / (pow(t1, 2./3.) - pow(t0, 2./3.));
    k1m = a0 - k2m * pow(t0, 2./3.);

    {
    double t, a, a1;
    line_t l1, l2;

    t = GET_START;
    a = k1m + k2m * pow(t, 2./3.);
    a1 = k1m + k2m * pow(t+.001, 2./3.);
    l1.p.x = t;
    l1.p.y = a;
    l1.theta = atan2(a1-a, .001);

    t = GET_END;
    a = sf_tbl[(int)(GET_END*1000)];
    a1 = sf_tbl[(int)(GET_END*1000+1)];
    l2.p.x = t;
    l2.p.y = a;
    l2.theta = atan2(a1-a, .001);

    init(&l1, &l2);
    }
}

double get_h_for_sf_init(double t)
{
    static struct {
        double t;
        double h;
    } tbl[] = {
        { 9.8, 150 },    // 100
        { 10.9,115 },    //  90
        { 12.3, 80 },
        { 14.0, 70 },
        { 16.3, 60 },
        { 19.6, 50 }, };

    if (t < 9.8) return 100;

    for (int i = 0; i < sizeof(tbl)/sizeof(tbl[0]) - 1; i++) {
        if (t >= tbl[i].t && t <= tbl[i+1].t) {
            double y = interpolate(t, tbl[i].t, tbl[i+1].t, tbl[i].h, tbl[i+1].h);
            return y;
        }
    }

    printf("ERROR t=%lf is not in tbl\n", t);
    exit(1);
}

// -----------------  UTILS  ------------------------------------------------

static double interpolate(double x, double x0, double x1, double y0, double y1)
{
    double result;

    if ((x >= x0 && x <= x1) || (x >= x1 && x <= x0)) {
        // okay
    } else {
        printf("ERROR interpolate x=%lf x0=%lf x1=%lf\n", x, x0, x1);
        exit(1);
    }

    result = y0 + (x - x0) * ((y1 - y0) / (x1 - x0));
    return result;
}
