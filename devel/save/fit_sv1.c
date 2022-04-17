#include <stdio.h>
#include <math.h>
#include <stdlib.h>

typedef struct {
    double x;
    double y;
} point_t;

typedef struct {
    point_t p;
    double theta;
} line_t;

#define DEG2RAD(d)   ((d) * (M_PI / 180))
//line_t l1 = { {10, 10},  DEG2RAD(5)  };
//line_t l2 = { {80, 40}, DEG2RAD(89.9999) };
line_t l1 = { {10, 10},  DEG2RAD(5)  };
line_t l2 = { {80, 40}, DEG2RAD(75) };
//line_t l1 = { {10, 10},  DEG2RAD(25)  };
//line_t l2 = { {80, 40}, DEG2RAD(75) };

double squared(double x);
point_t intersect(line_t *l1, line_t *l2);
double distance(point_t *p1, point_t *p2);
line_t advance_line(line_t *l, double d);
double interpolate(double x, double x0, double x1, double y0, double y1);

// ---------------------------------------------
    
int main(int argc, char **argv)
{
    point_t p;
    double d1, d2, delta;
    line_t l1a, l2a;

    p = intersect(&l1, &l2);
    fprintf(stderr,"# p %f %f\n", p.x, p.y);

    d1 = distance(&l1.p, &p);
    d2 = distance(&l2.p, &p);
    fprintf(stderr,"# d1,d2 %f %f\n", d1, d2);

    delta = d1 - d2;
    fprintf(stderr,"# delta %f\n", delta);
    if (delta > 0) {
        l1a = advance_line(&l1, delta);
        l2a = l2;
    } else {
        l1a = l1;
        l2a = advance_line(&l2, delta);
    }

    fprintf(stderr,"# l1a %f %f\n", l1a.p.x, l1a.p.y);
    fprintf(stderr,"# l2a %f %f\n", l2a.p.x, l2a.p.y);

    l1a.theta += M_PI/2;
    l2a.theta -= M_PI/2;
    p =  intersect(&l1a, &l2a);
    fprintf(stderr,"# p circle center = %f %f\n", p.x, p.y);

    double r = distance(&p, &l1a.p);
    fprintf(stderr,"# r %f\n", r);

    double x,y;
    double xc = p.x;
    double yc = p.y;
    for (x = l1.p.x; x < l2.p.x; x += .01) {
        if (x < l1a.p.x) {
            y = interpolate(x, l1.p.x, l1a.p.x, l1.p.y, l1a.p.y);
        } else if (x >= l2a.p.x) {
            y = interpolate(x, l2a.p.x, l2.p.x, l2a.p.y, l2.p.y);
        } else {
            y = yc - sqrt(squared(r) - squared(x-xc));
            //y = yc + sqrt(squared(r) - squared(x-xc));
        }
        printf("%f %f\n", x, y);
    }

    
    return 0;
}

// ---------------------------------------------

line_t advance_line(line_t *l, double d)
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

double squared(double x)
{
    return x * x;
}

double distance(point_t *p1, point_t *p2)
{
    return sqrt(squared(p1->x - p2->x) + squared(p1->y - p2->y));
}

double interpolate(double x, double x0, double x1, double y0, double y1)
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

