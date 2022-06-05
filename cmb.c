// xxx
// - keys for pause and continue and reset
// - seperate reset button
// - main pain prints
// - comments
// - review
// - README
// - macro for d_cmb_orig,  and call it D_ORIGIN

#include <common.h>
#include <util_sdl.h>

//
// defines
//

#define FONT_SZ 20

#define STATE_STR(s) \
    ((s) == STOPPED ? "STOPPED" : \
     (s) == RUNNING ? "RUNNING" : \
     (s) == DONE    ? "DONE"    : \
                      (assert(0),""))

#define DEG2RAD(deg)  ((deg) * (M_PI / 180))

#define MAX_GRAPH_POINTS 1000

#define TEMPERATURE(_t)  (3000. * (get_sf(.00038) / get_sf(_t)))

//
// typdedefs
// 

typedef enum {STOPPED, RUNNING, DONE} state_t;

typedef struct {
    char *title;
    char *units;
    int precision;
    double max_yval;
    double y[MAX_GRAPH_POINTS];
} graph_t;

//
// variables
//

int win_width, win_height;
double disp_width = 93;   // xxx init someplace else

state_t state;
double  t_done;
double  d_start;
double  t;
double  d;

graph_t graph[4];

//
// prototypes
//

void cmb_sim_init(void);
void *cmb_sim_thread(void *cx);
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
    cmb_sim_init();
    display_init();

    // runtime
    display_hndlr();
}

// -----------------  CMB SIM  ------------------------------------

void cmb_sim_init(void)
{
    pthread_t tid;

    // reset the simulation
    sim_reset();

    // create the cmd_sim_thread
    pthread_create(&tid, NULL, cmb_sim_thread, NULL);
}

void * cmb_sim_thread(void *cx)
{
    double d_si, t_si, h_si;

    while (true) {
        // poll for state == RUNNING
        while (state != RUNNING) {
            usleep(10000);
        }

        if (d == d_start) {
            // xxx macro for this
            double d_cmb_origin = d_start * get_sf(t) / get_sf(.000380);
            printf("START t=%f  d_photon=%f  d_cmb_origin=%f  temperature=%f\n", t, d, d_cmb_origin, TEMPERATURE(t));
        }

        // xxx comments
        d_si = d * M_PER_BLYR;
        t_si = t * S_PER_BYR;
        h_si = get_hsi(t_si);
        
        d_si += (c_si + h_si * d_si) * DELTA_T_SECS;
        t_si += DELTA_T_SECS;

        d = d_si / M_PER_BLYR;
        t = t_si / S_PER_BYR;

        // xxx these distances should be > 0
        double d_cmb_origin = d_start * get_sf(t) / get_sf(.000380);

        // xxx   sf, temp, dist
        int idx = (t / t_done) * MAX_GRAPH_POINTS;
        if (idx >= 0 && idx < MAX_GRAPH_POINTS) {
            graph[0].y[idx] = get_sf(t);
            graph[1].y[idx] = TEMPERATURE(t);
            graph[2].y[idx] = -d;
            graph[3].y[idx] = -d_cmb_origin;
        }

        // xxx comments
        if (d >= 0) {
            state = DONE;
            //double d_cmb_origin = d_start * get_sf(t) / get_sf(.000380);
            printf("DONE t=%f  d_photon=%f  d_cmb_origin=%f  temperature=%f\n", t, d, d_cmb_origin, TEMPERATURE(t));
            continue;
        }

        // delay to adjust rate xxx 
        static int count1;
        if (++count1 == 30) {
            usleep(100);
            count1 = 0;
        }

        static int count;
        if (count++ == 100000) {
            double d_cmb_origin = d_start * get_sf(t) / get_sf(.000380);
            printf("t=%f  d_photon=%f  d_cmb_origin=%f  temperature=%f\n", t, d, d_cmb_origin, TEMPERATURE(t));
            count = 0;
        }
    }
}

void sim_reset(void)
{
    graph_t *g;

    state = STOPPED;

    t_done = 13.8;  // xxx make adjustable
    get_diameter(t_done, &d_start);
    t = .000380;  // xxx define
    d = d_start;

    for (int i = 0; i < 4; i++) {
        memset(graph[i].y, 0, sizeof(graph[i].y));
    }

    g = &graph[0];
    g->max_yval  = get_sf(t_done);
    g->title     = "SCALE_FACTOR";
    g->units     = "";
    g->precision = 3;
    g->y[0]      = get_sf(.000380);

    g = &graph[1];
    g->max_yval  = 100;
    g->title     = "TEMP";
    g->units     = " K";
    g->precision = 1;
    g->y[0]      = TEMPERATURE(.00038);

    g = &graph[2];
    g->max_yval  = 8;  //xxx
    g->title     = "PHOTON";
    g->units     = " BLYR";
    g->precision = 3;
    g->y[0]      = -d_start;

    g = &graph[3];
    g->max_yval  = 50;  //xxx
    g->title     = "ORIGIN";
    g->units     = " BLYR";
    g->precision = 3;
    double d_cmb_orig = -d_start * (get_sf(t) / get_sf(.00038));  //xxx
    g->y[0]      = d_cmb_orig;
}

void sim_pause(void)
{
    if (state == RUNNING) {
        state = STOPPED;
    }
}

void sim_resume(void)
{
    if (state == STOPPED) {
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
        ERROR("sdl_init %dx%d failed\n", win_width, win_height);
        exit(1);
    }
    INFO("REQUESTED win_width=%d win_height=%d\n", REQUESTED_WIN_WIDTH, REQUESTED_WIN_HEIGHT);
    INFO("ACTUAL    win_width=%d win_height=%d\n", win_width, win_height);

    // xxx fail if size is wrong
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
        main_pane_hndlr, NULL,
            0, 0, win_width/2, win_height,
            PANE_BORDER_STYLE_MINIMAL,
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
            PANE_BORDER_STYLE_MINIMAL
        );

}

int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_CTRL       (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_ZOOM       (SDL_EVENT_USER_DEFINED + 1)

    static int yellow[256];
    int i;

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        INFO("MAIN_PANE x,y,w,h  %d %d %d %d\n",
            pane->x, pane->y, pane->w, pane->h);

        assert(pane->w == pane->h);

        for (i = 0; i < 256; i++) {
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
        double temperature = TEMPERATURE(t);
        char *ctrl_str;

#if 0
        yidx = 50 + temperature * ((255. - 50) / 3000);
#else
        yidx = log(temperature) * (255. / 8.);
#endif
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;
        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        sdl_render_point(pane, pane->w/2, pane->h/2, SDL_LIGHT_BLUE, 8);

        for (int deg = 0; deg < 360; deg += 45) {
            int x,y;

            x = pane->w/2 + d * (pane->w / disp_width) * cos(DEG2RAD(deg));
            y = pane->h/2 + d * (pane->h / disp_width) * sin(DEG2RAD(deg));

            sdl_render_point(pane, x, y, SDL_BLUE, 3);
        }

        double d_cmb_orig = -d_start * (get_sf(t) / get_sf(.00038));
        sdl_render_circle(pane, 
            pane->w / 2, pane->h / 2,
            d_cmb_orig * (pane->w / disp_width),
            5, SDL_BLUE);


        ctrl_str = (state == STOPPED && t == .00038 ? "RUN"    :
                    state == STOPPED                ? "RESUME" :
                    state == RUNNING                ? "PAUSE"  :
                    state == DONE                   ? "RESET"  :
                                                      (assert(0), ""));

        sdl_render_text_and_register_event(
            pane, 0, 0, FONT_SZ, ctrl_str, SDL_LIGHT_BLUE, SDL_BLACK, 
            SDL_EVENT_CTRL, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        sdl_render_printf(pane, 0, ROW2Y(1,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
            "STATE = %s  DISP_WIDTH=%0.6f",  
            STATE_STR(state), disp_width);

        sdl_render_printf(pane, 0, ROW2Y(3,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
            "TIME=%0.6f  D=%0.6f  TEMP=%0.1f",
            t, d, temperature);

        sdl_register_event(pane, &(rect_t){0,0,pane->w,pane->h}, 
            SDL_EVENT_ZOOM, SDL_EVENT_TYPE_MOUSE_WHEEL, pane_cx);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_CTRL:
            if (state == STOPPED) {
                sim_resume();
            } else if (state == RUNNING) {
                sim_pause();
            } else {
                sim_reset();
            }
            break;
        case SDL_EVENT_ZOOM: ;
            int dy = event->mouse_wheel.delta_y;
            disp_width += dy;
            // xxx clip
            if (disp_width < 1) disp_width = 1;
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

// xxx simplify
int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int graph_idx;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_xxx    (SDL_EVENT_USER_DEFINED + 0)

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        vars->graph_idx = (long)init_params;

        INFO("GRAPH_PANE_%d x,y,w,h  %d %d %d %d\n",
            vars->graph_idx,
            pane->x, pane->y, pane->w, pane->h);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // ------------------------
    // -------- RENDER --------
    // ------------------------

    if (request == PANE_HANDLER_REQ_RENDER) {
        graph_t *g = &graph[vars->graph_idx];
        int      n = 0;
        point_t  points[MAX_GRAPH_POINTS];
        int      last_i = -1;

        for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
            double yval = g->y[i];
            if (yval == 0) continue;

            last_i = i;

            points[n].x = ((double)i / MAX_GRAPH_POINTS) * pane->w;
            points[n].y = (pane->h - 1) -
                          ((yval / g->max_yval) * pane->h);
            n++;
        }

        // xxx assert
        if (last_i == -1) {
            return PANE_HANDLER_RET_NO_ACTION;
        }

        double t = last_i * (t_done / MAX_GRAPH_POINTS);
        if (t == 0) {
            t = .00038;
        }
        int t_precision = (t < .001 ? 6 : t < 1 ? 3 : 1);
        sdl_render_printf(
              pane, COL2X(3,FONT_SZ), 0, FONT_SZ,
              SDL_WHITE, SDL_BLACK, 
              "%s %0.*f%s - T %0.*f BYR - YMAX %0.*f%s",
              g->title, g->precision, g->y[last_i], g->units,
              t_precision, t,
              g->precision, g->max_yval, g->units);

        sdl_render_points(pane, points, n, SDL_WHITE, 1);


#if 0
            sdl_render_printf(pane, COL2X(10,FONT_SZ), 0, FONT_SZ,
              SDL_WHITE, SDL_BLACK, 
              "%s = %0.3f %s  Y_MAX = %0.3f %s",
              title, yval, units, max_yval, units);
#endif

//  TEMP=1500 K - T=12.800 BYR - YMAX=100 K
//  TEMPERATURE 1500 K - T 11 BYR -   YMAX = 100 K
        //int idx = (t / t_done) * MAX_GRAPH_POINTS;

        // xxx try points
        //sdl_render_lines(pane, points, n, SDL_WHITE);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_xxx:
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

