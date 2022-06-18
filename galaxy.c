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

#define PRECISION(x) ((x) == 0 ? 0 : (x) < .001 ? 6 : (x) < 1 ? 3 : (x) < 100 ? 1 : 0)

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
    double max_xval;
    double max_yval;
    double x_cursor;
    char title[100];
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
static bool         time_run;
static bool         auto_disp_width;

//
// prototypes
//

static void galaxy_sim_init(void);
static void *time_run_thread(void *cx);
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
        if (i < 6) {
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
    g->max_xval  = 100;
    g->max_yval  = get_sf(100);
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double tg = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = tg;
        g->p[i].y = get_sf(tg);
    }
    g->n = MAX_GRAPH_POINTS;

    // init hubble param graph
    g = &graph[1];
    g->exists    = true;
    g->max_xval  = 100;
    g->max_yval  = 1000;
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double tg = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = tg;
        g->p[i].y = get_h(tg);
    }
    g->n = MAX_GRAPH_POINTS;

    // init num_visible graph
    g = &graph[2];
    g->exists    = true;
    g->max_xval  = 100;
    g->max_yval  = 1800;
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double _t = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = _t;
        g->p[i].y = NO_VALUE;
    }
    g->n = MAX_GRAPH_POINTS;

    // init hubble law graph; the graph points are initialized in display_start
    g = &graph[3];
    g->exists    = true;
    g->max_xval  = 1000;
    g->max_yval  = 100000;
    g->n = 0;

    // create the sim_thread and time_run_thread
    pthread_create(&tid, NULL, sim_thread, NULL);
    pthread_create(&tid, NULL, time_run_thread, NULL);
}

static void * time_run_thread(void *cx)
{
    // if the time_run flag is set then advance the sim time by .02 BYR every 20 ms
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
    // This routine calculates the distance of a photon from the perpetual earth 
    // going backward in time. The starting time is the simulation time 't' or 't_sim', at
    // which time the distance of the photon from earth (d_photon) is 0.
    //
    // The distance of the photon is updated in time steps of -DELTA_T_SECS (-10000 yrs), until
    // the time reaches 0.75 BYR, which is the time that there are no galaxies in this simulation.
    //
    // At each time step the distance of the photon from the earth is compared with the 
    // distance of the galaxies from the earth at that time. If the distance of the photon has 
    // become greater than the distance to a galaxy then that galaxy is visible from the earth. 
    // The time that the photon would have left the visible galaxy and later arrived at earth 
    // is t_photon.

    double d_photon_si, t_photon_si;
    double d_photon, t_photon;
    double h_si, sf;
    int idx, numv;

    assert(t_sim >= .00038 && t_sim < 100);

    // init photon backtracing variables
    d_photon_si = 0;
    t_photon_si = t_sim * S_PER_BYR;
    idx = 0;
    numv = 0;

    // backtrack the photon from starting time t_sim to time 0.75 BYR
    while (true) {
        // calculate the position of the photon for time step of -DELTA_T_SECS
        h_si  = get_hsi(t_photon_si - DELTA_T_SECS);
        d_photon_si = (d_photon_si + c_si * DELTA_T_SECS) / (1 + h_si * DELTA_T_SECS);
        t_photon_si -= DELTA_T_SECS;

        // convert the photon distance and time from SI units to BLYR and BYR
        d_photon = d_photon_si / M_PER_BLYR;
        t_photon = t_photon_si / S_PER_BYR;

        // for all the galaxies that the photon now exceeds distance, set the
        // galaxies t_visisble, which indicates both that the galaxy is visible 
        // from earth and the time of the photon leaving the galaxy
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

        // in this simulation there should always be galaxies that are not visible
        assert(idx < MAX_GALAXY);

        // this simulation has no galaxies prior to 0.75 BYR, so when t_photon is 
        // less than 0.75 break out of this loop
        if (t_photon < 0.75) {
            break;
        }
    }

    // set t_visible to 0 for the rest of the galaxies
    for (int i = idx; i < MAX_GALAXY; i++) {
        galaxy[i].t_visible = 0;
    }

    // This section supports the Hubble Law graph.
    //
    // For galaxies that are visible and whose distance < 1000 MPC and z < .3,
    // add the information for these galaxies to the Hubble Law (hl) table.
    //
    // This table is used to display the Hubble Law graph (graph[3]).

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

        if (d_mpc >= 1000 || z > .3) {
            break;
        }

        hl.g[n].d_mpc = d_mpc;
        hl.g[n].v_kmps = v_kmps;
        hl.g[n].z = z;
        hl.g[n].h = v_kmps / d_mpc;
        n++;
    }
    hl.n = n;

    // publish num_visible
    num_visible = numv;

    // add the num_visible value to the Num Visible graph (graph[2])
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
        printf("ERROR: sdl_init %dx%d failed\n", win_width, win_height);
        exit(1);
    }
    printf("REQUESTED win_width=%d win_height=%d\n", REQUESTED_WIN_WIDTH, REQUESTED_WIN_HEIGHT);
    printf("ACTUAL    win_width=%d win_height=%d\n", win_width, win_height);

    // if created window does not have the requested size then exit
    if (win_width != REQUESTED_WIN_WIDTH || win_height != REQUESTED_WIN_HEIGHT) {
        printf("ERROR: failed to create window with requested size\n");
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

// this routine is called prior to pane handlers
static void display_start(void *cx)
{
    double sf = get_sf(t);
    double h = get_h(t);
    graph_t *gr;
    int i;

    // initialize the graphs cursor x,y values
    gr = &graph[0];
    gr->x_cursor = t;
    sprintf(gr->title, "T=%.*f  SF=%.*f", PRECISION(t), t, PRECISION(sf), sf);

    gr = &graph[1];
    gr->x_cursor = t;
    sprintf(gr->title, "T=%.*f  H=%.*f", PRECISION(t), t, PRECISION(h), h);

    gr = &graph[2];
    gr->x_cursor = t;
    sprintf(gr->title, "T=%.*f  NUM_VISIBLE=%d", PRECISION(t), t, num_visible);

    gr = &graph[3];
    for (int i = 0; i < hl.n; i++) {
        gr->p[i].x = hl.g[i].d_mpc;
        gr->p[i].y = hl.g[i].v_kmps;
    }
    gr->n = hl.n;
    if (hl.idx < 0 || hl.idx >= hl.n) {
        hl.idx = 0;
    }
    gr->x_cursor = hl.g[hl.idx].d_mpc;
    sprintf(gr->title, "MPC=%.*f  KM/S=%.*f  Z=%.*f  H=%.*f",
            PRECISION(hl.g[hl.idx].d_mpc), hl.g[hl.idx].d_mpc,
            PRECISION(hl.g[hl.idx].v_kmps), hl.g[hl.idx].v_kmps,
            PRECISION(hl.g[hl.idx].z), hl.g[hl.idx].z,
            PRECISION(hl.g[hl.idx].h), hl.g[hl.idx].h);
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

    static int yellow[256];

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

        // Display the galaxies that are visible as white points and the
        // galaxies that are not visible as blue points.
        //
        // The points1 array is for the visible galaxies (displayed white)
        // The points2 array is for the not visible galaxies (displayed blue)
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
            // adjust disp_width
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
            // adjust t   xxx ALT arrows?
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
        graph_t *gr = &graph[vars->graph_idx];
        static point_t points[1000];
        char str[100];
        int xi, n=0;

        assert(gr->n <= ARRAY_SIZE(points));

        // if this graph does not exist then return
        if (!gr->exists) {
            return PANE_HANDLER_RET_NO_ACTION;
        }

        // create the array of graph points that are to be displayed
        for (int i = 0; i < gr->n; i++) {
            if (gr->p[i].y == NO_VALUE) {
                continue;
            }
            points[n].x = ((gr->p[i].x / gr->max_xval) * (pane->w));
            points[n].y = (pane->h - 1) -
                          ((gr->p[i].y / gr->max_yval) * (pane->h - ROW2Y(1,FONT_SZ)));
            n++;
        }
        sdl_render_points(pane, points, n, SDL_WHITE, 2);

        // print max_yval
        sdl_render_printf(pane, 0, ROW2Y(1,FONT_SZ), 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%0.*f", PRECISION(gr->max_yval), gr->max_yval);

        // print max_xval
        sprintf(str, "%0.*f", PRECISION(gr->max_xval), gr->max_xval);
        sdl_render_printf(pane, pane->w-COL2X(strlen(str),FONT_SZ), pane->h-ROW2Y(1,FONT_SZ), 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%s", str);

        // display cursor
        xi = (gr->x_cursor / gr->max_xval) * pane->w;
        sdl_render_line(pane, xi-0, pane->h-1, xi-0, ROW2Y(2,FONT_SZ), SDL_WHITE);

        // display title
        sdl_render_printf(pane, COL2X(8,FONT_SZ), 0, 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%s", gr->title);

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
