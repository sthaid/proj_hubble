// xxx  todo
// - graphs
//   - hubble law
//   - diameter
// - diametet tracking
// - lock main pane to front
// - limit on repeat key
// - linear time option
// - auto time sweep ??

#include <common.h>
#include <util_sdl.h>

//
// defines
//

#define STATE_RESET    0
#define STATE_RUNNING  1
#define STATE_PAUSED   2
#define STATE_DONE     3

#define STATE_STR(x) \
    ((x) == STATE_RESET   ? "RESET"   : \
     (x) == STATE_RUNNING ? "RUNNING" : \
     (x) == STATE_PAUSED  ? "PAUSED"  : \
     (x) == STATE_DONE    ? "DONE"    : \
                            (assert(0),""))

#define MAX_GALAXY 1000000
#define MAX_GRAPH_POINTS 796

#define MB 0x100000
#define FONT_SZ 24
#define DEG2RAD(deg)  ((deg) * (M_PI / 180))

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
    double y_cursor;
    struct {
        double x;
        double y;
    } p[MAX_GRAPH_POINTS];
    int n;
} graph_t;

//
// variables
//

static int       win_width, win_height;
static double    disp_width;

static int       state;

static galaxy_t *galaxy;

static double    t;

static graph_t   graph[4];

//
// prototypes
//

static void galaxy_sim_init(void);
static void *sim_thread(void *cx);
static int sim_visible(double t_sim);

static void display_init(void);
static void display_hndlr(void);
static int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
static int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);

static double squared(double x);
static double get_temperature(double t);
static int qsort_compare(const void *a, const void *b);

// xxx placement
static void set_num_visible(double t_arg, int numv);
static int get_num_visible(double t_arg);

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

    size = MAX_GALAXY * sizeof(galaxy_t);
    printf("size needed %zd MB\n", size/MB);
    galaxy = malloc(size);
    if (galaxy == NULL) {
        printf("ERROR: failed to allocate memory for galaxy, size needed %zd MB\n", size/MB);
        exit(1);
    }

    printf("INITIALIZING\n");
    for (i = 0; i < MAX_GALAXY; i++) {
        galaxy_t *g = &galaxy[i];
        g->x = random_range(-1000, 1000);
        g->y = random_range(-1000, 1000);
        //g->x = random_range(-500, 500);
        //g->y = random_range(-500, 500);
        g->d = sqrt(squared(g->x) + squared(g->y));
        g->t_create = random_triangular(0.75, 1.25);
    }

    // sort by distance
    printf("SORTING\n");
    qsort(galaxy, MAX_GALAXY, sizeof(galaxy_t), qsort_compare);
    printf("SORTING DONE\n");

    // xxx
    //init_num_visible();

    // xxx
    t = 13.8;  // xxx
    //disp_width = get_diameter(t,NULL,NULL);
    disp_width = 200;

    // init scale factor graph
    g = &graph[0];
    g->exists    = true;
    g->x_str     = "T";
    g->y_str     = "SF";
    g->max_xval  = 100;
    g->max_yval  = get_sf(100);  //xxx tmax
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
    g->max_yval  = 7000;
    //g->max_yval  = 2000;
    g->x_cursor  = t;
    g->y_cursor  = NO_VALUE;
    for (i = 0; i < MAX_GRAPH_POINTS; i++) {
        double _t = (i + 0.5) * (g->max_xval / MAX_GRAPH_POINTS);
        g->p[i].x = _t;
        g->p[i].y = NO_VALUE;
    }
    g->n = MAX_GRAPH_POINTS;

    // create the sim_thread, which xxx .... 
    pthread_create(&tid, NULL, sim_thread, NULL);
}

static void * sim_thread(void *cx)
{
    double t_sim_done = 0;
    double t_sim_inprog = 0;

    while (true) {
        while (t == t_sim_done) {
            usleep(1000);
        }

        t_sim_inprog = t;
        sim_visible(t_sim_inprog);
        t_sim_done = t_sim_inprog;
    }

    return NULL;
}

static int sim_visible(double t_sim)
{
    // backtrack a photon 
    // at each step check to see which galaxies are now closer to earth than the photon,
    // mark these as visible

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

        sf = get_sf(t_photon);  // xxx check this
        while (idx < MAX_GALAXY && d_photon > galaxy[idx].d * sf) {  // xxx optimize this line ?
            if (t_photon > galaxy[idx].t_create) {  //xxx
                galaxy[idx].t_visible = t_photon;
                numv++;
            } else {
                galaxy[idx].t_visible = 0;
            }
            idx++;
        }

        if (t_photon < 0.75) {  // xxx define for 0.75
            break;
        }
        if (idx == MAX_GALAXY) {
            break;
        }
    }

    for (int i = idx; i < MAX_GALAXY; i++) {
        galaxy[i].t_visible = 0;
    }

    set_num_visible(t_sim, numv);

    return numv;
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

// xxx move main_pane to first
    // call the pane manger; 
    // - this will not return except when it is time to terminate the program
    sdl_pane_manager(
        NULL,           // context
        NULL,           // called prior to pane handlers
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

// - - - - - - - - -  MAIN PANE HNDLR  - - - - - - - - - - - -

static int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_AUTO_DW      (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_TIME_ADJUST  (SDL_EVENT_USER_DEFINED + 1)
    #define SDL_EVENT_TIME_RUN     (SDL_EVENT_USER_DEFINED + 2)

    #define PRECISION(x) ((x) == 0 ? 0 : (x) < .001 ? 6 : (x) < 1 ? 3 : (x) < 100 ? 1 : 0)

    #define TIME_ADJUST_EXPONENTIAL 0
    #define TIME_ADJUST_LINEAR      1

    static int yellow[256];
    static bool auto_dw = false;
    static int time_adjust = TIME_ADJUST_LINEAR;
    static bool time_run = false;

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

//{ static int cnt;
  //printf("render cnt %d\n", ++cnt);
//}
        // xxx numvisible is a problem
        if (time_run) {
            t += .02;
            if (t >= 100) {
                t = 99.9999;
                time_run = false;
            }
        }

// xxx maybe make a pre routine for this and post routine for time run
        // xxx  will need to move these to support time_run
        if (auto_dw) {
            disp_width = get_diameter(t,NULL,NULL);
            if (disp_width == 0) disp_width = 1e-7;
        }

        graph[0].x_cursor = t;  // xxx may want a routine for this
        graph[0].y_cursor = get_sf(t);
        graph[1].x_cursor = t;
        graph[1].y_cursor = get_h(t);
        graph[2].x_cursor = t;
        graph[2].y_cursor = get_num_visible(t);


        // display yellow background, the intensity represents the temperature
        double temp = get_temperature(t);

        yidx = log(temp) * (255. / 8.);
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;
        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        // xxx
        // xxx 1 is visible
        // xxx 2 not visible
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
        sdl_render_point(pane, pane->w/2, pane->h/2, SDL_LIGHT_BLUE, 8);

        // register the SDL_EVENT_AUTO_DW, which xxx
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(15,FONT_SZ), ROW2Y(2,FONT_SZ), FONT_SZ, 
                (auto_dw ? "AUTO_DW_IS_ON  " : "AUTO_DW_IS_OFF "),
                SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_AUTO_DW, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // xxx
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(15,FONT_SZ), ROW2Y(3,FONT_SZ), FONT_SZ, 
                (time_adjust == TIME_ADJUST_EXPONENTIAL ? "TIME_IS_EXP    " : "TIME_IS_LINEAR "),
                SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_TIME_ADJUST, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(15,FONT_SZ), ROW2Y(4,FONT_SZ), FONT_SZ, 
                (time_run ? "TIME_IS_RUNNING" : "TIME_IS_STOPPED"),
                SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_TIME_RUN, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // display current state at top middle
        sprintf(title_str, "%s  DISPLAY_WIDTH=%0.*f",\
            STATE_STR(state), PRECISION(disp_width), disp_width);
        len = strlen(title_str);
        sdl_render_printf(
            pane, pane->w/2 - COL2X(len,FONT_SZ/2), 0,
            FONT_SZ, SDL_WHITE, SDL_BLACK, "%s", title_str);

        // display status
        int row=2, numv;
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "T        %-8.*f", PRECISION(t), t);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "TEMP     %-8.*f", PRECISION(temp), temp);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "DIAMETER %-8.*f", PRECISION(diameter), diameter);
        numv = get_num_visible(t);
        if (numv != NO_VALUE) {
            sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "NUM_VIS  %-8d", numv);
        } else {
            sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "NUM_VIS  %-8s", "");
        }



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
        case SDL_EVENT_AUTO_DW:
            auto_dw = !auto_dw;
            if (auto_dw) {
                disp_width = get_diameter(t,NULL,NULL);
                if (disp_width == 0) disp_width = 1e-7;
            }
            break;
        case SDL_EVENT_TIME_ADJUST:
            time_adjust = (time_adjust == TIME_ADJUST_EXPONENTIAL ?
                           TIME_ADJUST_LINEAR : TIME_ADJUST_EXPONENTIAL);
            break;
        case SDL_EVENT_KEY_UP_ARROW:
        case SDL_EVENT_KEY_DOWN_ARROW:
        case SDL_EVENT_KEY_SHIFT_UP_ARROW:
        case SDL_EVENT_KEY_SHIFT_DOWN_ARROW:
            // adjust disp_width  use similar method as t
            if (auto_dw) {
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

            //if (time_run) {
                //break;
            //}

            // adjust t
            if (event->event_id == SDL_EVENT_KEY_INSERT) {
                t = .000380;
                //tidx = log(t / .00038) / log(1.01);
            } else if (event->event_id == SDL_EVENT_KEY_HOME) {
                t = 13.8;
                //tidx = log(t / .00038) / log(1.01);
            } else if (event->event_id == SDL_EVENT_KEY_END) {
                t = 99.721;
                //tidx = log(t / .00038) / log(1.01);
            } else if (time_adjust == TIME_ADJUST_EXPONENTIAL) {
                //static int tidx = 1055;  // xxx comment, and use define for 100
                int tidx;

                tidx = log(t / .00038) / log(1.01) + 0.5;
                

                switch (event->event_id) {
                case SDL_EVENT_KEY_RIGHT_ARROW:       tidx++;    break;
                case SDL_EVENT_KEY_LEFT_ARROW:        tidx--;    break;
                case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW: tidx+=10;  break;
                case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:  tidx-=10;  break;
                }
                if (tidx < 0) tidx = 0;
                if (tidx > 1254) tidx = 1254;  // xxx comment
                t = .000380 * pow(1.01, tidx);
            } else {  // time_adjust == TIME_ADJST_LINEAR
                switch (event->event_id) {
                case SDL_EVENT_KEY_RIGHT_ARROW:       t += .1;    break;
                case SDL_EVENT_KEY_LEFT_ARROW:        t -= .1;    break;
                case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW: t += 1;  break;
                case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:  t -= 1;  break;
                }
                if (t < .00038) t = .00038;
                if (t >= 100) t = 99.9999;
            }
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

        // xxx
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
        sdl_render_points(pane, points, n, SDL_WHITE, 1);

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
        sdl_render_line(pane, xi-1, pane->h-1, xi-1, ROW2Y(2,FONT_SZ), SDL_WHITE);
        sdl_render_line(pane, xi-0, pane->h-1, xi-0, ROW2Y(2,FONT_SZ), SDL_WHITE);
        sdl_render_line(pane, xi+1, pane->h-1, xi+1, ROW2Y(2,FONT_SZ), SDL_WHITE);

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

static struct {
    double t;
    int val;
} num_visible = {0,NO_VALUE};

static void set_num_visible(double t_arg, int numv)
{
    int i;

    i = (t_arg / 100.00001) * MAX_GRAPH_POINTS;

    if (i >= MAX_GRAPH_POINTS) {
        printf("ERROR: BUG i %d\n", i);
    } else {
        graph[2].p[i].y = numv;
        //printf("t=%f  num_vis=%d\n", t, numv);
    }
    if (graph[2].x_cursor == t_arg) {
        graph[2].y_cursor = numv;
    }

    num_visible.t = t_arg;
    num_visible.val = numv;

}

static int get_num_visible(double t_arg)
{
    if (t_arg == num_visible.t) {
        return num_visible.val;
    } else {
        int i = (t_arg / 100.00001) * MAX_GRAPH_POINTS;
        if (i >= MAX_GRAPH_POINTS) {
            printf("ERROR: BUG i %d\n", i);
            return NO_VALUE;
        } else {
            return graph[2].p[i].y;
            //printf("t=%f  num_vis=%d\n", t, numv);
        }
    }
}

