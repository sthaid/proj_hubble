// xxx  todo
// - check all prints
// - graphs
//   - hubble law
//   - diameter
// - lock main pane to front
// - limit on repeat key
// - add cmd to reset display width

// - use defines for
//    13.8
//    .00038
//   100
//   0.75
//   1.25

#include <common.h>
#include <util_sdl.h>

//
// defines
//

#define MB 0x100000
#define FONT_SZ 24

#define MAX_GALAXY 1000000
#define MAX_GRAPH_POINTS 796

#define NO_VALUE -999

//
// typdedefs
// 

typedef struct {
    float x;
    float y;
    float d;
    float t_create;
    float t_visible;
} galaxy_t;

typedef struct {
    bool exists;
    char *x_str;
    char *y_str;
    double max_xval;
    double max_yval;
    double x_cursor;
    double y_cursor;  // XXX title
    struct {
        double x;
        double y;
    } p[MAX_GRAPH_POINTS];
    int n;
} graph_t;

typedef struct {
    struct {
        double d_mpc;
        double v_kmps;
        double z;
        double h;
    } g[1000];
    int n;
    int idx;
} hubble_law_t;

//
// variables
//

static int          win_width, win_height;

static galaxy_t    *galaxy;
static double       t;
static int          num_visible;
static hubble_law_t hl;

static graph_t      graph[4];

static double       disp_width;
static bool         time_run; // xxx other mode flags here

//
// prototypes
//

static void galaxy_sim_init(void);
static void * time_run_thread(void *cx);
static void *sim_thread(void *cx);
static void sim_visible(double t_sim);

static void display_init(void);
static void display_hndlr(void);
static void display_start(void *cx);
static int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
static int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);

static double squared(double x);
static double get_temperature(double t);
static int qsort_compare(const void *a, const void *b);

// -----------------  MAIN  ---------------------------------------
  
int main(int argc, char **argv)
{
    // init
    sf_init();
    galaxy_sim_init();
    display_init();

    // runtime
    display_hndlr();
}

// -----------------  GALAXY SIM ----------------------------------

static void galaxy_sim_init(void)
{
    pthread_t tid;
    size_t size;
    int i;
    graph_t *g;

    // allocate memory for the galaxy array
    // xxx dont need to malloc?
    size = MAX_GALAXY * sizeof(galaxy_t);
    galaxy = malloc(size);
    if (galaxy == NULL) {
        printf("ERROR: failed to allocate memory for galaxy, size needed %zd MB\n", size/MB);
        exit(1);
    }

    // initialize the galaxy array, the 2 dimension position of the galaxies is 
    // randomly chosen in a box that is 2000 BLYR x 2000 BLYR
    for (i = 0; i < MAX_GALAXY; i++) {
        galaxy_t *g = &galaxy[i];
        if (i < 7) {
            g->x = (30 * (i + 1)) * (M_PER_MPC / M_PER_BLYR);
            g->y = 0;
            g->t_create = 1;
        } else {
            g->x = random_range(-1000, 1000);
            g->y = random_range(-1000, 1000);
            g->t_create = random_triangular(0.75, 1.25);
        }
        g->d = sqrt(squared(g->x) + squared(g->y));
    }

    // sort the galaxies by distance
    qsort(galaxy, MAX_GALAXY, sizeof(galaxy_t), qsort_compare);

    // set initial time and display width
    t = 13.8;
    disp_width = get_diameter(t,NULL,NULL);

    // init scale factor graph
    g = &graph[0];
    g->exists    = true;
    g->x_str     = "T";
    g->y_str     = "SF";
    g->max_xval  = 100;
    g->max_yval  = get_sf(100);
    g->x_cursor  = t;
    g->y_cursor  = get_sf(t);
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double tg = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = tg;
        g->p[i].y = get_sf(tg);
    }
    g->n = MAX_GRAPH_POINTS;

    // init hubble param graph
    g = &graph[1];
    g->exists    = true;
    g->x_str     = "T";
    g->y_str     = "H";
    g->max_xval  = 100;
    g->max_yval  = 1000;
    g->x_cursor  = t;
    g->y_cursor  = get_h(t);
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double tg = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = tg;
        g->p[i].y = get_h(tg);
    }
    g->n = MAX_GRAPH_POINTS;

    // init num_visible graph
    g = &graph[2];
    g->exists    = true;
    g->x_str     = "T";
    g->y_str     = "NUM_VISIBLE";
    g->max_xval  = 100;
    g->max_yval  = 1800;
    g->x_cursor  = t;
    g->y_cursor  = NO_VALUE;
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double _t = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = _t;
        g->p[i].y = NO_VALUE;
    }
    g->n = MAX_GRAPH_POINTS;

// XXX 
    // init hubble law graph
    g = &graph[3];
    g->exists    = true;
    g->x_str     = "XXX";
    g->y_str     = "XXX";
    g->max_xval  = 1000;
    g->max_yval  = 100000;  //xxx 60000;
    g->x_cursor  = 0; //XXX
    g->y_cursor  = NO_VALUE;
    g->n = 0;

    // create the sim_thread and time_run_thread
    pthread_create(&tid, NULL, sim_thread, NULL);
    pthread_create(&tid, NULL, time_run_thread, NULL);
}

static void * time_run_thread(void *cx)
{
    // xxx comments
    while (true) {
        if (time_run) {
            t += .02;
            if (t >= 100) {
                t = 99.9999;
                time_run = false;
            }
        }
        usleep(20000);
    }
    return NULL;
}

static void * sim_thread(void *cx)
{
    double t_sim_done = 0;
    double t_sim_inprog = 0;

    while (true) {
        // loop until t has changed
        while (t == t_sim_done) {
            usleep(1000);
        }

        // run simulation to determine which galaxies are visible
        t_sim_inprog = t;
        sim_visible(t_sim_inprog);
        t_sim_done = t_sim_inprog;
    }

    return NULL;
}

static void sim_visible(double t_sim)
{
    // backtrack a photon 
    // at each step check to see which galaxies are now closer to earth than the photon,
    // mark these as visible

    assert(t_sim >= .00038 && t_sim < 100);

    // xxx comments

    double d_photon_si, t_photon_si;
    double d_photon, t_photon;
    double h_si, sf;
    int idx, numv;

    d_photon_si = 0;
    t_photon_si = t_sim * S_PER_BYR;
    idx = 0;
    numv = 0;

    while (true) {
        h_si  = get_hsi(t_photon_si - DELTA_T_SECS);
        d_photon_si = (d_photon_si + c_si * DELTA_T_SECS) / (1 + h_si * DELTA_T_SECS);
        t_photon_si -= DELTA_T_SECS;

        d_photon = d_photon_si / M_PER_BLYR;
        t_photon = t_photon_si / S_PER_BYR;

        sf = get_sf(t_photon);
        while (idx < MAX_GALAXY && d_photon > galaxy[idx].d * sf) {
            if (t_photon > galaxy[idx].t_create) {
                galaxy[idx].t_visible = t_photon;
                numv++;
            } else {
                galaxy[idx].t_visible = 0;
            }
            idx++;
        }

        if (t_photon < 0.75) {
            break;
        }
        if (idx == MAX_GALAXY) {
            break;
        }
    }

    for (int i = idx; i < MAX_GALAXY; i++) {
        galaxy[i].t_visible = 0;
    }

    // XXX new routine ?  AND, fill in the graph
    double sfo, sfe, d, z, v, d_mpc, v_kmps;
    int n = 0;

    sfo = get_sf(t_sim);   // scale factor observed
    for (int i = 0; i < MAX_GALAXY; i++) {
        galaxy_t *g = &galaxy[i];
        if (g->t_visible == 0) {
            continue;
        }

        sfe = get_sf(g->t_visible);    // scale factor emitted
        d = g->d * sfo;
        z = (sfo / sfe) - 1;
        v = c_si * z;

        d_mpc =  d * (M_PER_BLYR / M_PER_MPC);
        v_kmps = v / M_PER_KM;

        if (d_mpc >= 1000 || z > .3) {  // xxx z
            break;
        }

        hl.g[n].d_mpc = d_mpc;
        hl.g[n].v_kmps = v_kmps;
        hl.g[n].z = z;
        hl.g[n].h = v_kmps / d_mpc;
        n++;
    }
    hl.n = n;

    // xxx publish
    num_visible = numv;
    // update the graph of num_visible 
    int i = (t_sim / 100) * MAX_GRAPH_POINTS;
    assert(i >= 0 && i < MAX_GRAPH_POINTS);
    graph[2].p[i].y = num_visible;
}


// -----------------  DISPLAY  ------------------------------------

static void display_init(void)
{
    bool fullscreen = false;
    bool resizeable = false;
    bool swap_white_black = false;

    #define REQUESTED_WIN_WIDTH  1600
    #define REQUESTED_WIN_HEIGHT 800

    // init sdl, and get actual window width and height
    win_width  = REQUESTED_WIN_WIDTH;
    win_height = REQUESTED_WIN_HEIGHT;
    if (sdl_init(&win_width, &win_height, fullscreen, resizeable, swap_white_black) < 0) {
        printf("ERROR sdl_init %dx%d failed\n", win_width, win_height);
        exit(1);
    }
    printf("REQUESTED win_width=%d win_height=%d\n", REQUESTED_WIN_WIDTH, REQUESTED_WIN_HEIGHT);
    printf("ACTUAL    win_width=%d win_height=%d\n", win_width, win_height);

    // if created window does not have the requested size then exit
    if (win_width != REQUESTED_WIN_WIDTH || win_height != REQUESTED_WIN_HEIGHT) {
        printf("ERROR failed to create window with requested size\n");
        exit(1);
    }
}

static void display_hndlr(void)
{
    assert(win_width == 2*win_height);

    // call the pane manger; 
    // - this will not return except when it is time to terminate the program
    sdl_pane_manager(
        NULL,           // context
        display_start,  // called prior to pane handlers
        NULL,           // called after pane handlers
        20000,          // 0=continuous, -1=never, else us
        5,              // number of pane handler varargs that follow
        graph_pane_hndlr, (void*)0,
            win_width/2, 0*win_height/4,  win_width/2, win_height/4,
            PANE_BORDER_STYLE_MINIMAL,
        graph_pane_hndlr, (void*)1,
            win_width/2, 1*win_height/4,  win_width/2, win_height/4,
            PANE_BORDER_STYLE_MINIMAL,
        graph_pane_hndlr, (void*)2,
            win_width/2, 2*win_height/4,  win_width/2, win_height/4,
            PANE_BORDER_STYLE_MINIMAL,
        graph_pane_hndlr, (void*)3,
            win_width/2, 3*win_height/4,  win_width/2, win_height/4,
            PANE_BORDER_STYLE_MINIMAL,
        main_pane_hndlr, NULL,
            0, 0, win_width/2, win_height,
            PANE_BORDER_STYLE_MINIMAL
        );
}

// - - - - - - - - -  DISPLAY START - -- - - - - - - - - - - -

static void display_start(void *cx)
{
    // this routine is called prior to pane handlers, and
    // initialize the graphs cursor x,y values
    graph[0].x_cursor = t;
    graph[0].y_cursor = get_sf(t);
    graph[1].x_cursor = t;
    graph[1].y_cursor = get_h(t);
    graph[2].x_cursor = t;
    graph[2].y_cursor = num_visible;

#if 0
    // update the graph of num_visible 
    assert(t_sim_done >= .00038 && t_sim_done < 100);
    int i = (t_sim_done / 100) * MAX_GRAPH_POINTS;
    graph[2].p[i].y = num_visible;
#endif

    for (int i = 0; i < hl.n; i++) {
        graph[3].p[i].x = hl.g[i].d_mpc;
        graph[3].p[i].y = hl.g[i].v_kmps;
    }
    graph[3].n = hl.n;
    if (hl.idx < 0 || hl.idx >= hl.n) {
        hl.idx = 0;
    }
    graph[3].x_cursor = hl.g[hl.idx].d_mpc;
}

// - - - - - - - - -  MAIN PANE HNDLR  - - - - - - - - - - - -

static int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_AUTO_DISP_WIDTH      (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_TIME_RUN     (SDL_EVENT_USER_DEFINED + 2)

    #define PRECISION(x) ((x) == 0 ? 0 : (x) < .001 ? 6 : (x) < 1 ? 3 : (x) < 100 ? 1 : 0)

    static int yellow[256];
    static bool auto_disp_width = false;

    assert(pane->w == pane->h);

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        printf("MAIN_PANE x,y,w,h  %d %d %d %d\n",
               pane->x, pane->y, pane->w, pane->h);

        for (int i = 0; i < 256; i++) {
            yellow[i] = FIRST_SDL_CUSTOM_COLOR+i;
            sdl_define_custom_color(yellow[i], i,i,0);
        }

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // ------------------------
    // -------- RENDER --------
    // ------------------------

    if (request == PANE_HANDLER_REQ_RENDER) {
        int yidx;
        char title_str[100];
        int len;

        // if auto_disp_width is enabled then update disp_width
        // to the diameter of the universe at time t
        if (auto_disp_width) {
            disp_width = get_diameter(t,NULL,NULL);
            if (disp_width == 0) disp_width = 1e-7;
        }

        // display yellow background, the intensity represents the temperature
        double temp = get_temperature(t);

        yidx = log(temp) * (255. / 8.);
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;
        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        // xxx comment
        // - 1 is visible
        // - 2 not visible
        #define MAX_GALAXY_POINTS 500

        point_t points1[MAX_GALAXY_POINTS];
        int n1 = 0, color1 = SDL_WHITE;
        point_t points2[MAX_GALAXY_POINTS];
        int n2 = 0, color2 = SDL_BLUE;
        int ctr = pane->w/2;
        double scale = pane->w/disp_width;
        double sf = get_sf(t);

        for (int i = 0; i < MAX_GALAXY; i++) {
            galaxy_t *g = &galaxy[i];

            if (g->d * sf > disp_width * .7071) {
                break;
            }

            if (t < g->t_create) {
                continue;
            }

            point_t *p = (g->t_visible ? points1 : points2);
            int *n     = (g->t_visible ? &n1 : &n2);
            int color  = (g->t_visible ? color1 : color2);
            
            p[*n].x = ctr + (g->x * sf) * scale;
            p[*n].y = ctr + (g->y * sf) * scale;
            *n = *n + 1;
            if (*n == MAX_GALAXY_POINTS) {
                sdl_render_points(pane, p, *n, color, 1);
                *n = 0;
            }
        }

        sdl_render_points(pane, points1, n1, color1, 1);
        sdl_render_points(pane, points2, n2, color2, 1);

        // display a circle around the observer, representing the current position
        // of the space from which the CMB photons originated
        double diameter = get_diameter(t,NULL,NULL);
        if (diameter > 0) {
            sdl_render_circle(
                pane,
                pane->w / 2, pane->h / 2,
                diameter/2 * (pane->w / disp_width),
                5, SDL_RED);
        }

        // display blue point at center, representing location of the observer
        sdl_render_point(pane, pane->w/2, pane->h/2, SDL_LIGHT_BLUE, 5);

        // register the SDL_EVENT_AUTO_DISP_WIDTH, which will toggle the
        // mode that automatically adjusts the display width to match the
        // diameter of the universe at time t
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(15,FONT_SZ), ROW2Y(2,FONT_SZ), FONT_SZ, 
                (auto_disp_width ? "AUTO_DW_IS_ON  " : "AUTO_DW_IS_OFF "),
                SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_AUTO_DISP_WIDTH, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // register the SDL_EVENT_TIME_RUN which, when enabled, causes time
        // to automatically advance to its maximum value of 100 BYR
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(15,FONT_SZ), ROW2Y(4,FONT_SZ), FONT_SZ, 
                (time_run ? "TIME_IS_RUNNING" : "TIME_IS_STOPPED"),
                SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_TIME_RUN, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // display the title_str, containing DISPLAY_WIDTH, at top center
        sprintf(title_str, "DISPLAY_WIDTH=%0.*f",\
                PRECISION(disp_width), disp_width);
        len = strlen(title_str);
        sdl_render_printf(
            pane, pane->w/2 - COL2X(len,FONT_SZ/2), 0,
            FONT_SZ, SDL_WHITE, SDL_BLACK, "%s", title_str);

        // display status values
        int row=2;
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "T        %-8.*f", PRECISION(t), t);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "TEMP     %-8.*f", PRECISION(temp), temp);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "DIAMETER %-8.*f", PRECISION(diameter), diameter);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "NUM_VIS  %-8d", num_visible);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_TIME_RUN:
        case 'x':
            time_run = !time_run;
            break;
        case SDL_EVENT_AUTO_DISP_WIDTH:
            auto_disp_width = !auto_disp_width;
            if (auto_disp_width) {
                disp_width = get_diameter(t,NULL,NULL);
                if (disp_width == 0) disp_width = 1e-7;
            }
            break;
        case SDL_EVENT_KEY_UP_ARROW:
        case SDL_EVENT_KEY_DOWN_ARROW:
        case SDL_EVENT_KEY_SHIFT_UP_ARROW:
        case SDL_EVENT_KEY_SHIFT_DOWN_ARROW:
            // adjust disp_width  use similar method as t
            if (auto_disp_width) {
                break;
            }
            switch (event->event_id) {
            case SDL_EVENT_KEY_UP_ARROW:         disp_width *= 1.01; break;
            case SDL_EVENT_KEY_DOWN_ARROW:       disp_width /= 1.01; break;
            case SDL_EVENT_KEY_SHIFT_UP_ARROW:   disp_width *= 1.1;  break;
            case SDL_EVENT_KEY_SHIFT_DOWN_ARROW: disp_width /= 1.1;  break;
            }
            break;
        case SDL_EVENT_KEY_RIGHT_ARROW:
        case SDL_EVENT_KEY_LEFT_ARROW:
        case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW:
        case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:
        case SDL_EVENT_KEY_INSERT:
        case SDL_EVENT_KEY_HOME:
        case SDL_EVENT_KEY_END:;
            // xxx also use alt arrows
            // adjust t
            if (event->event_id == SDL_EVENT_KEY_INSERT) {
                t = .000380;
            } else if (event->event_id == SDL_EVENT_KEY_HOME) {
                t = 13.8;
            } else if (event->event_id == SDL_EVENT_KEY_END) {
                t = 99.9999;
            } else {
                double _t = t;
                switch (event->event_id) {
                case SDL_EVENT_KEY_RIGHT_ARROW:       _t += .1;    break;
                case SDL_EVENT_KEY_LEFT_ARROW:        _t -= .1;    break;
                case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW: _t += 1;  break;
                case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:  _t -= 1;  break;
                }
                if (_t < .00038) _t = .00038;
                if (_t >= 100) _t = 99.9999;
                t = _t;
            }
            break;
        case '1':
            if (hl.idx > 0) hl.idx--;
            break;
        case '2':
            if (hl.idx < hl.n-1) hl.idx++;
            break;
        default: 
            break;
        }

        return PANE_HANDLER_RET_DISPLAY_REDRAW;
    }

    // ---------------------------
    // -------- TERMINATE --------
    // ---------------------------

    if (request == PANE_HANDLER_REQ_TERMINATE) {
        free(vars);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // not reached
    assert(0);
    return PANE_HANDLER_RET_NO_ACTION;
}

// - - - - - - - - -  GRAPH PANE HNDLR  - - - - - - - - - - - -

static int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int graph_idx;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        vars->graph_idx = (long)init_params;

        printf("GRAPH_PANE_%d x,y,w,h  %d %d %d %d\n",
               vars->graph_idx,
               pane->x, pane->y, pane->w, pane->h);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // ------------------------
    // -------- RENDER --------
    // ------------------------

    if (request == PANE_HANDLER_REQ_RENDER) {
        graph_t *g = &graph[vars->graph_idx];
        static point_t points[1000];
        char str[100];
        int xi, n=0;

        assert(g->n <= ARRAY_SIZE(points));

        // if this graph does not exist then return
        if (!g->exists) {
            return PANE_HANDLER_RET_NO_ACTION;
        }

        // create the array of graph points that are to be displayed
        for (int i = 0; i < g->n; i++) {
            if (g->p[i].y == NO_VALUE) {
                continue;
            }
            points[n].x = ((g->p[i].x / g->max_xval) * (pane->w));
            points[n].y = (pane->h - 1) -
                          ((g->p[i].y / g->max_yval) * (pane->h - ROW2Y(1,FONT_SZ)));
            n++;
        }
        sdl_render_points(pane, points, n, SDL_WHITE, 2);  // xxx 1

        // print max_yval
        sdl_render_printf(pane, 0, ROW2Y(1,FONT_SZ), 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%0.*f", PRECISION(g->max_yval), g->max_yval);

        // print max_xval
        sprintf(str, "%0.*f", PRECISION(g->max_xval), g->max_xval);
        sdl_render_printf(pane, pane->w-COL2X(strlen(str),FONT_SZ), pane->h-ROW2Y(1,FONT_SZ), 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%s", str);

        // display cursor
        xi = (g->x_cursor / g->max_xval) * pane->w;
        //sdl_render_line(pane, xi-1, pane->h-1, xi-1, ROW2Y(2,FONT_SZ), SDL_WHITE);
        sdl_render_line(pane, xi-0, pane->h-1, xi-0, ROW2Y(2,FONT_SZ), SDL_WHITE);
        //sdl_render_line(pane, xi+1, pane->h-1, xi+1, ROW2Y(2,FONT_SZ), SDL_WHITE);

        // display title
        if (g->y_cursor != NO_VALUE) {
            sprintf(str, "%s=%0.*f  %s=%0.*f", 
                    g->x_str, PRECISION(g->x_cursor), g->x_cursor,
                    g->y_str, PRECISION(g->y_cursor), g->y_cursor);
        } else {
            sprintf(str, "%s=%0.*f  %s=", 
                    g->x_str, PRECISION(g->x_cursor), g->x_cursor,
                    g->y_str);
        }

        sdl_render_printf(pane, COL2X(8,FONT_SZ), 0, 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%s", str);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        return PANE_HANDLER_RET_DISPLAY_REDRAW;
    }

    // ---------------------------
    // -------- TERMINATE --------
    // ---------------------------

    if (request == PANE_HANDLER_REQ_TERMINATE) {
        free(vars);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // not reached
    assert(0);
    return PANE_HANDLER_RET_NO_ACTION;
}

// -----------------  SUPPORT / UTILS  ----------------------------

static double squared(double x)
{
    return x * x;
}


static double get_temperature(double t)
{
    double temp;

    temp = TEMP_START / (get_sf(t) / get_sf(T_START));
    return temp;
}

static int qsort_compare(const void *a, const void *b)
{
    const galaxy_t *ga = a;
    const galaxy_t *gb = b;

    return (ga->d  > gb->d ? 1 :
            ga->d == gb->d ? 0 :
                            -1);
}
