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
#define M_PER_LYR   9.461e15
#define M_PER_BLYR   (M_PER_LYR * 1e9)

#define H_TO_SI (M_PER_KM / M_PER_MPC)

    const double c = 3e8;

double get_h(double t);
double get_hsi(double t);

double get_sf(double t);
void init_sf(void);
static double interpolate(double x, double x0, double x1, double y0, double y1);

void trace_photon_back(void);
void trace_photon_fwd(void);

// -----------------  MAIN --------------------------------------------------

int main(int argc, char **argv)
{
    //printf("H_TO_SI  %e\n", H_TO_SI);
    double tmax;

    if (argc < 2) {
        printf("ERROR tmax expected\n");
        return 1;
    }
    if (sscanf(argv[1], "%lf", &tmax) != 1) {
        printf("ERROR tmax invalid '%s'\n", argv[1]);
        return 1;
    }
    if (tmax > 1000) {
        printf("ERROR tmax invalid %f\n", tmax);
        return 1;
    }
        

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

    { double h, hsi;
    h = get_h(13.8);
    printf("h = %f  %e\n", h, h*H_TO_SI);
    hsi = get_hsi(13.8);
    printf("hsi = %e\n", hsi);
    }

    trace_photon_back();
    trace_photon_fwd();

    FILE *fpsf = fopen("filesf", "w");
    for (double t = .00038; t < tmax; t += .001) {
        fprintf(fpsf, "%12.6f %12.6f\n", t, get_sf(t));
    }
    fclose(fpsf);

    FILE *fph = fopen("fileh", "w");
    for (double t = 1; t < tmax; t += .001) {
        fprintf(fph, "%12.6f %12.6f\n", t, get_h(t));
    }
    fclose(fph);

    FILE *fphorizon = fopen("filehorizon", "w");
    for (double t = .00038; t < tmax; t += .001) {
        double horizon = c / (get_h(t) * H_TO_SI);
        horizon /= M_PER_BLYR;
        fprintf(fphorizon, "%12.6f %12.6f\n", t, horizon);
    }
    fclose(fphorizon);

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

double get_hsi(double t)
{
    return get_h(t) * H_TO_SI;
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

//#define USE_FIT

// t in byrs
double get_sf(double t)
{
    double a;

#ifdef USE_FIT
    if (t >= GET_START && t < GET_END) {
        a = get(t);
    } else 
#endif
    if (t < 9.8) {  // t >= .000380 && t < 9.8
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
    for (t = 13799; t >= 9800; t--) {
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
#ifdef USE_FIT
        { 9.8, 130 },    // 100
        { 10.9,115 },    //  90
#else
        { 9.8, 100 },    // 100
        { 10.9,90 },    //  90
#endif
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

#if 0
    if ((x >= x0 && x <= x1) || (x >= x1 && x <= x0)) {
        // okay
    } else {
        printf("ERROR interpolate x=%lf x0=%lf x1=%lf\n", x, x0, x1);
        exit(1);
    }
#endif

    result = y0 + (x - x0) * ((y1 - y0) / (x1 - x0));
    return result;
}

// xxxxxxxxxxxxxx

double tback, xback;
void trace_photon_back(void)
{
    double t, x;

    #define dt (t/1000)
    #define h (get_hsi(t/S_PER_BYR))

    double t_start_byr =
        13.8;  // diameter should be 92
        //24.5;  // diameter should be 200
        //49.8;    // diameter should be 800

    t = t_start_byr * S_PER_BYR;
    x = 0;

    FILE *fpdistback = fopen("filedistback", "w");
    while (true) {
        fprintf(fpdistback, "%f   %f\n",
            t / S_PER_BYR,
            x / M_PER_BLYR);

        x -= (c + h*x) * dt;

        t -= dt;

        if (t < .00038 * S_PER_BYR) break;
        if (x >= 0) break;  // ?
    }
    fclose(fpdistback);

    tback = t;
    xback = x;

    double diameter;
    diameter = (-x / M_PER_BLYR)  *
               (get_sf(t_start_byr) / get_sf(t/S_PER_BYR)) *
               2;
    printf("A_START = %f \n", get_sf(t_start_byr));
    printf("A_END   = %f \n", get_sf(t/S_PER_BYR));
    printf("DIAMETER = %f\n", diameter);
    printf("TEMPERATUS = %f\n", 2.7 / get_sf(t/S_PER_BYR));


    #undef dt
    #undef h
}


void trace_photon_fwd(void)
{
    double t, x;

    #define dt (t/1000)
    #define h (get_hsi(t/S_PER_BYR))

    //t = .00038 * S_PER_BYR;
    //x = -0.042349 * M_PER_BLYR;
    t = tback;
    x = xback;

    FILE *fpdistfwd = fopen("filedistfwd", "w");
    while (true) {
        fprintf(fpdistfwd, "%f   %f\n",
            t / S_PER_BYR,
            x / M_PER_BLYR);
        if (t / S_PER_BYR > 13.8) break;
        if (x >= 0) break;
        x += (c + h * x) * dt;
        t += dt;
    }
    fclose(fpdistfwd);

    #undef dt
    #undef h
}

