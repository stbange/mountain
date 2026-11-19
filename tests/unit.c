#include "app.h"
#include <SDL3/SDL_main.h>
#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int assertions;
#define CHECK(e)                                                                                   \
    do {                                                                                           \
        assertions++;                                                                              \
        if (!(e)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #e);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#define CLOSE(a, b) CHECK(fabs((double)(a) - (double)(b)) < .001)
static void maths(const Rules *rules) {
    CLOSE(clampf(-1, 0, 1), 0);
    CLOSE(clampf(2, 0, 1), 1);
    CLOSE(mixf(1, 3, .25f), 1.5f);
    CLOSE(smooth(0, 2, 1), .5f);
    CHECK(hash32(87) != hash32(88));
    CLOSE(noise2(-2, 3, 19), random2(-2, 3, 19));
    Color a = rgb(0xff8000), b = shade(a, .5f);
    CLOSE(blend(a, b, .5f).r, .75f);
    CLOSE(a.b, 0);
    World first, copy, other;
    world_init(&first, 42, rules);
    world_init(&copy, 42, rules);
    world_init(&other, 43, rules);
    bool differs = false;
    for (int i = -4; i <= 4; i++)
        for (int j = -4; j <= 4; j++) {
            float x = i * 275.3f, z = j * 302.1f;
            float h = height_at(&first, x, z);
            CHECK(isfinite(h));
            CLOSE(h, height_at(&copy, x, z));
            differs |= fabsf(h - height_at(&other, x, z)) > .5f;
        }
    CHECK(differs);
    CLOSE(sunlight_factor(12), 1);
    CLOSE(sunlight_factor(0), 0);
    CHECK(alpenglow(18) > alpenglow(12));
    Mat p = perspective(1, 1.5f, .1f, 2000), v = view_matrix((Vec){1, 2, 3}, 0, 0), id = identity(),
        m = multiply(id, p);
    for (int i = 0; i < 16; i++)
        CLOSE(m.m[i], p.m[i]);
    CLOSE(v.m[12], -1);
    CLOSE(v.m[13], -2);
    CLOSE(v.m[14], -3);
}
static void position(Game *g, float x, float z) {
    Vec p = cabin_world(&g->cabin, x, 0, z);
    g->x = p.x;
    g->z = p.z;
}
static void hiking(const Rules *r) {
    Game g;
    game_init(&g, r, r->seed);
    CHECK(!strcmp(trail_name(&g), "Larkspur Ridge"));
    CHECK(!game_action(&g, ACT_USE));
    CHECK(!game_action(&g, ACT_HOME));
    CHECK(!game_action(&g, (Action)45));
    game_conditions(&g, -1, 5, -2);
    CHECK(g.season == 3 && g.weather == 1);
    CLOSE(g.hour, 22);
    game_action(&g, ACT_TIME);
    CLOSE(g.hour, 2);
    game_action(&g, ACT_SEASON);
    CHECK(g.season == 0);
    game_action(&g, ACT_WEATHER);
    CHECK(g.weather == 2);
    game_look(&g, 26, 5);
    CHECK(fabsf(g.yaw) <= PI);
    CLOSE(g.pitch, 1.33f);
    game_look(&g, 0, -5);
    CLOSE(g.pitch, -1.33f);
    g.weather_clock = r->weather_interval;
    game_tick(&g, .1f, 0, 0);
    CHECK(g.weather != 2);
    CLOSE(g.walked, 0);
    game_tick(&g, 3, 1, 1);
    CLOSE(g.walked, r->pace * .2f);
    float elapsed = g.elapsed;
    game_tick(&g, -2, 0, 0);
    CLOSE(g.elapsed, elapsed);
    CHECK(game_action(&g, ACT_CABIN));
    CHECK(game_near(&g) == NEAR_OPEN);
    Vec p = cabin_world(&g.cabin, 2, 3, 4), q = cabin_local(&g.cabin, p.x, p.z);
    CLOSE(q.x, 2);
    CLOSE(q.z, 4);
    float natural = ground(&g, g.x + 200, g.z + 200);
    CLOSE(natural, height_at(&g.world, g.x + 200, g.z + 200));
    position(&g, 3, 5);
    CHECK(game_near(&g) == NEAR_NONE);
    position(&g, 0, 4.7f);
    g.yaw = g.cabin.angle;
    for (int i = 0; i < 320; i++)
        game_tick(&g, 1.f / 120, 1, 0);
    CHECK(!g.cabin.inside && cabin_local(&g.cabin, g.x, g.z).z >= 4.20f);
    CHECK(game_action(&g, ACT_USE));
    CHECK(game_near(&g) == NEAR_ENTER);
    CHECK(game_action(&g, ACT_USE));
    CHECK(g.cabin.inside && !game_action(&g, ACT_CABIN));
    CLOSE(eye(&g).y, g.cabin.y + 1.65f);
    position(&g, 3, 0);
    CHECK(game_near(&g) == NEAR_NONE);
    position(&g, 0, 0);
    CHECK(game_near(&g) == NEAR_NONE);
    position(&g, 2.8f, 3.6f);
    g.yaw = g.cabin.angle + PI;
    game_tick(&g, .16f, 1, 0);
    CHECK(g.cabin.inside && cabin_local(&g.cabin, g.x, g.z).z < 3.7f);
    position(&g, 0, -3.4f);
    CHECK(game_near(&g) == NEAR_NEXT);
    for (int i = 0; i < 1002; i++) {
        position(&g, 0, -3.5f);
        CHECK(game_action(&g, ACT_USE));
    }
    CHECK(g.cabin.room == 1002);
    position(&g, 0, 3.4f);
    CHECK(game_near(&g) == NEAR_BACK);
    CHECK(game_action(&g, ACT_USE) && g.cabin.room == 1001);
    position(&g, 0, 3.5f);
    g.yaw = g.cabin.angle + PI;
    for (int i = 0; i < 40 && g.cabin.room == 1001; i++)
        game_tick(&g, 1.f / 120, 1, 0);
    CHECK(g.cabin.room == 1000);
    position(&g, 0, -3.5f);
    g.yaw = g.cabin.angle;
    for (int i = 0; i < 40 && g.cabin.room == 1000; i++)
        game_tick(&g, 1.f / 120, 1, 0);
    CHECK(g.cabin.room == 1001);
    CHECK(game_action(&g, ACT_HOME));
    CHECK(game_near(&g) == NEAR_EXIT);
    for (int i = 0; i < 100 && g.cabin.inside; i++)
        game_tick(&g, 1.f / 120, 1, 0);
    CHECK(!g.cabin.inside && game_action(&g, ACT_CABIN) && !g.cabin.present);
}
static uint32_t geometry_hash(const Mesh *m) {
    const unsigned char *bytes = (const unsigned char *)m->v;
    uint32_t hash = 0x811c9dc5u;
    for (size_t i = 0; i < m->count * sizeof(Vertex); i++)
        hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}
static void composition(Script *s) {
    Game g;
    Mesh m = {0};
    game_init(&g, &s->rules, 41);
    landscape(&m, &g, 0, 0);
    size_t size = m.count;
    uint32_t original = geometry_hash(&m);
    CHECK(size > 65000 && !m.failed);
    landscape(&m, &g, 0, 0);
    CHECK(size == m.count && original == geometry_hash(&m));
    game_conditions(&g, 2, 1, 7);
    landscape(&m, &g, 0, 0);
    CHECK(geometry_hash(&m) != original);
    wildlife(&m, &g);
    CHECK(m.count >= 48);
    size = m.count;
    g.season = 3;
    g.weather = 2;
    wildlife(&m, &g);
    CHECK(m.count > size);
    cabin_mesh(&m, s, &g);
    CHECK(!m.count);
    game_action(&g, ACT_CABIN);
    landscape(&m, &g, 0, 0);
    cabin_mesh(&m, s, &g);
    size = m.count;
    CHECK(size > 1000);
    game_action(&g, ACT_USE);
    cabin_mesh(&m, s, &g);
    CHECK(m.count > size);
    game_action(&g, ACT_USE);
    g.cabin.room = 8;
    cabin_mesh(&m, s, &g);
    CHECK(m.count > size);
    Game grove;
    game_init(&grove, &s->rules, 41);
    bool found = false;
    for (int iz = 0; iz < 15 && !found; iz++)
        for (int ix = 0; ix < 15 && !found; ix++) {
            grove.x = 65 + ix * 4;
            grove.z = 18 + iz * 4;
            game_action(&grove, ACT_CABIN);
            if (grove.cabin.tree_supported) {
                found = true;
                cabin_mesh(&m, s, &grove);
                CHECK(m.count > 1000);
            } else
                game_action(&grove, ACT_CABIN);
        }
    CHECK(found);
    wildlife(&m, &g);
    CHECK(m.count > 0);
    CHECK(!strcmp(room_name(s, 0), "Hearth Room"));
    CHECK(strcmp(room_name(s, 2147483647u), "Hearth Room"));
    Pose pose = {{0}, 0};
    mesh_clear(&m);
    CHECK(script_room(s, &m, pose, 13, true, false));
    original = geometry_hash(&m);
    size = m.count;
    mesh_clear(&m);
    CHECK(script_room(s, &m, pose, 13, true, false));
    CHECK(size == m.count && original == geometry_hash(&m));
    CHECK(luaL_dostring(s->L, "function build_room() error('broken room') end; function "
                              "room_title() error('bad title') end") == LUA_OK);
    CHECK(!script_room(s, &m, pose, 1, false, false) && strstr(s->error, "broken room"));
    CHECK(!strcmp(room_name(s, 1), "Cabin"));
    mesh_clear(&m);
    box(&m, pose, (Vec){0}, (Vec){1, 2, 3}, rgb(0xe4e0dd));
    CHECK(m.count == 36);
    m.failed = true;
    tri(&m, (Vec){0}, (Vec){0}, (Vec){0}, rgb(0), rgb(0), rgb(0), 0);
    CHECK(m.count == 36);
    mesh_free(&m);
    CHECK(!m.count && !m.v);
}
static void event_key(SDL_Keycode k) {
    SDL_Event e = {0};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = k;
    CHECK(SDL_PushEvent(&e));
    CHECK(mountain_step());
}
static void platform(const char *assets) {
    CHECK(!mountain_open("/missing-mountain-assets", 120, 80));
    CHECK(mountain_open(assets, 240, 150));
    CHECK(mountain_value(17));
    CHECK(!strcmp(mountain_label(0), "Larkspur Ridge"));
    CHECK(!strcmp(mountain_label(1), "Summer"));
    CHECK(!strcmp(mountain_label(2), "Clear"));
    CHECK(!strcmp(mountain_label(3), "Hearth Room"));
    CHECK(!strcmp(mountain_option(0, 3), "Winter"));
    CHECK(!strcmp(mountain_option(1, 2), "Rain"));
    CHECK(mountain_height(0, 0) > mountain_water());
    mountain_resize(260, 160);
    mountain_frame();
    uint32_t checksum = display_sample(260, 160);
    mountain_seed(271);
    mountain_conditions(3, 2, 0);
    mountain_frame();
    CHECK(checksum != display_sample(260, 160));
    mountain_look(.2f, -.2f);
    mountain_input(.7f, .1f);
    mountain_walk(.1f, 1, 0);
    for (int k = 0; k <= 17; k++)
        CHECK(isfinite(mountain_value(k)));
    CHECK(mountain_value(7) == 271 && mountain_value(15) > 0);
    mountain_pause(1);
    CHECK(mountain_step());
    mountain_pause(0);
    mountain_xr_active(1);
    CHECK(mountain_step());
    Mat p = perspective(1, 1, .08f, 2400), v = identity();
    v.m[12] = .032f;
    mountain_xr_eye(&p, &v, 0, 0, 130, 160);
    v.m[12] = -.032f;
    mountain_xr_eye(&p, &v, 130, 0, 130, 160);
    mountain_xr_active(0);
    CHECK(mountain_sound());
    CHECK(!mountain_sound());
    SDL_Keycode keys[] = {SDLK_H, SDLK_E, SDLK_E,  SDLK_R,      SDLK_E, SDLK_H, SDLK_T, SDLK_U,
                          SDLK_Y, SDLK_N, SDLK_F1, SDLK_ESCAPE, SDLK_P, SDLK_M, SDLK_M, SDLK_J};
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++)
        event_key(keys[i]);
    SDL_Event mouse = {0};
    mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    SDL_PushEvent(&mouse);
    mouse.type = SDL_EVENT_MOUSE_MOTION;
    mouse.motion.xrel = 2;
    mouse.motion.yrel = 1;
    SDL_PushEvent(&mouse);
    CHECK(mountain_step());
    CHECK(mountain_photo());
    SDL_Event quit = {0};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
    CHECK(!mountain_step());
    /* Verify missing rendering assets don't leave resources live. */
    CHECK(display_open("/no-shaders") == NULL);
    mountain_close();
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    Script bad = {0}, s = {0};
    CHECK(!script_load(&bad, "/missing-trail"));
    CHECK(bad.error[0]);
    script_close(&bad);
    CHECK(script_load(&s, argv[1]));
    maths(&s.rules);
    hiking(&s.rules);
    composition(&s);
    script_close(&s);
    platform(argv[1]);
    printf("%d unit assertions passed.\n", assertions);
    return 0;
}
