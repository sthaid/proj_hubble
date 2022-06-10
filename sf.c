// I was not able to find a single equation for the scale factor in web searches.
// So, instead I used equations found on Wikipedia for the matter dominated era, and
// the dark energy dominated era:
// 
//     Era                      Start       End         Equation
//     ---                      -----       ---         --------
//     matter dominated         47,000      9.8 Byrs    a = k * t ^ (2/3)
//     dark energy dominated    9.8 Byrs    forever     a = k * exp(H * t)
// 
// And tuned these equations using the following criteria:
// - at t=13.8 (now) a=1
// - at t=.00038 (CMB) a=.0009   (2.7 deg k / 3000 deg k)
// - hubble paramter values
//       age     hubble parameter
//       9.8         100
//       10.9        90
//       12.3        80
//       14.0        70
//       16.3        60
//       19.6        50
// - transition from matter dominated to dark energy dominated occurs at 9.8 byrs
// 
// First the scale factor for the ranges 13.8 up to 19.6 and 13.8 down to 9.8 byrs 
// is calculated, in 1 million year increments, and the results saved in a table.
// Starting at t=13.8,a=1; the following is used. H(t) is from linear interpolation
// of the values in the table shown above.
//     da = H(t) * a * dt
// 
// From the preceeding we now know the scale factor at time=9.8. And we also know the scale
// factor at time=.00038 is a=.0009 from Wien's displacement law, and that the CMB was
// released at time=.00038 and at temperature=3000k. The following equation is solved
// for k1m and k2m intersecting these 2 points (the matter dominated era):
//      a = k1m + k2m * time^(2/3)
// 
// Scale factors for times > 19.6 are calculated based on a constant Hubble Parameter
// equal to 50 (H=50).
//     da/dt
//     ----- = H
//      a
// 
//      a               T
//      |        da     |
//     integral ---- = integral H dt
//      |        a      |
//     a(19.6)         19.6
// 
//     a = exp(H*(T-19.6) + ln(a(19.6)))
// 
// We now have scale factor determined for 3 time ranges:
//     1:  0.00038 to 9.8      k1m + k2m * t^(2/3)
//     2:  9.8 to 19.6         table lookup
//     3:  19.6 to infinity    exp(H*(T-19.6) + ln(a(19.6)))
// 
// When plotting this scale-factor it is evident that there is a discontinuity
// in the slope at t=9.8. To resolve this, range 1 and range 2 are smoothely connected using
// a line segment and an arc of a circle, starting at T=6.3 and ending at T=12.6.
// These time values were emperically determined to yield smooth graphs of the scale 
// factor and hubble constant vs time.
// 
// So, finally the scale factor calculated by this software is determined for these
// 4 time ranges (time is in billion years from the big bang):
//     1:  0.00038 to 6.3      k1m + k2m * t^(2/3)
//     2:  6.3 to 12.6         smooth connection of range 1 and 3
//     3:  12.6 to 19.6        table lookup
//     4:  19.6 to infinity    exp(H*(T-19.6) + ln(a(19.6)))

#include <common.h>

//
// defines
// 

#define H_TO_SI (M_PER_KM / M_PER_MPC)

#define JOIN_START   6.3  // byr
#define JOIN_END    12.6  // byr

//
// typedefs
//

typedef struct {
    double x;
    double y;
} point_t;

typedef struct {
    point_t p;
    double theta;
} line_t;

//
// variables
//

// the sf_tbl is indexed in 1 myr increments, and contains values for the
// range of indexes 9800 to 19600 (9.8 to 19.6 byr)
static double sf_tbl[20000];

// these constants are used to calculate the sf for range .000380 to 9.8 byr;
// the matter dominated era
static double k1m, k2m;

// used to enable unit testing of this file
static const bool run_unit_test = false;

//
// prototypes
//

static double sf_init_get_h(double t);
static void unit_test(void);
static void get_diameter_init(void);
static void join_init(line_t *l1, line_t *l2);
static double join_get_y(double x);
static double squared(double x);
static double distance(point_t *p1, point_t *p2);
static line_t move_line_point(line_t *l, double d);
static point_t intersect(line_t *l1, line_t *l2);
static double interpolate(double x, double x0, double x1, double y0, double y1);

// -----------------  SCALE FACTOR INIT  ------------------------------------

void sf_init(void)
{
    // init sf_tbl for time range 9.8 to 19.6 inclusive
    int tmyr;

    sf_tbl[13800] = 1;
    for (tmyr = 13801; tmyr <= 19600; tmyr++) {
        sf_tbl[tmyr] = sf_tbl[tmyr-1] + (sf_init_get_h(tmyr/1000.)*H_TO_SI) * sf_tbl[tmyr-1] * S_PER_MYR;
    }
    for (tmyr = 13799; tmyr >= 9800; tmyr--) {
        sf_tbl[tmyr] = sf_tbl[tmyr+1] - (sf_init_get_h(tmyr/1000.)*H_TO_SI) * sf_tbl[tmyr+1] * S_PER_MYR;
    }

    // init k1m and k2m, for the matter dominated range, .000380 to 9.8 byr
    double t0,t1,a0,a1;

    t0 = .000380;      // 380,000 yrs
    a0 = 2.7 / 3000;   // .0009
    t1 = 9.8;
    a1 = sf_tbl[(int)(9.8*1000)];
    k2m = (a1 - a0) / (pow(t1, 2./3.) - pow(t0, 2./3.));
    k1m = a0 - k2m * pow(t0, 2./3.);

    // init the algorithm that performs the smooth joining of the 2 ranges
    // - matter dominated (< 9.8 byr)
    // - dark energy dominates (>= 9.8 byr)
    double t, a, a_delta;
    line_t l1, l2;

    t = JOIN_START;
    a = k1m + k2m * pow(t, 2./3.);
    a_delta = k1m + k2m * pow(t+.001, 2./3.);
    l1 = (line_t){{t,a}, atan2(a_delta-a,.001)};

    t = JOIN_END;
    a = sf_tbl[(int)(JOIN_END*1000)];
    a_delta = sf_tbl[(int)(JOIN_END*1000+1)];
    l2 = (line_t){{t,a}, atan2(a_delta-a,.001)};

    join_init(&l1, &l2);

    // read the precomputed table of diameter, d_backtrack_end, and max_photon_distance
    // values from file sf.dat; or if this file doesn't yet exist create the file
    get_diameter_init();

    // init is complete, perform unit test
    if (run_unit_test) {
        unit_test();
        exit(1);
    }
}

// this routine is only used by the above sf_init routine
static double sf_init_get_h(double t)
{
    // note that the first 2 values are adjusted to compensate for the
    // smooth joining of the matter and dark energy dominated scale factors
    static struct {
        double t;
        double h;
    } tbl[] = {
        { 9.8, 130 },    // was 100
        { 10.9,115 },    // was 90
        { 12.3, 80 },
        { 14.0, 70 },
        { 16.3, 60 },
        { 19.6, 50 }, };

    if (t < 9.8 || t > 19.6) {
        printf("ERROR: t=%lf is not in tbl\n", t);
        exit(1);
    }

    for (int i = 0; i < sizeof(tbl)/sizeof(tbl[0]) - 1; i++) {
        if (t >= tbl[i].t && t <= tbl[i+1].t) {
            double y = interpolate(t, tbl[i].t, tbl[i+1].t, tbl[i].h, tbl[i+1].h);
            return y;
        }
    }

    printf("ERROR: BUG1\n");
    exit(1);
}

static void unit_test(void)
{
    printf("scale factor test\n");
    printf("  age=%10.6f sf=%6.4f sf_exp=%6.4f\n", 13.8, get_sf(13.8), 1.);
    printf("  age=%10.6f sf=%6.4f sf_exp=%6.4f\n", .000380, get_sf(.000380), .0009);

    printf("hubble parameter test\n");
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 9.8, get_h(9.8), 100.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 10.9, get_h(10.9), 90.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 12.3, get_h(12.3), 80.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 14.0, get_h(14.0), 70.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 16.3, get_h(16.3), 60.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 19.6, get_h(19.6), 50.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 30.0, get_h(30.0), 50.);
    printf("  age=%10.6f h=%5.1f h_exp=%5.1f\n", 100.0, get_h(100.0), 50.);

    printf("diameter test\n");
    printf("  age=%10.6f d=%5.1f d_exp=%5.1f\n",  13.8, get_diameter(13.8,NULL,NULL), 93.);   // wikipedia
    printf("  age=%10.6f d=%5.1f d_exp=%5.1f\n",  14.9, get_diameter(14.9,NULL,NULL), 100.);  // ask-ethan
    printf("  age=%10.6f d=%5.1f d_exp=%5.1f\n",  24.5, get_diameter(24.5,NULL,NULL), 200.);  // ask-ethan
    printf("  age=%10.6f d=%5.1f d_exp=%5.1f\n",  37.6, get_diameter(37.6,NULL,NULL), 400.);  // ask-ethan
    printf("  age=%10.6f d=%5.1f d_exp=%5.1f\n",  49.8, get_diameter(49.8,NULL,NULL), 800.);  // ask-ethan
}

// -----------------  API - GET SCALE FACTOR  -------------------------------

// returns scale factor, arg t units BYR
double get_sf(double t)
{
    double a;

    if (t >= JOIN_START && t < JOIN_END) {
        a = join_get_y(t);
    } else if (t < 9.8) {  // t >= .000380 && t < 9.8
        a = k1m + k2m * pow(t, 2./3.);
    } else if (t < 19.6) {  // t >= 9.8 && t < 19.6
        int idx;
        double t0, t1, a0, a1;
        idx = t * 1000;
        t0 = .001 * idx;
        t1 = .001 * (idx+1);
        a0 = sf_tbl[idx];
        a1 = sf_tbl[idx+1];
        a = interpolate(t, t0, t1, a0, a1);
    } else {  // t >= 19.6
        double h19_6_si, a19_6;
        h19_6_si = 50 * H_TO_SI;
        a19_6 = sf_tbl[19600];
        a = exp(h19_6_si * (t-19.6)*S_PER_BYR + log(a19_6));
    }

    return a;
}

// -----------------  API - GET HUBBLE PARAMETER  ---------------------------

// The Hubble Parameter is the value of the Hubble Constant at universe age of t.
// Units of t are BYR.
//
//                     da / dt
// Hubble Parameter = ---------
//                       a
//
// get_h return value units: km/sec/megaparsec
//
// get_h return value units: /sec

double get_h(double t)
{
    double h, a, a1;
    double dt = t / 100000;

    a = get_sf(t);
    a1 = get_sf(t+dt);

    h = ((a1 - a) / (dt*S_PER_BYR)) / a;
    h = h / H_TO_SI;

    return h;
}

double get_hsi(double t_sec)
{
    double h_si, a, a1;
    double t_byr = t_sec / S_PER_BYR;
    double dt_sec = t_sec / 100000;
    double dt_byr = dt_sec / S_PER_BYR;

    a = get_sf(t_byr);
    a1 = get_sf(t_byr + dt_byr);

    h_si = ((a1 - a) / dt_sec) / a;

    return h_si;
}

// -----------------  API - GET UNIVERSE DIAMETER  --------------------------

// This desciption assumes a hypothetical Earth that is permanently at a fixed location
// in space, over the time interval from the big bang to the end of the universe.
//
// This routine determines the distance of a photon from earth going backward in time.
// The photon has arrived at earth at 't_backtrack_start', and thus it's distance from
// earth at 't_backtrack_start' is 0.
//
// Its position is evaluated as time decrements by DELTA_T_SECS, until time becomes .000380 BLYR
// (the time that the universe became transparent and CMB was released). This final position is
// 'd_backtrack_end'.
//
// The function return value is the diameter of the universe in BLYR, at time t_backtrack_start.
// This is determined by multiplying the distance of the photon from the hypothetical 
// permanent earth at time .00038 BYR (d_backtrack_end) by how much the universe has expanded
// from time .00038 to t_backtrack_start.
// 
// Returns 
// - value: diameter of universe in BLYR
// - d_backtrack_end: distance of the photon from the permanent earh location,
//    at time .00038 BYR
// - max_photon_distance: the maximum distance of the photon from the earth during 
//    the time interval t_backtrack_start down to .00038 BYR.

typedef struct {
    double t;
    double diameter;
    double d_backtrack_end;
    double max_photon_distance;
} diameter_t;

static diameter_t *tbl;
static int max_tbl;

// The computation time it takes to backtrack a photon from time t_backtrack_start
// to time .00038 byr is significant. To eliminate program execution delays caused
// by this computation time, these values are pre-computed and saved to file sf.dat.
static void get_diameter_init(void)
{
    int rc, fd, len;
    struct stat buf;
    const char *filename = "sf.dat";

    // if sf.dat file exists then read it and return
    rc = stat(filename, &buf);
    if (rc == 0) {
        if (buf.st_size % sizeof(diameter_t)) {
            printf("ERROR: file st.dat size must be multiple of %zd\n",
                   sizeof(diameter_t));
            exit(1);
        }
        max_tbl = buf.st_size / sizeof(diameter_t);
        tbl = calloc(max_tbl, sizeof(diameter_t));
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            printf("ERROR: open %s, %s\n", filename, strerror(errno));
            exit(1);
        }
        len = read(fd, tbl, buf.st_size);
        if (len != buf.st_size) {
            printf("ERROR: read %s, %s\n", filename, strerror(errno));
            exit(1);
        }
        close(fd);

        printf("diameter tbl has been read from %s\n", filename);
        return;
    }

    // Loop over range of times and determine the diameter, d_backtrack_end and 
    // max_phton_distance for each.
    // And save these computed values to file sf.dat.

    double t, t_incr;
    double diameter, d_backtrack_end, max_photon_distance;
    int cnt=0;
    #define MAX_TBL 100000

    // create empty file sf.dat
    fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, 0644);
    if (fd < 0) {
        printf("ERROR: create %s, %s\n", filename, strerror(errno));
        exit(1);
    }

    // allocate memory to save the results, an excessive allocation (MAX_TBL) is used
    tbl = calloc(MAX_TBL, sizeof(diameter_t));
    if (tbl == NULL) {
        printf("ERROR: failed to allocate tbl\n");
        exit(1);
    }
    max_tbl = 0;

    // calculate the diameter, d_backtrack_end and max_photon_distance for 
    // range of times from .01 to 200 BYR
    printf("creating diameter table file %s ...\n", filename);
    for (t = .01; t < 200; t += t_incr) {
        diameter = get_diameter_ex(t, &d_backtrack_end, &max_photon_distance);
        printf("  %d - t=%f diameter=%f\n", ++cnt, t, diameter);

        if (max_tbl >= MAX_TBL) {
            printf("ERROR: diamter tbl is full\n");
            exit(1);
        }

        tbl[max_tbl].t = t;
        tbl[max_tbl].diameter = diameter;
        tbl[max_tbl].d_backtrack_end = d_backtrack_end;
        tbl[max_tbl].max_photon_distance = max_photon_distance;
        max_tbl++;

        t_incr = (t < 0.1 ? .001 :
                  t < 1.0 ? .01 :
                            .1);
    }

    // write tbl of results to file sf.dat
    len = write(fd, tbl, max_tbl*sizeof(diameter_t));
    if (len != max_tbl*sizeof(diameter_t)) {
        printf("ERROR: write tbl to %s, %s\n", filename, strerror(errno));
        exit(1);
    }

    // close fd, and exit program
    close(fd);
    printf("diameter table file %s has been created;\n", filename);
    printf("program must be restarted\n");
    exit(0);
}

double get_diameter(double t_backtrack_start, double *d_backtrack_end, double *max_photon_distance)
{
    static int idx;
    double diameter;

    #define MATCH (t_backtrack_start >= tbl[idx].t && t_backtrack_start <= tbl[idx+1].t)

    #define EPSILON 1e-6
    #define IS_CLOSE(a,b) \
                ({ double ratio = (a)/(b); \
                   ratio > 1-EPSILON && ratio < 1+EPSILON; })

    // search the tbl, starting from last tbl idx, to locate the idx 
    // where t_backtrack_start is in between tbl[idx] and tbl[idx+1]
    if (MATCH) {
        // idx is already okay
    } else if (t_backtrack_start > tbl[idx+1].t) {
        // search forward
        for (idx = idx+1; idx < max_tbl-1; idx++) {
            if (MATCH) break;
        }
    } else {
        // search backward
        for (idx = idx-1; idx >= 0; idx--) {
            if (MATCH) break;
        }
    }

    // if matching idx is not found then reset idx and use get_diameter_ex
    if (idx < 0 || idx >= max_tbl-1) {
        idx = 0;
        printf("WARNING: time %f not in diameter tbl\n", t_backtrack_start);
        return get_diameter_ex(t_backtrack_start, d_backtrack_end, max_photon_distance);
    }

    // sanity check that an idx was found
    if (!MATCH) {
        printf("BUG: MATCH is false, t=%f idx=%d\n", t_backtrack_start, idx);
        exit(1);
    }

    // if the time for tbl[idx] is very close to the caller's 
    // requested t_backtrack_start, then return values from tbl[idx]
    if (IS_CLOSE(t_backtrack_start, tbl[idx].t)) {
        if (d_backtrack_end) {
            *d_backtrack_end = tbl[idx].d_backtrack_end;
        }
        if (max_photon_distance) {
            *max_photon_distance = tbl[idx].max_photon_distance;
        }
        diameter = tbl[idx].diameter;
        return diameter;
    }

    // if the time for tbl[idx+1] is very close to the caller's 
    // requested t_backtrack_start, then return values from tbl[idx+1]
    if (IS_CLOSE(t_backtrack_start, tbl[idx+1].t)) {
        if (d_backtrack_end) {
            *d_backtrack_end = tbl[idx+1].d_backtrack_end;
        }
        if (max_photon_distance) {
            *max_photon_distance = tbl[idx+1].max_photon_distance;
        }
        diameter = tbl[idx+1].diameter;
        return diameter;
    }

    // determine return values by interpolating
    diameter = interpolate(t_backtrack_start,
                           tbl[idx].t, tbl[idx+1].t,
                           tbl[idx].diameter, tbl[idx+1].diameter);
    if (d_backtrack_end) {
        *d_backtrack_end = interpolate(t_backtrack_start,
                                       tbl[idx].t, tbl[idx+1].t,
                                       tbl[idx].d_backtrack_end, tbl[idx+1].d_backtrack_end);
    }
    if (max_photon_distance) {
        *max_photon_distance = interpolate(t_backtrack_start,
                                           tbl[idx].t, tbl[idx+1].t,
                                           tbl[idx].max_photon_distance, tbl[idx+1].max_photon_distance);
    }
    return diameter;
}
    
double get_diameter_ex(double t_backtrack_start, double *d_backtrack_end_arg, double *max_photon_distance)
{
    double t_si, d_photon_si, h_si;
    double d_backtrack_end, t_backtrack_end, diameter;
    double max_d_photon_si;

    // init
    t_si = t_backtrack_start * S_PER_BYR;  // secs
    d_photon_si = 0;      // meters
    max_d_photon_si = 0;  // meters

    // assume the earth is permanent ...
    // 
    // starting from caller supplied time t_backtrack_start  and with photon 
    // at distance 0 from earth, calculate the distance of the photon from earth
    // going back in time to .00038 byrs (time of the CMB that we now observe)
    while (true) {
        h_si  = get_hsi(t_si - DELTA_T_SECS);
        d_photon_si = (d_photon_si + c_si * DELTA_T_SECS) / (1 + h_si * DELTA_T_SECS);
        t_si -= DELTA_T_SECS;

        if (d_photon_si > max_d_photon_si) {
            max_d_photon_si = d_photon_si;
        }

        if (t_si < (.00038 * S_PER_BYR + DELTA_T_SECS/2)) {
            break;
        }

        if (d_photon_si < 0) {
            printf("ERROR: d_photon_si>=0: d_photon_si=%f h=%f t=%f t_backtrack_start=%f\n", 
                   d_photon_si, h_si/H_TO_SI, t_si/S_PER_BYR, t_backtrack_start);
            exit(1);
        }
    }

    // converrt the distance of the photon from earth to blyr units;
    // convert the time the backtrack ended to byr units (should be close to .00038 byr)
    d_backtrack_end = d_photon_si / M_PER_BLYR;
    t_backtrack_end = t_si / S_PER_BYR;

    // The radius of the universe now, is where that spot in space where the
    // CMB originated, is now located. And the diameter is twice radius
    diameter = d_backtrack_end *
               (get_sf(t_backtrack_start) / get_sf(t_backtrack_end)) *
               2;

#if 0
    // debug prints
    printf("t_bt_start, t_bt_end = %f %f\n", t_backtrack_start, t_backtrack_end);
    printf("a_bt_start, a_bt_end = %f %f\n", get_sf(t_backtrack_start), get_sf(t_backtrack_end));
    printf("cmb temperature      = %f\n", 2.7 / get_sf(t_backtrack_end));
    printf("d_bt_end             = %f\n", d_backtrack_end);
    printf("diameter             = %f\n", diameter);
#endif

    // return additional values to caller
    if (d_backtrack_end_arg) {
        *d_backtrack_end_arg = d_backtrack_end;
    }
    if (max_photon_distance) {
        *max_photon_distance = max_d_photon_si / M_PER_BLYR;
    }

    // return result
    return diameter;
}

// -----------------  SUPPORT - JOIN 2 LINES SMOOTHLY  -----------------------

// The 2 lines being joined are each specified as a point and an angle.
//
// These points are smoothly connected (without a discontinuity in the slope),
// by connecting with a line segment and a arc of a circle.
//
// Only some pairs of lines can be joined using this technique.
// The caller must supply 2 lines that can be joined.
//
// Join_init determines the location of the line segment and circle arc.
// Joing_get_y is the function that returns the smooth joined y value.

static double   x_start, x_end;
static point_t  circle_p;
static double   circle_r;
static point_t  p_line_start, p_line_end;

static void join_init(line_t *l1, line_t *l2)
{
    point_t p; 
    double d1, d2, delta;
    line_t l1a, l2a;

    // save x_start and x_end
    x_start = l1->p.x;
    x_end = l2->p.x;

    // p is the intersection of l1 and l2
    p = intersect(l1, l2);

    // create l1a and l2a, which advance either the l1 or l2 point
    // so that they are equidistinct to p
    d1 = distance(&l1->p, &p);
    d2 = distance(&l2->p, &p);
    delta = d1 - d2;
    if (delta > 0) {
        l1a = move_line_point(l1, delta);  // this delta is > 0
        l2a = *l2;
    } else {
        l1a = *l1;
        l2a = move_line_point(l2, delta);  // this delta is < 0
    }

    // keep track of the start and end of the interpolation line
    if (delta > 0) {
        p_line_start = l1->p;
        p_line_end = l1a.p;
    } else {
        p_line_start = l2a.p;
        p_line_end = l2->p;
    }

    // rotate the l1a and l2a lines 90 degrees so that they are
    // now oriented as a circle radius
    l1a.theta += M_PI/2;
    l2a.theta += M_PI/2;

    // find the center and radius of the circle
    circle_p = intersect(&l1a, &l2a);
    circle_r = distance(&circle_p, &l1a.p);
}

static double join_get_y(double x)
{
    double y;

    if (x < x_start || x >= x_end) {
        fprintf(stderr, "ERROR x=%f should be in range %f to %f\n", x, x_start, x_end);
        exit(1);
    }

    if (x >= p_line_start.x && x <= p_line_end.x) {
        y = interpolate(x, p_line_start.x, p_line_end.x, p_line_start.y, p_line_end.y);
    } else {
        y = circle_p.y - sqrt(squared(circle_r) - squared(x-circle_p.x));
        //y = circle_p.y + sqrt(squared(circle_r) - squared(x-circle_p.x));
    }

    return y;
}

// -----------------  SUPPORT - UTILS  ---------------------------------------

static double squared(double x)
{
    return x * x;
}

static double distance(point_t *p1, point_t *p2)
{
    return sqrt(squared(p1->x - p2->x) + squared(p1->y - p2->y));
}

static line_t move_line_point(line_t *l, double d)
{
    line_t ret;

    ret.p.x = l->p.x + d * cos(l->theta);
    ret.p.y = l->p.y + d * sin(l->theta);
    ret.theta = l->theta;
    return ret;

}

static point_t intersect(line_t *l1, line_t *l2)
{
    double x1, y1, m1, x2, y2, m2;
    double x, y;

    x1 = l1->p.x;
    y1 = l1->p.y;
    m1 = tan(l1->theta);

    x2 = l2->p.x;
    y2 = l2->p.y;
    m2 = tan(l2->theta);

    x = ((-m2*x2 + y2) - (-m1*x1+y1)) / (m1 - m2);
    y = m1 * (x - x1) + y1;

    return (point_t){x,y};
}

static double interpolate(double x, double x0, double x1, double y0, double y1)
{
    double result;

    result = y0 + (x - x0) * ((y1 - y0) / (x1 - x0));
    return result;
}

