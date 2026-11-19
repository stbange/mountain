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
static bool walk_to(double x, double z) {
    double dx = x - mountain_value(0), dz = z - mountain_value(1);
    mountain_look((float)(atan2(dx, -dz) - mountain_value(2)), 0);
    double remaining = hypot(dx, dz);
    while (remaining > .0001) {
        double step = fmin(remaining, .8);
        mountain_walk((float)(step / 5.3), 1, 0);
        remaining -= step;
        REQUIRE(!mountain_wet(mountain_value(0), mountain_value(1)));
    }
    REQUIRE(hypot(x - mountain_value(0), z - mountain_value(1)) < .005);
    return true;
}
static bool walk_route(uint32_t seed) {
    mountain_seed(seed);
    int count = (int)mountain_value(20), stop = (int)mountain_value(21);
    REQUIRE(stop > 0 && stop < count - 1);
    for (int i = 1; i < count; i++) {
        REQUIRE(walk_to(mountain_path(i, 0), mountain_path(i, 1)));
        if (i == stop || i == count - 1) {
            mountain_frame();
            REQUIRE(display_sample(480, 300) != 0);
        }
    }
    REQUIRE(fabs(mountain_value(15) - mountain_value(22)) < .02);
    double ridge_x = mountain_value(0), ridge_z = mountain_value(1);
    float ridge_height = mountain_height(ridge_x, ridge_z);
    int sightings = 0;
    for (int i = 1; i <= 104; i++) {
        REQUIRE(walk_to(mountain_extension(i, 0), mountain_extension(i, 1)));
        if (i % 16 == 0)
            sightings |= mountain_sightings();
    }
    REQUIRE(sightings && mountain_value(27) > 2300 && fabs(mountain_value(26) - 104) < .001);
    mountain_frame();
    REQUIRE(display_sample(480, 300) != 0);
    REQUIRE(mountain_action(ACT_CABIN) && mountain_action(ACT_USE) && mountain_action(ACT_USE));
    REQUIRE(mountain_value(11) && mountain_action(ACT_HOME));
    REQUIRE(mountain_action(ACT_USE) && !mountain_value(11));
    REQUIRE(mountain_action(ACT_CABIN));
    for (int i = 103; i >= 0; i--)
        REQUIRE(walk_to(mountain_extension(i, 0), mountain_extension(i, 1)));
    REQUIRE(hypot(ridge_x - mountain_value(0), ridge_z - mountain_value(1)) < .005);
    REQUIRE(fabs(ridge_height - mountain_height(ridge_x, ridge_z)) < .001);
    REQUIRE(mountain_value(15) > 5000);
    return true;
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
    REQUIRE(walk_route((uint32_t)mountain_value(23)));
    REQUIRE(walk_route(4812));
    return true;
}
int main(int argc, char **argv) {
    if (argc != 2 || !mountain_open(argv[1], 480, 300))
        return 1;
    bool passed = run();
    mountain_close();
    if (passed)
        puts("Live game passed. Both mountains supported a five-kilometre round trip, distant "
             "wildlife "
             "and a remote cabin visit. Doorways, room travel, conditions and photography passed.");
    return passed ? 0 : 1;
}
