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
// in the slope at t=9.8. To resolve this, range 1 and range 2 are connected using
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
// 

//
// defines
// 

#define M_PER_KM     1000.               // meters per kilometer
#define M_PER_MPC    3.086e22            // meters per megaparsec
#define S_PER_YR     3.156e7             // seconds per year (365.25 days)
#define S_PER_BYR    (S_PER_YR * 1e9)
#define S_PER_MYR    (S_PER_YR * 1e6)
#define M_PER_LYR    9.461e15
#define M_PER_BLYR   (M_PER_LYR * 1e9)

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

//
// prototypes
//

static double sf_init_get_h(double t);
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
    for (t = 13799; t >= 9800; t--) {
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

    // unit test
    // xxx
}

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
        printf("ERROR t=%lf is not in tbl\n", t);
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

// -----------------  SCALE FACTOR API  -------------------------------------

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

// xxx redo to use h2si
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

// -----------------  JOIN 2 LINES SMOOTHLY  --------------------------------

// xxx description,
// xxx and comments throughout

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

// -----------------  UTILS  ------------------------------------------------

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

