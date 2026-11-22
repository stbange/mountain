#include "app.h"
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>
#include "walk.h"

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
static bool expedition_walk(int id) {
    REQUIRE(mountain_visit(id));
    const int pass = (int)mountain_value(32), ascent = (int)mountain_value(31);
    const int count = (int)mountain_value(20), stop = (int)mountain_value(21);
    const int first = ascent ? count - 1 : stop > 3 ? stop / 2 : 1;
    for (int i = 1; i < count; i++) {
        REQUIRE(walk_to(mountain_path(i, 0), mountain_path(i, 1)));
        if (i == first)
            REQUIRE(mountain_value(13) == NEAR_READ && mountain_action(ACT_USE) &&
                    mountain_value(30) == 1);
        if (i == stop && !ascent) {
            double x = mountain_path(i, 0), z = mountain_path(i, 1);
            double yaw = atan2(mountain_path(count - 1, 0) - x, z - mountain_path(count - 1, 1));
            if (id == SURVEYED || id == DUNES || id == FUNGAL) {
                mountain_conditions(1, 0, 12);
                REQUIRE(!mountain_action(ACT_USE));
                mountain_conditions(1, 0, 17.5f);
                REQUIRE(mountain_action(ACT_USE));
            } else if (id == BASALT || id == KARST || id == PRISM || id == CINDER) {
                for (int n = 0; n < 3; n++) {
                    REQUIRE(walk_to(x + 3.5 * cos(yaw) + (n - 1) * 3.2 * sin(yaw),
                                    z + 3.5 * sin(yaw) - (n - 1) * 3.2 * cos(yaw)));
                    REQUIRE(mountain_action(ACT_USE) == (n == 2));
                }
                REQUIRE(walk_to(x, z));
            } else {
                mountain_look((float)(yaw + PI - mountain_value(2)), 0);
                REQUIRE(!mountain_action(ACT_USE));
                REQUIRE(mountain_observe(PI));
            }
            REQUIRE(mountain_value(30) == 2);
        }
    }
    if (ascent) {
        for (int k = 1; k < ascent; k++) {
            REQUIRE(walk_to(mountain_ascent(k, 0), mountain_ascent(k, 1)));
            if (k == pass) {
                double yaw = atan2(mountain_ascent(ascent - 1, 0) - mountain_value(0),
                                   mountain_value(1) - mountain_ascent(ascent - 1, 1));
                mountain_look((float)(yaw - mountain_value(2)), 0);
                REQUIRE(mountain_action(ACT_USE) && mountain_value(30) == 2);
            }
        }
    }
    REQUIRE(mountain_value(35) == 0 && mountain_value(13) == NEAR_TREASURE);
    REQUIRE(mountain_action(ACT_USE) && mountain_value(30) == 3);
    mountain_frame();
    REQUIRE(display_sample(480, 300));
    if (ascent) {
        double dx = mountain_ascent(ascent - 1, 0) - mountain_ascent(0, 0);
        double dz = mountain_ascent(ascent - 1, 1) - mountain_ascent(0, 1);
        mountain_look((float)(atan2(dx, -dz) - mountain_value(2)), 0);
        bool warned = false;
        for (int k = 0; k < 150 && mountain_value(33) != 2; k++) {
            mountain_walk(.045f, 1, 0);
            warned |= mountain_value(33) == 1;
        }
        REQUIRE(warned && mountain_value(33) == 2);
        for (int k = 0; k < 14; k++)
            mountain_walk(.1f, 0, 0);
        REQUIRE(mountain_value(35) == 1 && mountain_value(33) == 3 && mountain_value(30) == 3);
        for (int k = ascent - 2; k >= 0; k--)
            REQUIRE(walk_to(mountain_ascent(k, 0), mountain_ascent(k, 1)));
    }
    return true;
}

static bool shelter_playtest(int id) {
    REQUIRE(mountain_visit(id));
    mountain_conditions(1, 0, 12);
    REQUIRE(mountain_action(ACT_CABIN) && mountain_value(13) == NEAR_OPEN);
    mountain_frame();
    uint32_t closed = display_sample(480, 300);
    REQUIRE(mountain_action(ACT_USE) && mountain_value(13) == NEAR_ENTER);
    mountain_frame();
    REQUIRE(display_sample(480, 300) != closed);
    REQUIRE(mountain_action(ACT_USE) && mountain_value(11));
    REQUIRE(*mountain_label(12) && *mountain_label(13) && *mountain_label(14));
    REQUIRE(mountain_action(ACT_LOOKOUT_PREDATORS) && mountain_action(ACT_LOOKOUT_GIANTS));
    for (int mode = 1; mode <= 3; mode++) {
        REQUIRE(mountain_action(ACT_LOOKOUT_WEATHER) && mountain_value(39) == mode &&
                *mountain_option(2, mode));
        mountain_walk(.12f, 0, 0);
        mountain_frame();
        REQUIRE(display_sample(480, 300) && !mountain_value(33));
    }
    REQUIRE(mountain_action(ACT_HOME) && mountain_action(ACT_USE) && !mountain_value(11));
    REQUIRE(mountain_action(ACT_LOOKOUT_WEATHER) && mountain_action(ACT_LOOKOUT_PREDATORS) &&
            mountain_action(ACT_LOOKOUT_GIANTS) && mountain_action(ACT_CABIN));
    return true;
}
static bool kitchen_playtest(void) {
    REQUIRE(mountain_visit(0));
    double bearing = mountain_value(2);
    REQUIRE(walk_to(7 * sin(bearing), -7 * cos(bearing)) && mountain_value(13) == NEAR_FORAGE);
    press(SDLK_E);
    REQUIRE(mountain_stock(1, 0) >= 2 && mountain_value(13) != NEAR_FORAGE);
    press(SDLK_R);
    press(SDLK_Q);
    press(SDLK_1);
    press(SDLK_2);
    int coffee = (int)mountain_value(53), cookies = (int)mountain_value(54);
    REQUIRE(mountain_recipe_value(coffee, 2) == 2 && mountain_recipe_value(cookies, 2) == 3);
    press(SDLK_V);
    REQUIRE(mountain_value(13) == NEAR_CHAIR);
    press(SDLK_E);
    REQUIRE(mountain_value(49));
    double x = mountain_value(0), z = mountain_value(1);
    walk(10, 1, 1);
    REQUIRE(mountain_value(0) == x && mountain_value(1) == z && mountain_photo());
    mountain_frame();
    uint32_t served = display_sample(480, 300);
    press(SDLK_1);
    press(SDLK_2);
    press(SDLK_2);
    press(SDLK_2);
    REQUIRE(!mountain_value(48) && mountain_value(47) == coffee);
    press(SDLK_1);
    REQUIRE(!mountain_value(47));
    mountain_frame();
    REQUIRE(display_sample(480, 300) != served);
    press(SDLK_E);
    REQUIRE(!mountain_value(49));
    press(SDLK_R);
    REQUIRE(!mountain_value(12));
    REQUIRE(mountain_visit(LUNAR) && mountain_stock(1, 0) >= 2 &&
            mountain_recipe_value(cookies, 1));
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
    for (int i = 0; i < 3; i++) {
        for (int step = 0; step < 320 && mountain_value(13) != NEAR_ROOM_DOOR; step++)
            walk(1, 1, 0);
        REQUIRE(mountain_value(13) == NEAR_ROOM_DOOR && mountain_value(38) == i + 1);
        walk(24, 1, 0);
        REQUIRE(mountain_value(12) == i);
        press(SDLK_E);
        REQUIRE(mountain_value(12) == i && mountain_value(38) == i + 2);
        mountain_frame();
        REQUIRE(display_sample(480, 300));
        walk(14, 1, 0);
        REQUIRE(mountain_value(12) == i + 1);
    }
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
    REQUIRE(mountain_sound());
    for (int i = 0; i < (int)mountain_value(29); i++) {
        if (i == PRISM)
            REQUIRE(!mountain_sound());
        REQUIRE(expedition_walk(i));
    }
    REQUIRE((uint32_t)mountain_value(34) == 0xffffff);
    for (int i = 0; i < (int)mountain_value(29); i++)
        REQUIRE(shelter_playtest(i));
    REQUIRE(kitchen_playtest());
    return true;
}
int main(int argc, char **argv) {
    isolated_journal();
    if (argc != 2 || !mountain_open(argv[1], 480, 300))
        return 1;
    bool passed = run();
    mountain_close();
    remove_test_journal();
    if (passed)
        puts("Live game passed. All twelve treasure hunts, the summit climb and fall recovery "
             "passed. Both mountains supported a five-kilometre round trip, distant "
             "wildlife "
             "and a remote cabin visit. All twelve destination shelters, encounters and "
             "photography passed. Foraging, brewing, baking and balcony dining passed.");
    return passed ? 0 : 1;
}
