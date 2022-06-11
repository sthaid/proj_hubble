#include <common.h>

#include <sys/queue.h>
void galaxy_init(void);

int main(int argc, char **argv)
{
    // init
    //sf_init();


    galaxy_init();
    return 0;



    display_init(false);

    // xxx timing test
    unsigned long start=microsec_timer();
    int i;
    double diameter;
    for (i = 0; i < 1000; i++) {
        diameter = get_diameter(13.8);
    }
    printf("%f\n", diameter);
    printf("%f secs\n",
       (microsec_timer() - start) / 1000000.);

    // runtime
    display_hndlr();
}

#if 0
galaxy_t
- dist             at sf 1
- az and el
- creation_time
- link

define MAX_GALAXY   1e6
MAX_GALAXY_XY    100
MAX_GALAXY_Z     .873     MAX_GALAXY_XY * tan(0.5)

create the galaxy and sort them by distance, so we only need to loop over the first to display them

create linked list for each 1 degree telescope direction
- these will be sorted by distance

display all galaxys
- and just for telescoep azimuth 0

axiumth to idx
  az in range 0 to 359.99
  .5 to 1.5  => 1
  (floor)(az + .5)
  if (idx == 360) idx = 0

-.5 to .5 => 0

add .5 and then floor


-179.5 => -180




#endif
#define MAX_GALAXY    10000000
#define MAX_GALAXY_DIST    100.
//#define MAX_GALAXY_Z     8.73     // MAX_GALAXY_DIST * tan(0.5)
#define MAX_GALAXY_Z     1.

#define DEG_PER_RAD  (180. / M_PI)

struct galaxy_s {
    double x;      // units
    double y;
    double z;
    double dist;   // distance at sf=1, units blyr
    double az;     // azimuth and elevation, units degrees
    double el;
    double creation_time;  // units byr
    TAILQ_ENTRY(galaxy_s) entries;
};

struct galaxy_s *galaxy;

TAILQ_HEAD(tailhead, galaxy_s);

struct tailhead th[360];

// return uniformly distributed random numbers in range min to max inclusive
double random_range(double min, double max)
{
    return ((double)random() / RAND_MAX) * (max - min) + min;
}

double squared(double v)
{
    return v * v;
}

int compare(const void *a_arg, const void *b_arg);

#define MB 0x100000L

void galaxy_init(void)
{
    int i;
    double dist, x, y, z;

    printf("sizeof galaxy_s %ld\n", sizeof(struct galaxy_s));
    printf("MB needed  %ld\n", MAX_GALAXY * sizeof(struct galaxy_s) / MB);
    galaxy = calloc(MAX_GALAXY, sizeof(struct galaxy_s));
    printf("galaxy = %p\n", galaxy);
    if (galaxy == NULL) exit(1);

    for (i = 0; i < MAX_GALAXY; i++) {
        struct galaxy_s *g = &galaxy[i];

        // choose random location for galaxy xxx minimum distance?
        while (true) {
            x = random_range(-MAX_GALAXY_DIST, MAX_GALAXY_DIST);
            y = random_range(-MAX_GALAXY_DIST, MAX_GALAXY_DIST);
            z = random_range(-MAX_GALAXY_Z, MAX_GALAXY_Z);
            dist = sqrt( squared(x) + squared(y) + squared(z) );
            if (dist < MAX_GALAXY_DIST) {
                break;
            }
        }

        // xxxxxx
        g->x = x;
        g->y = y;
        g->z = z;
        g->dist = dist;
        g->az = atan2(g->y, g->x) * DEG_PER_RAD;
        if (g->az < 0) g->az += 360;
        g->el = atan2(g->z, g->dist) * DEG_PER_RAD;

        // init creation time, 0.5 to 1.5 byr after big bang
        g->creation_time = random_range(0.5, 1.5);
    }

    // sort the galaxies by distance
    qsort(galaxy, MAX_GALAXY, sizeof(struct galaxy_s), compare);

#if 0
    for (i = 0; i < MAX_GALAXY; i++) {
        struct galaxy_s *g = &galaxy[i];
        printf("%10.3f %10.3f %10.3f - %10.3f %10.3f %10.3f %10.3f\n",
               g->x, g->y, g->z,
               g->dist, g->az, g->el, g->creation_time);

    }
#endif

    for (i = 0; i < 360; i++) {
        TAILQ_INIT(&th[i]);
    }

    //static int hist[360];

    // create lists of galaxies based on azimuth;
    // each list containing 1 degree of azimuth
    for (i = 0; i < MAX_GALAXY; i++) {
        struct galaxy_s *g = &galaxy[i];
        int idx;
        idx = floor(g->az);
        if (idx < 0 || idx >= 360) {
            printf("ERROR idx %d\n", idx);
            exit(1);
        }
        //hist[idx]++;

        double azbase = idx+0.5;
        double azel = sqrt( squared(g->az - azbase) + squared(g->el) );
        //printf("xxxxxx %f - %f + %f\n", azel, g->az-azbase, g->el);
        if (azel <= 0.5) 
            TAILQ_INSERT_TAIL(&th[idx], g, entries);
    }

    //for (i = 0; i < 360; i++) {
        //printf("HIST[%d] = %d\n", i, hist[i]);
    //}

    int cnt=0;
    struct galaxy_s *g;
    TAILQ_FOREACH(g, &th[0], entries) {
        printf("%10.3f %10.3f %10.3f - %10.3f %10.3f %10.3f %10.3f\n",
               g->x, g->y, g->z,
               g->dist, g->az, g->el, g->creation_time);
        cnt++;
    }
    printf("cnt = %d\n", cnt);

}

int compare(const void *a_arg, const void *b_arg)
{
    const struct galaxy_s *a = a_arg;
    const struct galaxy_s *b = b_arg;

    if (a->dist > b->dist)
        return 1;
    else if (a->dist < b->dist)
        return -1;
    else 
        return 0;
}

