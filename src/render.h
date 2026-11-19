#ifndef MOUNTAIN_RENDER_H
#define MOUNTAIN_RENDER_H
#include "mountain.h"
#include <SDL3/SDL.h>
typedef struct Display Display;
typedef struct {
    int x, y, w, h;
} Viewport;
Display *display_open(const char *assets);
void display_close(Display *d);
void display_draw(Display *d, Script *s, const Game *g, Viewport vp, const Mat *xr_proj,
                  const Mat *xr_view);
void display_native(Display *d, Script *s, const Game *g, int w, int h, bool help, bool clean);
bool display_save(Display *d, Script *s, const Game *g, const char *path);
uint32_t display_sample(int w, int h);
#endif
