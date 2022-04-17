
// gcc -Wall -Werror -o sf sf.c -lm


#include <stdio.h>
#include <math.h>
#include <stdbool.h>

int main()
{

    // https://en.wikipedia.org/wiki/Scale_factor_(cosmology)
    // 
    // ERA                      FROM                TO          SCALE FACTOR
    // radiation domminated     end-of-inflation    47,000      a = K1 * t ^ (1/2)
    // matter dominated         47,000              9.8 Byrs    a = K2 * t ^ (2/3)
    // dark energy              9.8 Byrs            forever     a = K3 * exp(H * t)   
    //
    //                                                          where H = sqrt(Lambda/3) = xxxx

    // H = 70 (km/s)/Mpc
    // H = 70 * 1000 / 3.086e+22 
    //          m/km   m/Mpc
    // H = 2.27e-18  /s

    // Lambda = 2.036e-35  /s/s
    // sqrt(2.036e-35 / 3) = 2.6051232e-18   which is close to H from above, but not same
    // convert 2.605e-18 to km/s/Mpc => 2.605e-18 / 1000 * 3.086e+22 => 80.4 km/s/Mpc

    // age of universe = 13.77 Byrs

// https://www.forbes.com/sites/startswithabang/2018/04/28/ask-ethan-how-big-will-the-universe-get/?sh=76c586c41f52
// AGE OF
// UNIVERSE        DIAMETER                  SF       HUBBLE CONSTANT
// ========        ========                  ====     ==============
// 13.8             92   today               1.00     65
// 14.9            100                       1.09
// 24.5            200                       2.17     57
// 37.6            400                       4.35     55.4
// 49.8            800    doubles ever 12.2  8.70


//Universe becomes transparent. Temperature is T=3000 K, time is 380,000 years after the Big Bang. Ordinary matter can now fall into the dark matter clumps. The CMB travels freely from this time until now, so the CMB anisotropy gives a picture of the Universe at this time.

// T now is 2.7, so scale factor at age .00038 byrs is .0009; and the diameter is 
//     .0009 * 92 = .0828

// .000380
// AGE OF
// UNIVERSE        DIAMETER     SF
// ========        ========     =====
// .000380         .0828        .0009




    #define BYR2SEC(_byrs)  ((_byrs) * 31557600e9)

    const double H = 2.27e-18;

    double a2, a3, t;
    char s[100];
    double K2, K3;
    double byrs;

    K2 = .0009 / pow( BYR2SEC(.00038), 2./3);
    printf("K2 %le\n", K2);

    double a, delta_a;
    a = K2 * pow(BYR2SEC(13.77), 2./3);
    delta_a = 1 - a;
    printf("a = %le , delta_a = %lf\n", a, delta_a);

    K3 = delta_a / exp(H * BYR2SEC(13.77));
    printf("K3 %lf\n", K3);

    byrs = .00038;
    t = BYR2SEC(byrs);
    a2 = K2 * pow(t, 2./3.);
    a3 = K3 * exp(H * t);
    printf("Time = %0.9lf byrs   a2 = %0.6lf  a3 = %0.6lf  total = %0.6lf\n",
          byrs, a2, a3, a2+a3);

    byrs = 13.77;  
    t = BYR2SEC(byrs);
    a2 = K2 * pow(t, 2./3.);
    a3 = K3 * exp(H * t);
    printf("Time = %0.9lf byrs   a2 = %0.6lf  a3 = %0.6lf  total = %0.6lf\n",
          byrs, a2, a3, a2+a3);

    byrs = 24.5;  // a should be 4 ?
    t = BYR2SEC(byrs);
    a2 = K2 * pow(t, 2./3.);
    a3 = K3 * exp(H * t);
    printf("Time = %0.9lf byrs   a2 = %0.6lf  a3 = %0.6lf  total = %0.6lf\n",
          byrs, a2, a3, a2+a3);

    byrs = 37.6;  // a should be 8 ?
    t = BYR2SEC(byrs);
    a2 = K2 * pow(t, 2./3.);
    a3 = K3 * exp(H * t);
    printf("Time = %0.9lf byrs   a2 = %0.6lf  a3 = %0.6lf  total = %0.6lf\n",
          byrs, a2, a3, a2+a3);

//https://www.forbes.com/sites/startswithabang/2018/04/28/ask-ethan-how-big-will-the-universe-get/?sh=76c586c41f52

    while (true) {
        printf("byrs? ");
        if (fgets(s,sizeof(s),stdin) == NULL) break;
        if (sscanf(s, "%lf", &byrs) != 1) continue;
#if 1
        t = BYR2SEC(byrs);
        a2 = K2 * pow(t, 2./3.);
        a3 = K3 * exp(H * t);
        printf("Time = %0.9lf byrs   a2 = %0.6lf  a3 = %0.6lf  total = %0.6lf\n",
              byrs, a2, a3, a2+a3);
#else
        t = byrs;
a = 
7.209e-12 * pow(t,9) - 
1.113e-9 * pow(t,8) + 
7.29e-8 * pow(t,7) - 
2.64e-6 * pow(t,6) + 
5.776e-5 * pow(t,5) - 
0.0007852 * pow(t,4) + 
0.006647 * pow(t,3) - 
0.03411 * pow(t,2) + 
0.1594 * t + 
0.02016 ;
        printf("Time = %0.9lf byrs   a = %0.6lf\n", byrs, a);
#endif

    }

    // number of seconds in one year = 31557600
    // T47kyrs    = 1.483e+12
    // T9.8byrs   = 3.092e+17
    // T13.77byrs = 4.345e+17

    // try 1, didn't work
    //   1 = K2 * 5.737e11 + K3 * 2.681
    //   K2 * 4.573e11 = K3 * 2.018
    //
    //   K2 = K3 * 4.413e-12
    //
    //   1 = K3 * 2.532 + K3 * 2.681
    //   K3 = 1 / (2.532 + 2.681) = 0.1918
    //   K2 = 0.1918 * 4.413e-12 = 8.464e-13
    //
    //   K2 = 8.464e-13
    //   K3 = 0.1918

#if 0 // try2
    a = K2 * t ^ (2/3)
    a = K3 * exp(H * t)   

    .00009 = K2 * (380000*31557600) ^ (2/3)

    K2 = .00009 / ((380000*31557600)^.66666)
    K2 = 1.718e-13

    1 = K2 * 5.737e11 + K3 * 2.681

    K3 = 1 - (K2 * 5.737e11) / 2.681
    K3 = 1 - (1.718e-13 * 5.737e11) / 2.681
    K3 = 0.96323;

    K2 = 1.718e-13
    K3 = 0.9632
#endif

#if 0
    //const double K2 = 8.464e-13;
    //const double K3 = 0.1918;
    //const double K2 = 1.718e-13;
    //const double K3 = 0.9632;

    #define YR2SEC 31557600.0
    const double H = 2.27e-18;

    double a1, t1, c1, d1;
    double a2, t2, c2, d2;
    double K2, K3;

    a1 = .00009;
    t1 = 380000 * YR2SEC;
    c1 = pow(t1, (2./3.));
    d1 = exp(H * t1);

    a2 = 1;
    t2 = 13.77e9 * YR2SEC;
    c2 = pow(t2, (2./3.));
    d2 = exp(H * t2);
    
    K3 = (a2 - c2 * a1 / c1) / (-c2 * d1 / c1 + d2);
    K2 = (a1 - d1 * K3) / c1;

    printf("K2 = %le  K3 = %le\n", K2, K3);


    double t, a, byrs;
    char s[100];

    #define BYR2SEC(_byrs)  ((_byrs) * 31557600e9)

    while (true) {
        printf("byrs? ");
        if (fgets(s,sizeof(s),stdin) == NULL) break;
        if (sscanf(s, "%lf", &byrs) != 1) continue;
        t = BYR2SEC(byrs);
        a = K2 * pow(t, (2./3.)) + K3 * exp(H * t);

        printf("Time = %0.9lf byrs   a = %0.6lf\n", byrs, a);
    }
#endif

    return 0;
}
