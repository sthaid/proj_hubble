#include <common.h>

//
// defines
//

#define DEG2RAD(deg)  ((deg) * (M_PI / 180))

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

//
// variables
//

static int     win_width, win_height;

static state_t state;
static double  t_done;
static double  disp_width;

static double  t;
static double  d_start;
static double  d_space;
static double  d_photon;

graph_t graph[4];

//
// prototypes
//

static void sim_init(void);
static void *sim_thread(void *cx);
static void sim_reset(void);
static void sim_pause(void);
static void sim_resume(void);

static void display_init(void);
static void display_hndlr(void);
static void display_start(void *cx);
static int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);

static double get_temperature(double t);

// -----------------  MAIN  ---------------------------------------
  
int main(int argc, char **argv)
{
    // init
    sf_init();
    sim_init();
    display_init();

    // runtime
    display_hndlr();
}

// -----------------  CMB SIM  ------------------------------------

static void sim_init(void)
{
    pthread_t tid;

    // reset the simulation
    sim_reset();

    // create the cmd_sim_thread
    pthread_create(&tid, NULL, sim_thread, NULL);
}

static void *sim_thread(void *cx)
{
    double d_photon_si, t_si, h_si;
    double sf, sf_t_start;
    double temp;
    int count = 0;
    graph_t *gr;

    sf_t_start = get_sf(T_START);

    while (true) {
        // poll for state == RUNNING
        while (state != RUNNING) {
            usleep(10000);
        }

        // convert values to SI units
        d_photon_si = d_photon * M_PER_BLYR;
        t_si = t * S_PER_BYR;
        h_si = get_hsi(t_si);
        
        // update the distance of the photon from the hypothetical
        // perpetual earth, for DELTA_T_SECS time increment
        d_photon_si += (-c_si + h_si * d_photon_si) * DELTA_T_SECS;
        t_si += DELTA_T_SECS;

        // convert values back to BLYR and BYR units
        d_photon = d_photon_si / M_PER_BLYR;
        t = t_si / S_PER_BYR;

        // determine, for the new time 't':
        // - the distance of the patch of space, from which the
        //   CMB was emitted, to the perpetual earth, 
        // - the temperature of the CMB
        // these are both determined by the amount the universe
        // has expanded form T_START (.000380 BYR) to time 't'
        sf = get_sf(t);
        d_space = d_start * (sf / sf_t_start);
        temp = TEMP_START / (sf / sf_t_start);

        // save values to be displayed in the 4 graphs
        int gridx = (t / t_done) * MAX_GRAPH_POINTS;
        if (gridx >= 0 && gridx < MAX_GRAPH_POINTS) {
            gr = &graph[0];
            gr->p[gridx].x = t;
            gr->p[gridx].y = sf;
            gr->n = gridx+1;

            gr = &graph[1];
            gr->p[gridx].x = t;
            gr->p[gridx].y = get_h(t);
            gr->n = gridx+1;

            gr = &graph[2];
            gr->p[gridx].x = t;
            gr->p[gridx].y = temp;
            gr->n = gridx+1;

            gr = &graph[3];
            gr->p[gridx].x = t;
            gr->p[gridx].y = d_photon;
            gr->n = gridx+1;
        }

        // if the CMB photon has reached the earth, then we're done
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

    return NULL;
}

static void sim_reset(void)
{
    graph_t *gr;
    double initial_distance, max_photon_distance;

    if (t_done == 0 || disp_width == 0) {
        t_done = 13.8;
        disp_width = get_diameter(t_done, NULL, NULL);
    }

    state = RESET;

    get_diameter(t_done, &initial_distance, &max_photon_distance);

    t            = T_START;
    d_start      = initial_distance;
    d_photon     = initial_distance;
    d_space      = initial_distance;

    // clear graph data in preparation for re-init below
    memset(graph,0,sizeof(graph));

    // init scale factor graph
    gr = &graph[0];
    gr->exists    = true;
    gr->max_xval  = t_done;
    gr->max_yval  = get_sf(t_done);
    gr->x_cursor  = NO_VALUE;
    gr->p[0].x    = T_START;
    gr->p[0].y    = get_sf(T_START);
    gr->n         = 1;

    gr = &graph[1];
    gr->exists    = true;
    gr->max_xval  = t_done;
    gr->max_yval  = 1000;  // km/s/mpc
    gr->x_cursor  = NO_VALUE;
    gr->p[0].x    = T_START;
    gr->p[0].y    = get_h(T_START);
    gr->n         = 1;

    gr = &graph[2];
    gr->exists    = true;
    gr->max_xval  = t_done;
    gr->max_yval  = 100;  // degrees k
    gr->x_cursor  = NO_VALUE;
    gr->p[0].x    = T_START;
    gr->p[0].y    = get_temperature(T_START);
    gr->n         = 1;

    gr = &graph[3];
    gr->exists    = true;
    gr->max_xval  = t_done;
    gr->max_yval  = max_photon_distance;
    gr->x_cursor  = NO_VALUE;
    gr->p[0].x    = T_START;
    gr->p[0].y    = d_photon;
    gr->n         = 1;
}

static void sim_pause(void)
{
    if (state == RUNNING) {
        state = PAUSED;
    }
}

static void sim_resume(void)
{
    if (state == RESET || state == PAUSED) {
        state = RUNNING;
    }
}

// -----------------  DISPLAY  ------------------------------------

static void display_init(void)
{
    bool fullscreen = false;
    bool resizeable = false;
    bool swap_white_black = false;

    // init sdl, and get actual window width and height
    win_width  = WIN_WIDTH;
    win_height = WIN_HEIGHT;
    if (sdl_init(&win_width, &win_height, fullscreen, resizeable, swap_white_black) < 0) {
        printf("ERROR sdl_init %dx%d failed\n", win_width, win_height);
        exit(1);
    }
    printf("REQUESTED win_width=%d win_height=%d\n", WIN_WIDTH, WIN_HEIGHT);
    printf("ACTUAL    win_width=%d win_height=%d\n", win_width, win_height);

    // if created window does not have the requested size then exit
    if (win_width != WIN_WIDTH || win_height != WIN_HEIGHT) {
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

// this routine is called prior to pane handlers
static void display_start(void *cx)
{
    double sf, h, temp, d_photon;
    graph_t *gr;

    // initialize the graphs cursor x,y values
    gr = &graph[0];
    assert(gr->n > 0 && gr->n <= MAX_GRAPH_POINTS);
    sf = gr->p[gr->n-1].y;
    sprintf(gr->title, "T=%.*f  SF=%.*f", PRECISION(t), t, PRECISION(sf), sf);

    gr = &graph[1];
    assert(gr->n > 0 && gr->n <= MAX_GRAPH_POINTS);
    h = gr->p[gr->n-1].y;
    sprintf(gr->title, "T=%.*f  H=%.*f", PRECISION(t), t, PRECISION(h), h);

    gr = &graph[2];
    assert(gr->n > 0 && gr->n <= MAX_GRAPH_POINTS);
    temp = gr->p[gr->n-1].y;
    sprintf(gr->title, "T=%.*f  TEMPERATURE=%.*f", PRECISION(t), t, PRECISION(temp), temp);

    gr = &graph[3];
    assert(gr->n > 0 && gr->n <= MAX_GRAPH_POINTS);
    d_photon = gr->p[gr->n-1].y;
    if (d_photon < 0) d_photon = 0;
    sprintf(gr->title, "T=%.*f  D_PHOTON=%.*f", PRECISION(t), t, PRECISION(d_photon), d_photon);
}

// - - - - - - - - -  MAIN PANE HNDLR  - - - - - - - - - - - - - - - - 

static int main_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_CTRL             (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_ZOOM             (SDL_EVENT_USER_DEFINED + 1)
    #define SDL_EVENT_RESET            (SDL_EVENT_USER_DEFINED + 2)

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
        double temp = get_temperature(t);

        // display yellow background, the intensity represents the temperature
        yidx = log(temp) * (255. / 8.);
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;
        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        // display blue point at center, representing location of the observer
        sdl_render_point(pane, pane->w/2, pane->h/2, SDL_LIGHT_BLUE, 6);

        // display points surrouding the observer; 
        // these points represent the position of the CMB photons as they move due to
        // their velocity (c) toward the observer and the expansion of space
        // which is away from the observer
        for (int deg = 0; deg < 360; deg += 45) {
            int x,y;
            x = pane->w/2 + d_photon * (pane->w / disp_width) * cos(DEG2RAD(deg));
            y = pane->h/2 + d_photon * (pane->h / disp_width) * sin(DEG2RAD(deg));
            sdl_render_point(pane, x, y, SDL_RED, 3);
        }

        // display a circle around the observer, representing the current position
        // of the space from which the CMB photons originated
        sdl_render_circle(
            pane, 
            pane->w / 2, pane->h / 2,
            d_space * (pane->w / disp_width),
            5, SDL_RED);

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
                pane, pane->w-COL2X(5,FONT_SZ), ROW2Y(0,FONT_SZ), FONT_SZ, 
                "RESET", SDL_LIGHT_BLUE, SDL_BLACK, 
                SDL_EVENT_RESET, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // register the SDL_EVENT_ZOOM which is used to adjust the 
        // display width scale using the mouse wheel
        sdl_register_event(pane, &(rect_t){0,0,pane->w,pane->h}, 
            SDL_EVENT_ZOOM, SDL_EVENT_TYPE_MOUSE_WHEEL, pane_cx);

        // display current state at top middle
        sprintf(title_str, "DISPLAY_WIDTH=%0.*f",\
                PRECISION(disp_width), disp_width);
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
            PRECISION(temp), temp);
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
        case SDL_EVENT_CTRL: case 'x':
            // the SDL_EVENT_CTRL or the space-bar are used to
            // control the state of the simulation (resume, pause, reset)
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
        case SDL_EVENT_KEY_UP_ARROW:
        case SDL_EVENT_KEY_DOWN_ARROW: ;
        case SDL_EVENT_KEY_SHIFT_UP_ARROW:
        case SDL_EVENT_KEY_SHIFT_DOWN_ARROW: ;
            // the up/down arrow keys provide control of the display width
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
            // the left/right arrow keys adjust t_done
            if (state == RESET) {
                double e = 1e-6;
                switch (event->event_id) {
                case SDL_EVENT_KEY_RIGHT_ARROW:       t_done += .1;    break;
                case SDL_EVENT_KEY_LEFT_ARROW:        t_done -= .1;    break;
                case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW: t_done += 1;  break;
                case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:  t_done -= 1;  break;
                }
                if (t_done < .1+e) t_done = .1;
                if (t_done > T_MAX-e) t_done = T_MAX;
                sim_reset();
                disp_width = get_diameter(t_done, NULL, NULL);
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

// -----------------  SUPPORT UTILS  ---------------------------------------

static double get_temperature(double t)
{
    double temp;

    temp = TEMP_START / (get_sf(t) / get_sf(T_START));
    return temp;
}

