
typedef struct {
    double x;
    double y;
} point_t;

typedef struct {
    point_t p;
    double theta;
} line_t;

void init(line_t *l1, line_t *l2);
double get(double x);

