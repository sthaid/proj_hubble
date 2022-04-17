#include <stdio.h>
#include <math.h>

typedef struct {
    double x;
    double y;
    double m;
} point_t;

point_t start = { 10, 10, .01 };
point_t end = { 20, 15, 99 };

void init_curve(point_t *start, point_t *end);

// ---------------------------------------------
    
int main(int argc, char **argv)
{
    init_curve(&start, &end);

    return 0;
}

void init_curve(point_t *start, point_t *end)
{
    // fit this function to smoothly connect start and end
    //
    // x1,y1,m1 = start
    // x2,y2,m2 = end  
    //
    // y = a + b * (x - c)^n
    // m = n * b * (x - c)^(n-1)
    // 
    // perform the following for range of values of n, 
    // and check for match at the end point ...
    //
    // divide the 2 equations for slope and solve for c
    //          x1 - k * x2
    //    c = ----------------
    //           1 - k
    //    k = exp(log(m1 / m2) * (n -1))
    //
    // using the equation for slope, and the start point, calculate b
    //              m1
    //    b = -------------------
    //        n * (x1 - c)^(n-1)
    //
    // and calculate a from the start point
    //    a = y1 - b * (x1 - c)^n

    double a, b, c, n, k;
    double x1, y1, m1, x2, y2, m2;

    x1 = start->x;
    y1 = start->y;
    m1 = start->m;

    x2 = end->x;
    y2 = end->y;
    m2 = end->m;

    for (n = .1; n < 3; n += .1) {

    k = exp(log(m1 / m2) * (n -1));
    c = (x1 - k * x2) / (1 - k);
    if (x1 - c <= 0) continue;
    b = m1 / (n * pow(x1 - c, n - 1));
    a = y1 - b * pow(x1 - c, n);

    double y1calc, m1calc, y2calc, m2calc;
    y1calc = a + b * pow(x1 - c, n);
    m1calc = n * b * pow(x1 - c, n - 1);
    y2calc = a + b * pow(x2 - c, n);
    m2calc = n * b * pow(x2 - c, n - 1);

    double y1a, y1b, slope;
    y1a = a + b * pow(x1 - c, n);
    y1b = a + b * pow(x1+.001 - c, n);
    slope = (y1b-y1a)/.001;
    printf("SLOPE  %f\n", slope);


    printf("n=%f - k a b c = %f %f %f %f\n", n, k, a, b, c);
    printf("start x,y,m = %f %f %f   calc y,m = %f %f\n", x1, y1, m1, y1calc, m1calc);
    printf("end   x,y,m = %f %f %f   calc y,m = %f %f\n", x2, y2, m2, y2calc, m2calc);
    printf("\n");
    }
}
