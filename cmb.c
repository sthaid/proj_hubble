// xxx
// - keys for pause and continue and reset
// - comments
// - review
// - README
// - vertical drag to adjust size

#include <common.h>
#include <util_sdl.h>

//
// defines
//

#define FONT_SZ 24

#define DEG2RAD(deg)  ((deg) * (M_PI / 180))

#define MAX_GRAPH_POINTS 1000

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
double disp_width;

state_t state;
double  t_done = 13.8;  // xxx define

double  t;
double  d_start;
double  d_space;
double  d_photon;
double  temperature;

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
    double d_photon_si, t_si, h_si;
    int count = 0;

    while (true) {
        // poll for state == RUNNING
        while (state != RUNNING) {
            usleep(10000);
        }

        // xxx comments
        d_photon_si = d_photon * M_PER_BLYR;
        t_si = t * S_PER_BYR;
        h_si = get_hsi(t_si);
        
        d_photon_si += (-c_si + h_si * d_photon_si) * DELTA_T_SECS;
        t_si += DELTA_T_SECS;

        d_photon = d_photon_si / M_PER_BLYR;
        t = t_si / S_PER_BYR;

        d_space = d_start * get_sf(t) / get_sf(T_START);
        temperature = (3000. * (get_sf(T_START) / get_sf(t)));  // xxx define for initial temperature

        int gridx = (t / t_done) * MAX_GRAPH_POINTS;
        if (gridx >= 0 && gridx < MAX_GRAPH_POINTS) {
            graph[0].y[gridx] = get_sf(t);
            graph[1].y[gridx] = temperature;
            graph[2].y[gridx] = d_photon;
            graph[3].y[gridx] = d_space;
        }

        // xxx comments
        if (d_photon <= 0) {
            state = DONE;
            d_photon = 0;
            continue;
        }

        // delay to complete simulation in about 10 seconds
        if (++count >= (150 * (t_done / 13.8))) {
            usleep(1000);
            count = 0;
        }
    }
}

void sim_reset(void)
{
    graph_t *g;
    double diameter, initial_distance, max_photon_distance;

    state = RESET;

    //t_done = 13.8;  // xxx make adjustable
    //t_done = 50.;  // xxx make adjustable

    diameter = get_diameter(t_done, &initial_distance, &max_photon_distance);

    d_start      = initial_distance;
    t            = T_START;
    d_photon     = initial_distance;
    d_space      = initial_distance;
    temperature  = 3000; // xxx

    disp_width = diameter;

    for (int i = 0; i < 4; i++) {
        memset(graph[i].y, 0, sizeof(graph[i].y));
    }

    g = &graph[0];
    g->max_yval  = get_sf(t_done);
    g->title     = "SCALE_FACTOR";
    g->units     = "";
    g->precision = 3;
    g->y[0]      = get_sf(T_START);

    g = &graph[1];
    g->max_yval  = 1000;
    g->title     = "TEMP";
    g->units     = " K";
    g->precision = 1;
    g->y[0]      = temperature;

    g = &graph[2];
    g->max_yval  = max_photon_distance;
    g->title     = "PHOTON";
    g->units     = " BLYR";
    g->precision = 3;
    g->y[0]      = d_photon;

    g = &graph[3];
    g->max_yval  = diameter/2;
    g->title     = "SPACE";
    g->units     = " BLYR";
    g->precision = 3;
    g->y[0]      = d_space;
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

    #define SDL_EVENT_CTRL   (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_ZOOM   (SDL_EVENT_USER_DEFINED + 1)
    #define SDL_EVENT_RESET  (SDL_EVENT_USER_DEFINED + 2)

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
        yidx = log(temperature) * (255. / 8.);
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;
        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        // display blue point at center, representing location of the observer
        sdl_render_point(pane, pane->w/2, pane->h/2, SDL_LIGHT_BLUE, 8);

        // display points surrouding the observer; 
        // these points represent the position of the CMB photons as they move due to
        // their velocity (c) toward the observer and the expansion of space
        // which is away from the observer
        for (int deg = 0; deg < 360; deg += 45) {
            int x,y;
            x = pane->w/2 + d_photon * (pane->w / disp_width) * cos(DEG2RAD(deg));
            y = pane->h/2 + d_photon * (pane->h / disp_width) * sin(DEG2RAD(deg));
            sdl_render_point(pane, x, y, SDL_BLUE, 3);
        }

        // display a circle around the observer, representing the current position
        // of the space from which the CMB photons originated
        sdl_render_circle(
            pane, 
            pane->w / 2, pane->h / 2,
            d_space * (pane->w / disp_width),
            5, SDL_BLUE);

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
        if (state != DONE) {
            sdl_render_text_and_register_event(
                pane, pane->w-COL2X(5,FONT_SZ), 0, FONT_SZ, "RESET", SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_RESET, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);
        }

        // register the SDL_EVENT_ZOOM which is used to adjust the 
        // display width scale using the mouse wheel
        sdl_register_event(pane, &(rect_t){0,0,pane->w,pane->h}, 
            SDL_EVENT_ZOOM, SDL_EVENT_TYPE_MOUSE_WHEEL, pane_cx);

        // display current state at top middle
        sprintf(title_str, "%s  DISP_WIDTH=%0.*f",\
            STATE_STR(state), PRECISION(disp_width), disp_width);
        len = strlen(title_str);
        sdl_render_printf(
            pane, pane->w/2 - COL2X(len,FONT_SZ/2), 0,
            FONT_SZ, SDL_WHITE, SDL_BLACK, "%s", title_str);

        // display status
        //   T_DONE  nn.nn
        //   T       nn.nn
        //   TEMP    nn.nn
        //   PHOTON  nn.nn
        //   SPACE   nn.nn
        sdl_render_printf(
            pane, 0, ROW2Y(2,FONT_SZ),
            FONT_SZ, SDL_WHITE, SDL_BLACK, "T_DONE %-8.*f",
            PRECISION(t_done), t_done);
        sdl_render_printf(
            pane, 0, ROW2Y(3,FONT_SZ),
            FONT_SZ, SDL_WHITE, SDL_BLACK, "T      %-8.*f",
            PRECISION(t), t);
        sdl_render_printf(
            pane, 0, ROW2Y(4,FONT_SZ),
            FONT_SZ, SDL_WHITE, SDL_BLACK, "TEMP   %-8.*f",
            PRECISION(temperature), temperature);
        sdl_render_printf(
            pane, 0, ROW2Y(5,FONT_SZ),
            FONT_SZ, SDL_WHITE, SDL_BLACK, "PHOTON %-8.*f",
            PRECISION(d_photon), d_photon);
        sdl_render_printf(
            pane, 0, ROW2Y(6,FONT_SZ),
            FONT_SZ, SDL_WHITE, SDL_BLACK, "SPACE  %-8.*f",
            PRECISION(d_space), d_space);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_CTRL:
            if (state == RESET || state == PAUSED) {
                if (state == RESET) sim_reset();  // xxx temp
                sim_resume();
            } else if (state == RUNNING) {
                sim_pause();
            } else {
                sim_reset();
            }
            break;
        case SDL_EVENT_RESET:
            sim_reset();
            break;
        case SDL_EVENT_ZOOM: ;
            int dy = event->mouse_wheel.delta_y;
            disp_width += dy;
            // xxx clip
            if (disp_width < 1) disp_width = 1;
            break;
        case SDL_EVENT_KEY_UP_ARROW:
        case SDL_EVENT_KEY_DOWN_ARROW:
            if (state == RESET) {
                double delta;
                double e = 1e-6;
                delta = (t_done < .001-e ? 0.0001 :
                         t_done < .01-e  ? 0.001  :
                         t_done < .1-e   ? 0.01   :
                         t_done < 15-e   ? 0.1    :
                                            1.);
                t_done = t_done + (event->event_id == SDL_EVENT_KEY_UP_ARROW ? delta : -delta);
                if (t_done < .1+e) {
                    t_done = .1;
                }
                //xxx sim_reset();
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

int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int graph_idx;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

// xxx cleanup this routine
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
        int      n = 0;
        point_t  points[MAX_GRAPH_POINTS];
        int      last_i = -1;

        for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
            double yval = g->y[i];
            if (yval == 0) continue;

            last_i = i;

            points[n].x = ((double)i / MAX_GRAPH_POINTS) * pane->w;
            points[n].y = (pane->h - 1) -
                          ((yval / g->max_yval) * (0.85 * pane->h));
            n++;
        }

        // xxx assert
        if (last_i == -1) {
            return PANE_HANDLER_RET_NO_ACTION;
        }

        double t = last_i * (t_done / MAX_GRAPH_POINTS);
        if (t == 0) {
            t = T_START;
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

