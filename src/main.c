#include "app.h"
#include <SDL3/SDL_main.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static void web_frame(void) {
    mountain_step();
}
#endif

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    if (!mountain_open(NULL, 960, 600)) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            window.mountainError("The trail needs a working WebGL 2 display and the game assets.");
        });
#endif
        return 1;
    }
#ifdef __EMSCRIPTEN__
    EM_ASM({ window.mountainReady(); });
    emscripten_set_main_loop(web_frame, 0, 1);
#else
    while (mountain_step()) {
    }
    mountain_close();
#endif
    return 0;
}
