// xxx

#include <common.h>
#include <util_sdl.h>

//
// defines
//

#define FONT_SZ 24

#define DEG2RAD(deg)  ((deg) * (M_PI / 180))

#define MAX_GRAPH_POINTS 796 // xxx assert

// xxx may not need
#define STATE_STR(x) \
    ((x) == RESET   ? "RESET"   : \
     (x) == RUNNING ? "RUNNING" : \
     (x) == PAUSED  ? "PAUSED"  : \
     (x) == DONE    ? "DONE"    : \
                      (assert(0),""))

//
// typdedefs
// 

typedef enum {RESET, RUNNING, PAUSED, DONE} state_t;

typedef struct {
    char *x_str;
    char *y_str;
    double max_xval;
    double max_yval;
    double x_cursor;
    struct {
        double x;
        double y;
    } p[MAX_GRAPH_POINTS];
    int n;
} graph_t;

// xxx ADD_GRAPH_POINT

//
// variables
//

int win_width, win_height;
double disp_width;

state_t state;

double  t;
int     num_visible;

graph_t graph[4];

//
// prototypes
//

void galaxy_sim_init(void);
void *galaxy_sim_thread(void *cx);
void sim_reset(void);
void sim_pause(void);
void sim_resume(void);

void display_init(void);
void display_hndlr(void);
int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);

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

// -----------------  CMB SIM  ------------------------------------

static double get_temperature(double t)
{
    double temp;

    temp = TEMP_START / (get_sf(t) / get_sf(T_START));
    return temp;
}

#define MB 0x100000
#define MAX_GALAXY 1000000

typedef struct {
    float x;
    float y;
    float d;
    float t_create;
    float t_visible;
} galaxy_t;

galaxy_t *galaxy;

static double squared(double x)
{
    return x * x;
}

int compare(const void *a, const void *b)
{
    const galaxy_t *ga = a;
    const galaxy_t *gb = b;

    return (ga->d  > gb->d ? 1 :
            ga->d == gb->d ? 0 :
                            -1);
}

void galaxy_sim_init(void)
{
    pthread_t tid;
    size_t size;
    int i;

    size = MAX_GALAXY * sizeof(galaxy_t);
    printf("size needed %zd MB\n", size/MB);
    galaxy = malloc(size);
    if (galaxy == NULL) {
        printf("ERROR: failed to allocate memory for galaxy, size needed %zd MB\n", size/MB);
        exit(1);
    }
    printf("  malloc ret %p\n", galaxy);

    printf("INITIALIZING\n");
    for (i = 0; i < MAX_GALAXY; i++) {
        galaxy_t *g = &galaxy[i];
        g->x = random_range(-500, 500);
        g->y = random_range(-500, 500);
        g->d = sqrt(squared(g->x) + squared(g->y));
        g->t_create = random_triangular(0.75, 1.25);
    }

    // sort by distance
    printf("SORTING\n");
    qsort(galaxy, MAX_GALAXY, sizeof(galaxy_t), compare);
    printf("SORTING DONE\n");

#if 0
    for (i = 0; i < 50; i++) {
        printf("%d t_create = %f\n", i, galaxy[i].t_create);
    }
    exit(1);
#endif


    // reset the simulation
    sim_reset();

    // create the cmd_sim_thread
    pthread_create(&tid, NULL, galaxy_sim_thread, NULL);
}

void * galaxy_sim_thread(void *cx)
{
    double t_computed = 0;
    double t_working = 0;

    while (true) {
        while (t == t_computed) {
            usleep(1000);
        }

//restart:
        t_working = t;
        t_computed = 0;
        printf("STARTING for t = %f\n", t_working);

        // xxx time this

#if 0
        printf("CLEARING T_VISIBLE\n");
        for (int i = 0; i < MAX_GALAXY; i++) {
            galaxy[i].t_visible = 0;
        }
        printf("DONE CLEARING T_VISIBLE\n");
#endif

        // backtrack a photon 
        // at each step check to see which galaxies are now closer to earth than the photon,
        // mark these as visible

        double d_photon_si, t_photon_si;
        double d_photon, t_photon;
        double h_si, sf;
        int idx, lcl_num_visible;

        d_photon_si = 0;
        t_photon_si = t_working * S_PER_BYR;
        idx = 0;
        lcl_num_visible = 0;

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
                    lcl_num_visible++;
                }
                idx++;
            }

#if 0
            if (t != t_working) {
                printf("RESTARTING\n");
                goto restart;
            }
#endif
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

        num_visible = lcl_num_visible;
        t_computed = t_working;

        printf("DONE t=%f  t_photon=%f  num_visible=%d\n", 
               t_computed, t_photon, num_visible);
    }
}

void add_graph_point(graph_t *g, double x, double y)
{
    if (g->n >= MAX_GRAPH_POINTS) {
        printf("ERROR graph full\n");
        return;
    }
    g->p[g->n].x = x;
    g->p[g->n].y = y;
    g->n++;
}

void sim_reset(void)
{
    t            = 13.8;  // xxx
    disp_width   = get_diameter(t,NULL,NULL);

    graph_t *g;

#if 0
typedef struct {
    char *x_str;
    char *y_str;
    double max_xval;
    double max_yval;
    double x_cursor;
    struct {
        double x;
        double y;
    } p[MAX_GRAPH_POINTS];
    int n; 
} graph_t;
#endif

    g = &graph[0];
    g->x_str     = "T";
    g->y_str     = "SF";
    g->max_xval  = 100;
    g->max_yval  = get_sf(100);  // xxx tmax
    g->x_cursor  = t;

    // xxx move graphs to sim_init?
    for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
        double ti = i * (100. / MAX_GRAPH_POINTS);
        if (ti < T_START) continue;
        add_graph_point(g, ti, get_sf(ti));
    }
}

void sim_pause(void)
{
    if (state == RUNNING) {
        state = PAUSED;
    }
}

void sim_resume(void)
{
    if (state == RESET || state == PAUSED) {
        state = RUNNING;
    }
}

// -----------------  DISPLAY  ------------------------------------

void display_init(void)
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

void display_hndlr(void)
{
    assert(win_width == 2*win_height);

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

int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_CTRL      (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_ZOOM      (SDL_EVENT_USER_DEFINED + 1)
    #define SDL_EVENT_RESET     (SDL_EVENT_USER_DEFINED + 2)
    #define SDL_EVENT_RESET_DW  (SDL_EVENT_USER_DEFINED + 3)

    #define PRECISION(x) ((x) == 0 ? 0 : (x) < .001 ? 6 : (x) < 1 ? 3 : (x) < 100 ? 1 : 0)

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
        char *ctrl_str;
        char title_str[100];
        int len;

        // display yellow background, the intensity represents the temperature
        double temp = get_temperature(t);

        yidx = log(temp) * (255. / 8.);
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;
        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        // xxx
// xxx 1 is visible
// xxx 2 not visible
        #define MAX_POINTS 500

        point_t points1[MAX_POINTS];
        int n1 = 0, color1 = SDL_WHITE;

        point_t points2[MAX_POINTS];
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
            if (*n == MAX_POINTS) {
                sdl_render_points(pane, p, *n, color, 1);
                *n = 0;
            }
        }
        sdl_render_points(pane, points1, n1, color1, 1);
        sdl_render_points(pane, points2, n2, color2, 1);

        // display a circle around the observer, representing the current position
        // of the space from which the CMB photons originated
        double diameter = get_diameter(t,NULL,NULL);
        sdl_render_circle(
            pane,
            pane->w / 2, pane->h / 2,
            diameter/2 * (pane->w / disp_width),
            5, SDL_RED);

        // display blue point at center, representing location of the observer
        sdl_render_point(pane, pane->w/2, pane->h/2, SDL_LIGHT_BLUE, 8);

        // display the control string and register the SDL_EVENT_CTRL
        ctrl_str = (state == RESET    ? "RUN"    :
                    state == PAUSED   ? "RESUME" :
                    state == RUNNING  ? "PAUSE"  :
                    state == DONE     ? "RESET"  :
                                        (assert(0), ""));
        sdl_render_text_and_register_event(
            pane, 0, 0, FONT_SZ, ctrl_str, SDL_LIGHT_BLUE, SDL_BLACK, 
            SDL_EVENT_CTRL, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // register the SDL_EVENT_RESET, which resets the simulation
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(8,FONT_SZ), 0, FONT_SZ, "RESET", SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_RESET, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // register the SDL_EVENT_RESET_DW, which resets the display width
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(8,FONT_SZ), ROW2Y(2,FONT_SZ), FONT_SZ, "RESET_DW", SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_RESET_DW, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // register the SDL_EVENT_ZOOM which is used to adjust the 
        // display width scale using the mouse wheel
        sdl_register_event(pane, &(rect_t){0,0,pane->w,pane->h}, 
            SDL_EVENT_ZOOM, SDL_EVENT_TYPE_MOUSE_WHEEL, pane_cx);

        // display current state at top middle
        sprintf(title_str, "%s  DISPLAY_WIDTH=%0.*f",\
            STATE_STR(state), PRECISION(disp_width), disp_width);
        len = strlen(title_str);
        sdl_render_printf(
            pane, pane->w/2 - COL2X(len,FONT_SZ/2), 0,
            FONT_SZ, SDL_WHITE, SDL_BLACK, "%s", title_str);

        // display status
        int row=3;
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "T       %-8.*f", PRECISION(t), t);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "TEMP    %-8.*f", PRECISION(temp), temp);
        sdl_render_printf(pane, 0, ROW2Y(row++,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
                          "VISIBLE %-8d", num_visible);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_CTRL: case ' ':
            // the SDL_EVENT_CTRL or the space-bar are used to
            // control the state of the simulation (resume, pause, reset)
            // xxx needed?
            if (state == RESET || state == PAUSED) {
                sim_resume();
            } else if (state == RUNNING) {
                sim_pause();
            } else {
                sim_reset();
            }
            break;
        case SDL_EVENT_RESET: case 'r':
            // the SDL_EVENT_RESET or 'r', resets the simulation
            sim_reset();
            break;
        case SDL_EVENT_RESET_DW: case 'R':
            // the SDL_EVENT_RESET_DW or 'R', resets the display_width
            disp_width = get_diameter(t,NULL,NULL);
            break;
#if 0
        case SDL_EVENT_ZOOM: ;
            // SDL_EVENT_ZOOM provides fine grain control of the display width
            double dy = -event->mouse_wheel.delta_y;
            if (dy == 0) break;
            if (dy < 0 && disp_width <= 1) dy = -.01;
            if (dy > 0 && disp_width < 1) dy = +.01;
            disp_width += dy;
            if (dy == 1) disp_width = floor(disp_width);
            if (dy == -1) disp_width = ceil(disp_width);
            if (disp_width < .01) disp_width = .01;
            break;
#endif
        case SDL_EVENT_KEY_UP_ARROW:
        case SDL_EVENT_KEY_DOWN_ARROW:
        case SDL_EVENT_KEY_SHIFT_UP_ARROW:
        case SDL_EVENT_KEY_SHIFT_DOWN_ARROW:
            // adjust disp_width
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
        case SDL_EVENT_KEY_SHIFT_LEFT_ARROW: ;
            // adjust t
            double tt = t;
            switch (event->event_id) {
            case SDL_EVENT_KEY_RIGHT_ARROW:       tt *= 1.01; break;
            case SDL_EVENT_KEY_LEFT_ARROW:        tt /= 1.01; break;
            case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW: tt *= 1.1;  break;
            case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:  tt /= 1.1;  break;
            }
            if (tt < T_START) tt = T_START;
            if (tt > 100) tt = 100;
            t = tt;
            disp_width = get_diameter(t,NULL,NULL);  // xxx tracking mode
                // xxx WARNING: time 0.007411 not in diameter tbl
            graph[0].x_cursor = t;
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

int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
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

#define MAX_GP 1000 // xxx

    if (request == PANE_HANDLER_REQ_RENDER) {
        graph_t *g = &graph[vars->graph_idx];
        point_t  points[MAX_GP];
        char str[100];
        int xi, yi;

        // create the array of graph points that are to be displayed
        for (int i = 0; i < g->n; i++) {
            points[i].x = (g->p[i].x / g->max_xval) * pane->w ;
            points[i].y = (pane->h - 1) -
                          ((g->p[i].y / g->max_yval) * (pane->h - ROW2Y(1,FONT_SZ)));
        }
        sdl_render_points(pane, points, g->n, SDL_WHITE, 1);

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
        sdl_render_line(pane, xi-0, pane->h-1, xi-0, ROW2Y(1,FONT_SZ), SDL_WHITE);

        // display title
#if 0
        int x = MAX_GRAPH_POINTS * (t / 100-1e-6);
        int y = (pane->h - 1) - ((g->y[x] / g->max_yval) * (0.85 * pane->h));
        sdl_render_line(pane, x+0, y+20, x+0, y-20, SDL_WHITE);
        sdl_render_line(pane, x-1, y+20, x-1, y-20, SDL_WHITE);
        sdl_render_line(pane, x+1, y+20, x+1, y-20, SDL_WHITE);
#endif

#if 0
        //int x = MAX_GRAPH_POINTS * (g->x / g->max_xval)
        double y = 
        sprintf(str, "%s=%0.*f  %s=%0.*f", 
                g->x_str, PRECISION(g->x), g->x,
                g->y_str, PRECISION(y), y);
        sdl_render_printf(pane, COL2X(8,FONT_SZ), 0, 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%s", str);
#endif

#if 0
        // display the title line xxx needs work
        int t_precision = (t < .001 ? 6 : t < 1 ? 3 : 1);
        sdl_render_printf(
              pane, COL2X(2,FONT_SZ), 0, FONT_SZ,
              SDL_WHITE, SDL_BLACK, 
              "%s %0.*f%s - T %0.*f BYR - YMAX %0.*f",
              g->title, g->precision, g->y[last_i], g->units,
              t_precision, t,
              g->precision, g->max_yval);
#endif


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

