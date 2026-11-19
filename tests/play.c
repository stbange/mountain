#include "app.h"
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>

#define REQUIRE(x)                                                                                 \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "Playtest failed at %s:%d: %s\n", __FILE__, __LINE__, #x);             \
            return false;                                                                          \
        }                                                                                          \
    } while (0)
static void press(SDL_Keycode key) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    SDL_PushEvent(&event);
    mountain_step();
}
static void walk(int frames, float forward, float right) {
    for (int i = 0; i < frames; i++)
        mountain_walk(.05f, forward, right);
}
static bool run(void) {
    mountain_frame();
    uint32_t trail = display_sample(480, 300);
    press(SDLK_H);
    mountain_frame();
    uint32_t closed = display_sample(480, 300);
    REQUIRE(mountain_value(13) == NEAR_OPEN && trail != closed);
    walk(40, 1, 0);
    REQUIRE(!mountain_value(11));
    press(SDLK_E);
    mountain_frame();
    REQUIRE(mountain_value(10) && closed != display_sample(480, 300));
    walk(4, 1, 0);
    REQUIRE(mountain_value(11));
    press(SDLK_H);
    REQUIRE(mountain_value(9));
    mountain_look(.16f, 0);
    walk(160, 1, 0);
    REQUIRE(mountain_value(12) >= 3);
    press(SDLK_R);
    REQUIRE(mountain_value(12) == 0);
    press(SDLK_E);
    REQUIRE(!mountain_value(11));
    press(SDLK_H);
    REQUIRE(!mountain_value(9));
    int old = (int)mountain_value(5);
    press(SDLK_Y);
    REQUIRE(mountain_value(5) == (old + 1) % 4);
    press(SDLK_T);
    press(SDLK_U);
    press(SDLK_F1);
    press(SDLK_ESCAPE);
    press(SDLK_N);
    mountain_conditions(3, 2, 23);
    mountain_frame();
    REQUIRE(display_sample(480, 300) != trail);
    REQUIRE(mountain_photo());
    return true;
}
int main(int argc, char **argv) {
    if (argc != 2 || !mountain_open(argv[1], 480, 300))
        return 1;
    bool passed = run();
    mountain_close();
    if (passed)
        puts("Live game passed: collision, visible doorway, walking across three rooms, returning "
             "home, conditions and photograph.");
    return passed ? 0 : 1;
}
