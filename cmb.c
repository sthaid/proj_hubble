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
//
// typdedefs
// 

typedef enum {STOPPED, RUNNING, DONE} state_t;

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

#define TEMPERATURE(_t)  (3000. * (get_sf(.00038) / get_sf(_t)))

void * cmb_sim_thread(void *cx)
{
    double d_si, t_si, h_si;

    while (true) {
        // poll for state == RUNNING
        while (state != RUNNING) {
            usleep(10000);
        }

        if (d == d_start) {
            double d_cmb_origion = d_start * get_sf(t) / get_sf(.000380);
            printf("START t=%f  d_photon=%f  d_cmb_origion=%f  temperature=%f\n", t, d, d_cmb_origion, TEMPERATURE(t));
        }

        // xxx comments
        d_si = d * M_PER_BLYR;
        t_si = t * S_PER_BYR;
        h_si = get_hsi(t_si);
        
        d_si += (c_si + h_si * d_si) * DELTA_T_SECS;
        t_si += DELTA_T_SECS;

        d = d_si / M_PER_BLYR;
        t = t_si / S_PER_BYR;

        // xxx comments
        if (d >= 0) {
            state = DONE;
            double d_cmb_origion = d_start * get_sf(t) / get_sf(.000380);
            printf("DONE t=%f  d_photon=%f  d_cmb_origion=%f  temperature=%f\n", t, d, d_cmb_origion, TEMPERATURE(t));
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
            double d_cmb_origion = d_start * get_sf(t) / get_sf(.000380);
            printf("t=%f  d_photon=%f  d_cmb_origion=%f  temperature=%f\n", t, d, d_cmb_origion, TEMPERATURE(t));
            count = 0;
        }
    }
}

void sim_reset(void)
{
    state = STOPPED;

    t_done = 13.8;  // xxx make adjustable
    get_diameter(t_done, &d_start);
    t = .000380;  // xxx define
    d = d_start;
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
    // call the pane manger; 
    // - this will not return except when it is time to terminate the program
    sdl_pane_manager(
        NULL,           // context
        NULL,           // called prior to pane handlers
        NULL,           // called after pane handlers
        100000,         // 0=continuous, -1=never, else us
        1,              // number of pane handler varargs that follow
        main_pane_hndlr, NULL,
            0, 0, win_height, win_height,
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

    static int yellow[256];
    int i;

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        INFO("PANE x,y,w,h  %d %d %d %d\n",
            pane->x, pane->y, pane->w, pane->h);

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

        yidx = 30 + temperature * ((255. - 30) / 3000);
        if (yidx < 0) yidx = 0;
        if (yidx > 255) yidx = 255;

        sdl_render_fill_rect(pane, &(rect_t){0,0,pane->w, pane->h}, yellow[yidx]);

        ctrl_str = (state == STOPPED && t == .00038 ? "RUN"    :
                    state == STOPPED                ? "RESUME" :
                    state == RUNNING                ? "PAUSE"  :
                    state == DONE                   ? "RESET"  :
                                                      (assert(0), ""));

        sdl_render_text_and_register_event(
            pane, 0, 0, FONT_SZ, ctrl_str, SDL_LIGHT_BLUE, SDL_BLACK, 
            SDL_EVENT_CTRL, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        sdl_render_printf(pane, 0, ROW2Y(1,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
            "STATE = %s", 
            STATE_STR(state));

        sdl_render_printf(pane, 0, ROW2Y(3,FONT_SZ), FONT_SZ, SDL_WHITE, SDL_BLACK, 
            "TIME=%0.6f  D=%0.6f  TEMP=%0.1f",
            t, d, temperature);

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

