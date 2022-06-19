#include <common.h>

extern graph_t graph[0];

// - - - - - - - - -  GRAPH PANE HNDLR  - - - - - - - - - - - -

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
                          ((gr->p[i].y / gr->max_yval) * (pane->h - ROW2Y(1,FONT_SZ) - 2));
            n++;
        }
        sdl_render_points(pane, points, n, SDL_WHITE, 2);

        // print max_yval
        sdl_render_printf(pane, 0, ROW2Y(0,FONT_SZ), 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%0.*f", PRECISION(gr->max_yval), gr->max_yval);

        // print max_xval
        sprintf(str, "%0.*f", PRECISION(gr->max_xval), gr->max_xval);
        sdl_render_printf(pane, pane->w-COL2X(strlen(str),FONT_SZ), pane->h-ROW2Y(1,FONT_SZ), 
                          FONT_SZ, SDL_WHITE, SDL_BLACK,
                          "%s", str);

        // display cursor
        if (gr->x_cursor != NO_VALUE) {
            xi = (gr->x_cursor / gr->max_xval) * pane->w;
            sdl_render_line(pane, xi-0, pane->h-1, xi-0, ROW2Y(2,FONT_SZ), SDL_WHITE);
        }

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
