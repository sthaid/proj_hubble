#include <common.h>
#include <util_sdl.h>

//
// defines
//

#define T_START .000380   // byr = 380,000 years

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
double  t;
double  d_start;
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
int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);

// -----------------  MAIN  ---------------------------------------
  
int main(int argc, char **argv)
{
    // init
    sf_init();
    cmb_sim_init();
while (1) pause();
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

    // xxx test, set state to running
    state = RUNNING;
}

void * cmb_sim_thread(void *cx)
{
    const double c_si = 3e8;
    double d_si, t_si;

    #define DELTA_T_SECS (1000e-9 * S_PER_BYR)  // 1000 years

    while (true) {
        // poll for state == RUNNING
        while (state != RUNNING) {
            usleep(10000);
        }

        // xxx comments
        d_si = d * M_PER_BLYR;
        t_si = t * S_PER_BYR;
        
        d_si += (c_si + get_hsi(t_si) * d_si) * DELTA_T_SECS;
        t_si += DELTA_T_SECS;

        d = d_si / M_PER_BLYR;
        t = t_si / S_PER_BYR;

        // xxx comments
        if (d >= 0) {
            state = DONE;
            printf("DONE  %f  %f\n", t, d);
            continue;
        }

        // delay to adjust rate xxx 
        static int count;
        if (count++ == 1000) {
            printf("%f  %f\n", t, d);
            count = 0;
        }
    }
}

void sim_reset(void)
{
    state = STOPPED;
    t_done    = (t_done == 0 ? 13.8 : t_done);
    t         = T_START;
    d_start   = -0.042349;   // xxx tbd, func of t_done
    d         = d_start;
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
        graph_pane_hndlr, NULL,
            0, 0, win_width, win_height,
            PANE_BORDER_STYLE_MINIMAL
        );
}

int graph_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_XXX        (SDL_EVENT_USER_DEFINED + 0)

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        INFO("PANE x,y,w,h  %d %d %d %d\n",
            pane->x, pane->y, pane->w, pane->h);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // ------------------------
    // -------- RENDER --------
    // ------------------------

    if (request == PANE_HANDLER_REQ_RENDER) {
        sdl_render_printf(pane, 0, 0, 20, SDL_WHITE, SDL_BLACK, "HELLO");
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
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

