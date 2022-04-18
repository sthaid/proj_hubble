#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "common.h"

#define DEG2RAD(d)   ((d) * (M_PI / 180))

line_t l1 = { {10, 10},  DEG2RAD(5)  };
line_t l2 = { {80, 40}, DEG2RAD(45) };
//line_t l1 = { {10, 10},  DEG2RAD(5)  };
//line_t l2 = { {20, 40}, DEG2RAD(80) };

double squared(double x);
point_t intersect(line_t *l1, line_t *l2);
double distance(point_t *p1, point_t *p2);
line_t move_line_point(line_t *l, double d);
double interpolate(double x, double x0, double x1, double y0, double y1);

#if 0
// -----------------  MAIN  ----------------------------------

int main(int argc, char **argv)
{
    init(&l1, &l2);

    for (double x = l1.p.x; x < l2.p.x; x += .01) {
        printf("%f %f\n", x, get(x));
    }

    return 0;
}
#endif

// -----------------  SMOOTH ATTACH CURVES  ------------------

double x_start, x_end;
point_t circle_p;
double circle_r;
point_t p_line_start, p_line_end;

double get(double x)
{
    double y;

    if (x < x_start || x >= x_end) {
        fprintf(stderr, "ERROR x=%f should be in range %f to %f\n", x, x_start, x_end);
        exit(1);
    }

    if (x >= p_line_start.x && x <= p_line_end.x) {
        y = interpolate(x, p_line_start.x, p_line_end.x, p_line_start.y, p_line_end.y);
    } else {
        y = circle_p.y - sqrt(squared(circle_r) - squared(x-circle_p.x));   // xxx + or -
        //y = circle_p.y + sqrt(squared(circle_r) - squared(x-circle_p.x));   // xxx + or -
    }

    return y;
}

void init(line_t *l1, line_t *l2)
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

// -----------------  UTILS  -------------------

double squared(double x)
{
    return x * x;
}

double distance(point_t *p1, point_t *p2)
{
    return sqrt(squared(p1->x - p2->x) + squared(p1->y - p2->y));
}

line_t move_line_point(line_t *l, double d)
{
    line_t ret;

    ret.p.x = l->p.x + d * cos(l->theta);
    ret.p.y = l->p.y + d * sin(l->theta);
    ret.theta = l->theta;
    return ret;

}

point_t intersect(line_t *l1, line_t *l2)
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

double interpolate(double x, double x0, double x1, double y0, double y1)
{
    double result;

    result = y0 + (x - x0) * ((y1 - y0) / (x1 - x0));
    return result;
}

