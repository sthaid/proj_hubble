#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

double get_a(double t);
char *stars(double a);
double get_h(double t);
double get_hsi(double t);
void size_of_universe(void);
void trace_photon_fwd(void);
void trace_photon_back(void);

double k1e,k2e,k1m,k2m;
double H;

double c = 299792458;

int main()
{
    double a0,t0,a1,t1;

    #define M_PER_KM   1000.     // meters per kilometer
    #define M_PER_MPC  3.086e22  // meters per megaparsec
    #define S_PER_YR   3.156e7   // seconds per year (365.25 days)
    #define S_PER_BYR  (S_PER_YR * 1e9)
    #define LYR_PER_MPC  3.262e+6
    #define M_PER_LYR   9.461e15
    #define M_PER_BLYR   (M_PER_LYR * 1e9)

    // a = k1e + k2e * exp(H t)
    H = 65. * M_PER_KM / M_PER_MPC * S_PER_BYR;
#if 0
    t0 = 13.8;
    a0 = 1;
    t1 = 24.5;
    a1 = 200. / 92.;
    k2e = (a1 - a0) / (exp(H * t1) - exp(H * t0));
    k1e = a0 - k2e * exp(H * t0);
#else
    k2e = 1 / (exp(H * 13.8));
    k1e = 1 - k2e * exp(H * 13.8);
#endif
    //printf("k1e = %f  k2e = %f\n", k1e, k2e);

#define K98  9.8
    // a = k1m + k2m * t ^(2/3)
    t0 = .000380;      // 380,000 yrs
    a0 = 2.7 / 3000;   // .0009
    t1 = K98;   
    a1 = k1e + k2e * exp(H * t1);
    k2m = (a1 - a0) / (pow(t1, 2./3.) - pow(t0, 2./3.));
    k1m = a0 - k2m * pow(t0, 2./3.);

#if 0
    char s[100];
    double a,t,d;
    while (printf("? "), fgets(s,sizeof(s),stdin)) {
        if (sscanf(s, "%lf", &t) != 1) continue;

        a = get_a(t);

        d = 92 * a;
        printf("t = %0.6f  a = %0.6f  d = %0.6f\n", t, a, d);
    }
#endif

#if 1
    double a,h,t;
    //for (t = .00038; t < 100; t+= .010) 
    //for (t = .00038; t < 40; t+= .010) 
    //for (t = .00038; t < 37.6; t+= .010) 
    //for (t = .00038; t < 24.5; t+= .010) 
    //for (t = .00038; t < 14; t+= .010) 
    //for (t = .00038; t < 1; t+= .001) 
    //for (t = .00038; t < .100; t+= .100/1000) 
    for (t = 5; t < 37.6; t+= .010) 
    {
        a = get_a(t);
        h = get_h(t);
        //printf("%0.6f %0.6f - %s\n", t, a, stars(a));
        printf("%0.6f %0.6f %0.6f\n", t, h, a);
    }
    exit(1);
#endif
    double adot = H * k2e * exp(H * 13.8);
    adot /= S_PER_BYR;
    adot *= M_PER_MPC;
    adot /= 1000;
    printf("adot = %f\n", adot);


    // should be 65?
    printf("h_now = %f\n", get_h(13.8));
    printf("hsi_now = %e\n", get_hsi(13.8*S_PER_BYR));

    //size_of_universe();

    trace_photon_fwd();
    //trace_photon_back();

    return 0;
}

void trace_photon_fwd(void)
{
    double t, x;

    #define dt (t/1000)
    #define h (get_hsi(t))
    //#define h (get_hsi(13.8*S_PER_BYR))

    t = .00038 * S_PER_BYR;
    x = -0.037514 * M_PER_BLYR;

    //x *= 1.3225;  // converges
    //x *= 1.3300;  // diverges

    while (true) {
        printf("T = %f   X = %f\n",
            t / S_PER_BYR,  
            x / M_PER_BLYR);
        if (x >= 0) break;
        if (t / S_PER_BYR > 1000) break;
        x += (c + h * x) * dt;
        t += dt;

    }

    #undef dt
    #undef h
}

void trace_photon_back(void)
{
    double t, x;

    #define dt (t/1000)
    #define h (get_hsi(t))

    double t_start_byr = 
        13.8;  // diameter should be 92
        //24.5;  // diameter should be 200
        //49.8;    // diameter should be 800

    t = t_start_byr * S_PER_BYR;
    x = 0;

    while (true) {
        printf("T = %f   X = %f\n",
            t / S_PER_BYR,  
            x / M_PER_BLYR);

        x -= (c + h*x) * dt;

        t -= dt;

        if (t < .00038 * S_PER_BYR) break;
        //if (x >= 0) break;
    }

    double diameter;
    diameter = (-x / M_PER_BLYR)  *
               (get_a(t_start_byr) / get_a(t/S_PER_BYR)) * 
               2;
    printf("A_START = %f \n", get_a(t_start_byr));
    printf("A_END   = %f \n", get_a(t/S_PER_BYR));
    printf("DIAMETER = %f\n", diameter);


    #undef dt
    #undef h
}

void size_of_universe(void)
{
    // XXX not working
    double d;
    // calc size it time .00038
    //diameter = ((c / 1000) / h) * LYR_PER_MPC * 2;

    d = ((c / 1000) / get_h(.00038)) * LYR_PER_MPC * 2;
    d /= 1e9;
    printf("d at .00038 = %f blyr\n", d);
    printf("d at NOW    = %f blyr\n", d / get_a(.00038));

    // git diameter now by applying scale factor
}

char *stars(double a)
{
    int n;
    static char str[1000];

    n = a * 10;
    memset(str, '*', n);
    str[n] = '\0';
    return str;
}

double get_a(double t)
{
    double a;

    if (t < K98) {
        a = k1m + k2m * pow(t, 2./3.);
    } else {
        a = k1e + k2e * exp(H * t);
    }

    return a;
}

double get_h(double t)
{
    double h, a, a1;
    double dt = t / 100000;

    // use derivative
    a = get_a(t);
    a1 = get_a(t+dt);

    h = ((a1 - a) / dt) / a;
    h /= S_PER_BYR;
    h *= M_PER_MPC;
    h /= 1000;
    return h;
}

double get_hsi(double t_sec)
{
    double t_byr = t_sec / S_PER_BYR;
    double h, a, a1;
    double dt_byr = t_byr / 100000;

    // use derivative
    a = get_a(t_byr);
    a1 = get_a(t_byr+dt_byr);

    h = ((a1 - a) / dt_byr) / a;
    h /= S_PER_BYR;
    return h;
}
