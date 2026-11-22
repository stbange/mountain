#include "app.h"
#include <SDL3/SDL_main.h>
#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "walk.h"

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
static float projected(Mat m, Vec p, int row) {
    return m.m[row] * p.x + m.m[row + 4] * p.y + m.m[row + 8] * p.z + m.m[row + 12];
}
static void lighting(const Rules *r) {
    Vec morning = sun_direction(8), evening = sun_direction(16), noon = sun_direction(12);
    CLOSE(morning.y, evening.y);
    CLOSE(morning.x, -evening.x);
    CLOSE(noon.y, 1);
    CHECK(morning.x > 0 && evening.z < 0 && sun_direction(0).y < 0);
    for (int i = 0; i < 24; i++) {
        Vec d = sun_direction((float)i);
        CLOSE(d.x * d.x + d.y * d.y + d.z * d.z, 1);
    }
    World world = {0};
    Vec eye = {680, 55, -720}, object = {670, 51, -696};
    Mat original = shadow_matrix(&world, eye, evening, 66, 1024);
    float screen[3];
    for (int axis = 0; axis < 3; axis++)
        screen[axis] = projected(original, object, axis);
    world.ox = WORLD_FRAME;
    world.oz = -WORLD_FRAME;
    eye.x -= WORLD_FRAME;
    eye.z += WORLD_FRAME;
    object.x -= WORLD_FRAME;
    object.z += WORLD_FRAME;
    Mat moved = shadow_matrix(&world, eye, evening, 66, 1024);
    for (int axis = 0; axis < 3; axis++)
        CLOSE(projected(moved, object, axis), screen[axis]);
    // Light-space depth decreases towards the sun; the same ray stays on the same texel.
    Vec closer = {object.x + evening.x * 10, object.y + evening.y * 10, object.z + evening.z * 10};
    CLOSE(projected(moved, closer, 0), screen[0]);
    CLOSE(projected(moved, closer, 1), screen[1]);
    CHECK(projected(moved, closer, 2) < screen[2]);
    Lighting earth = earth_lighting(), desert = r->places[DUNES].lighting,
             moon = r->places[LUNAR].lighting;
    CHECK(earth.sky.b > earth.sky.r && desert.bounce.r > 3 * desert.bounce.b);
    CHECK(moon.sky.b < earth.sky.b * .1f && moon.clouds == 0 && moon.softness == 0);
}
static void position(Game *g, float x, float z) {
    Cabin frame = cabin_frame(g);
    Vec p = cabin_world(&frame, x, 0, z);
    g->x = p.x;
    g->z = p.z;
}
static void hiking(const Rules *r) {
    Game g;
    game_init(&g, r, r->seed);
    CHECK(!strcmp(trail_name(&g), "Larkspur Ridge"));
    CHECK(!game_action(&g, ACT_USE));
    CHECK(game_action(&g, ACT_HOME) && g.cabin.inside && !g.cabin.room);
    CHECK(game_action(&g, ACT_USE) && game_action(&g, ACT_USE) && game_action(&g, ACT_CABIN));
    game_init(&g, r, r->seed);
    CHECK(!game_action(&g, (Action)45) && !game_action(&g, ACT_DOOR));
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
    CHECK(!g.changing_weather);
    game_action(&g, ACT_CHANGING_WEATHER);
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
    CHECK(game_action(&g, ACT_DOOR) && !g.cabin.open);
    CHECK(game_action(&g, ACT_DOOR) && g.cabin.open);
    CHECK(game_action(&g, ACT_USE));
    CHECK(g.cabin.inside && !game_action(&g, ACT_CABIN));
    CLOSE(eye(&g).y, g.cabin.y + 1.65f);
    position(&g, 0, 3.6f);
    CHECK(game_action(&g, ACT_DOOR) && !g.cabin.open);
    g.yaw = g.cabin.angle + PI;
    for (int i = 0; i < 12; i++)
        game_tick(&g, .16f, 1, 0);
    CHECK(g.cabin.inside && game_near(&g) == NEAR_OPEN);
    CHECK(game_action(&g, ACT_USE) && g.cabin.open);
    position(&g, 3, 0);
    CHECK(game_near(&g) == NEAR_ROOM_DOOR);
    position(&g, 0, 0);
    CHECK(game_near(&g) == NEAR_NONE);
    position(&g, 2.8f, 3.6f);
    g.yaw = g.cabin.angle + PI;
    game_tick(&g, .16f, 1, 0);
    CHECK(g.cabin.inside && cabin_local(&g.cabin, g.x, g.z).z < 3.7f);
    position(&g, 0, -3.4f);
    CHECK(game_near(&g) == NEAR_ROOM_DOOR);
    g.yaw = g.cabin.angle;
    for (int i = 0; i < 30; i++)
        game_tick(&g, .12f, 1, 0);
    CHECK(!g.book.count && !g.cabin.room);
    for (int i = 0; i < 1002; i++) {
        position(&g, 0, -cabin_layout(&g)->depth + .52f);
        CHECK(game_near(&g) == NEAR_ROOM_DOOR && game_action(&g, ACT_USE));
        CHECK(g.book.count == (uint32_t)i + 1 && g.cabin.room == (uint32_t)i);
        CHECK(game_near(&g) == NEAR_NEXT && game_action(&g, ACT_USE));
    }
    CHECK(g.cabin.room == 1002);
    CHECK(game_near(&g) == NEAR_BACK && game_action(&g, ACT_DOOR));
    CHECK(game_near(&g) == NEAR_ROOM_DOOR && game_action(&g, ACT_DOOR));
    CHECK(game_action(&g, ACT_USE) && g.cabin.room == 1001);
    position(&g, 0, cabin_layout(&g)->depth - .4f);
    g.yaw = g.cabin.angle + PI;
    for (int i = 0; i < 40 && g.cabin.room == 1001; i++)
        game_tick(&g, .05f, 1, 0);
    CHECK(g.cabin.room == 1000);
    position(&g, 0, -cabin_layout(&g)->depth + .4f);
    g.yaw = g.cabin.angle;
    for (int i = 0; i < 40 && g.cabin.room == 1000; i++)
        game_tick(&g, .05f, 1, 0);
    CHECK(g.cabin.room == 1001 && g.book.count == 1002);
    CHECK(game_action(&g, ACT_HOME));
    position(&g, 4, 0);
    CHECK(game_action(&g, ACT_USE) && game_action(&g, ACT_USE));
    CHECK(g.cabin.room == 1003 && cabin_room(&g.book, g.cabin.room)->turn == 1);
    CHECK(cabin_layout(&g)->balcony && cabin_layout(&g)->width > 10);
    const Layout *l = cabin_layout(&g);
    position(&g, -l->width + .4f, 0);
    CHECK(game_near(&g) == NEAR_BALCONY_DOOR && game_action(&g, ACT_USE));
    CHECK(game_near(&g) == NEAR_BALCONY && game_action(&g, ACT_USE));
    Cabin frame = cabin_frame(&g);
    CHECK(cabin_local(&frame, g.x, g.z).x < -l->width && g.cabin.inside);
    CHECK(game_action(&g, ACT_DOOR));
    g.yaw = frame.angle;
    for (int i = 0; i < 30; i++)
        game_tick(&g, .15f, 0, -1);
    CLOSE(cabin_local(&frame, g.x, g.z).x, -l->width - 2.95f);
    for (int i = 0; i < 30; i++)
        game_tick(&g, .15f, 0, 1);
    CHECK(cabin_local(&frame, g.x, g.z).x < -l->width);
    CHECK(game_action(&g, ACT_DOOR));
    game_tick(&g, .2f, 0, 1);
    CHECK(cabin_local(&frame, g.x, g.z).x > -l->width && !g.falls);
    for (int wall = 0; wall < 4; wall++) {
        for (int side = 0; side < 2; side++) {
            float along = (side ? .60f : -.60f);
            if (wall <= REAR)
                position(&g, along * l->width, (wall == FRONT ? 1 : -1) * (l->depth - .4f));
            else
                position(&g, (wall == RIGHT_WALL ? 1 : -1) * (l->width - .4f), along * l->depth);
            int win = wall * 2 + side;
            CHECK(cabin_window(&g) == win && game_near(&g) == NEAR_PANE_CLOSED);
            CHECK(!game_action(&g, ACT_DOOR) && game_action(&g, ACT_BLINDS));
            CHECK(game_action(&g, ACT_USE) && game_near(&g) == NEAR_PANE_OPEN);
            CHECK(cabin_room(&g.book, 1003)->panes == ((2u << win) - 1));
        }
    }
    position(&g, 0, l->depth - .5f);
    CHECK(game_action(&g, ACT_USE) && !g.cabin.room);
    CHECK(game_near(&g) == NEAR_NEXT && game_action(&g, ACT_DOOR));
    CHECK(game_action(&g, ACT_USE) && game_action(&g, ACT_USE) && g.cabin.room == 1003);
    CHECK(cabin_room(&g.book, 1003)->panes == 255 && cabin_room(&g.book, 1003)->blinds == 255);
    CHECK(game_action(&g, ACT_LOOKOUT_WEATHER) && game_action(&g, ACT_LOOKOUT_PREDATORS) &&
          game_action(&g, ACT_LOOKOUT_GIANTS));
    char *saved = cabin_encode(&g.book);
    RoomBook copy = {0};
    CHECK(saved && cabin_decode(&copy, saved, r->layouts));
    CHECK(copy.count == 1003 && copy.weather == 1 && copy.predators && copy.giants);
    char *again = cabin_encode(&copy);
    CHECK(!strcmp(saved, again));
    free(again);
    free(saved);
    const char *invalid[] = {NULL,
                             "bad",
                             "CAB10",
                             "CAB1q000000000000000",
                             "CAB10000000000020000",
                             "CAB10000000001000000",
                             "CAB1000000000000000000000000210000000000002100000"};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++)
        CHECK(!cabin_decode(&copy, invalid[i], r->layouts) && copy.count == 1003);
    cabin_forget(&copy);
    CHECK(game_action(&g, ACT_HOME) && game_near(&g) == NEAR_EXIT);
    for (int i = 0; i < 100 && g.cabin.inside; i++)
        game_tick(&g, 1.f / 120, 1, 0);
    CHECK(!g.cabin.inside && game_action(&g, ACT_CABIN) && !g.cabin.present);
    CHECK(game_action(&g, ACT_HOME) && g.cabin.inside && g.book.count == 1003);
    cabin_forget(&g.book);
}
static void marked_hikes(const Rules *rules, const char *assets) {
    Game g;
    const uint32_t seeds[] = {4812, 291, 3107, 0, UINT32_MAX, rules->survey.seed};
    for (size_t j = 0; j < sizeof seeds / sizeof *seeds; j++) {
        game_init(&g, rules, seeds[j]);
        Route *r = &g.world.route;
        CHECK(r->count > 2 && r->overlook > 0 && r->overlook < r->count - 1);
        CHECK(r->length > 150 && r->length < 1000);
        CHECK(route_distance(r, 900, 900) == 32);
        float walked = 0;
        for (int i = 1; i < r->count; i++) {
            Place a = r->point[i - 1], b = r->point[i];
            float len = hypotf(a.x - b.x, a.z - b.z);
            CHECK(len > 0);
            for (int k = 0; k <= (int)len + 1; k++) {
                float t = (float)k / ((int)len + 1), x = mixf(a.x, b.x, t), z = mixf(a.z, b.z, t);
                CHECK(height_at(&g.world, x, z) > water_at(&g.world, x, z));
                CHECK(route_distance(r, x, z) < .002f);
            }
            walked += len;
        }
        CLOSE(walked, r->length);
        if (g.world.survey) {
            const Survey *v = g.world.survey;
            CHECK(!strcmp(trail_name(&g), "Taggart Lake") && v->side == 513);
            CHECK(!survey_lake(v, 0, 0) && survey_lake(v, -110, -20));
            CHECK(survey_height(v, -110, -20) < 0);
            CHECK(survey_height(v, -865, -840) * v->scale + v->surface > 3900);
            CHECK(isfinite(survey_height(v, 99999, -99999)));
            CHECK(height_at(&g.world, r->point[r->count - 1].x, r->point[r->count - 1].z) >
                  height_at(&g.world, r->point[r->overlook].x, r->point[r->overlook].z));
        }
    }
    Survey missing = {0};
    CHECK(!survey_load(&missing, "/no-mountain"));
    strcpy(missing.file, "scripts/world.lua");
    missing.scale = 5;
    CHECK(!survey_load(&missing, assets) && !missing.data);
    game_init(&g, rules, 4812);
    Route original = g.world.route;
    game_init(&g, rules, 4812);
    CHECK(!memcmp(&original, &g.world.route, sizeof original));
    select_season(&g, 2);
    select_weather(&g, 1);
    CHECK(!g.cycle_seasons && !g.changing_weather);
    g.season_clock = rules->season_interval;
    g.weather_clock = rules->weather_interval;
    game_tick(&g, .2f, 0, 0);
    CHECK(g.season == 2 && g.weather == 1);
    game_action(&g, ACT_CYCLE_SEASONS);
    g.season_clock = rules->season_interval;
    game_tick(&g, .2f, 0, 0);
    CHECK(g.season == 3 && g.weather == 1);
    game_action(&g, ACT_CHANGING_WEATHER);
    select_season(&g, 1);
    CHECK(g.changing_weather && !g.cycle_seasons);
    game_conditions(&g, 1, 0, 12);
    CHECK(insects_active(&g));
    game_conditions(&g, 1, 2, 12);
    CHECK(!insects_active(&g));
    game_conditions(&g, 0, 0, 0);
    CHECK(!insects_active(&g));
    game_conditions(&g, 3, 0, 12);
    CHECK(!insects_active(&g));
}
static uint32_t geometry_hash(const Mesh *m) {
    const unsigned char *bytes = (const unsigned char *)m->v;
    uint32_t hash = 0x811c9dc5u;
    for (size_t i = 0; i < m->count * sizeof(Vertex); i++)
        hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}
/* Position a test observer exactly the way a long hike partitions global metres
   into a signed origin and small local coordinates. */
static void observe(Game *g, double x, double z) {
    g->world.ox = (int64_t)floor(x / WORLD_FRAME + .5) * WORLD_FRAME;
    g->world.oz = (int64_t)floor(z / WORLD_FRAME + .5) * WORLD_FRAME;
    g->x = (float)(x - g->world.ox);
    g->z = (float)(z - g->world.oz);
    trail_window(&g->world, g->x, g->z);
}
static void endless_terrain(const Rules *rules) {
    const uint32_t seeds[] = {4812, rules->survey.seed, 0, UINT32_MAX};
    unsigned char *map = malloc(TRAIL_TEXTURE_SIZE * TRAIL_TEXTURE_SIZE);
    CHECK(map);
    CHECK(cell_hash(12, -5, 19) != cell_hash((INT64_C(1) << 32) + 12, -5, 19));
    for (size_t n = 0; n < sizeof seeds / sizeof *seeds; n++) {
        Game g;
        game_init(&g, rules, seeds[n]);
        World fresh = g.world;
        Place end = g.world.route.point[g.world.route.count - 1];
        Place join = trail_knot(&g.world, 0);
        CLOSE(end.x, join.x);
        CLOSE(end.z, join.z);
        float low = 1e6f, high = -1e6f;
        for (int k = -5; k <= 5; k++) {
            double x = 320000004.0 + k * 215., z = -718000061.0 - k * 97.;
            float h = raw_height(&g.world, x, z);
            low = fminf(low, h);
            high = fmaxf(high, h);
            observe(&g, x, z);
            CLOSE(h, height_at(&g.world, g.x, g.z));
            CLOSE(h, raw_height(&fresh, x, z));
        }
        CHECK(high - low > 20);
        const int64_t stations[] = {0,   1,   9,     64,       127, 128, 190,
                                    256, 327, 10003, 10000007, 128, 1,   0};
        for (size_t j = 0; j < sizeof stations / sizeof *stations; j++) {
            int64_t i = stations[j];
            double ax, az, bx, bz;
            trail_coordinates(&fresh, i, &ax, &az);
            trail_coordinates(&fresh, i + 1, &bx, &bz);
            observe(&g, (ax + bx) * .5, (az + bz) * .5);
            double x, z;
            trail_coordinates(&g.world, i, &x, &z);
            CLOSE(ax, x);
            CLOSE(az, z);
            Place a = trail_knot(&g.world, i), b = trail_knot(&g.world, i + 1);
            float ha = height_at(&g.world, a.x, a.z), hb = height_at(&g.world, b.x, b.z);
            CHECK(ha > water_at(&g.world, a.x, a.z));
            CHECK(hb > water_at(&g.world, b.x, b.z));
            CHECK(fabsf(hb - ha) < .32 * hypot(bx - ax, bz - az) + .01);
            float middle = height_at(&g.world, g.x, g.z);
            CHECK(middle > water_at(&g.world, g.x, g.z));
            CLOSE(middle, height_global(&fresh, (ax + bx) * .5, (az + bz) * .5));
            CHECK(trail_distance(&g.world, g.x, g.z) < .001f);
            if (i == 0 || j % 3 == 0) {
                route_texture(&g.world, g.x, g.z, map);
                int center = (TRAIL_TEXTURE_SIZE / 2) * (TRAIL_TEXTURE_SIZE + 1);
                CHECK(map[center] < 45);
                bool off_path = false;
                for (int side = -48; side <= 48; side += 12) {
                    int dx = g.world.trail.dx ? 0 : side, dz = g.world.trail.dx ? side : 0;
                    float distance = trail_distance(&g.world, g.x + dx + .5f, g.z + dz + .5f);
                    int expected = (int)fminf(255, distance * 42.5f);
                    CHECK(abs(map[center + dz * TRAIL_TEXTURE_SIZE + dx] - expected) <= 1);
                    off_path |= expected == 255;
                }
                CHECK(off_path);
            }
        }
        observe(&g, 0, 0);
        double loop_x, loop_z;
        trail_coordinates(&fresh, 783, &loop_x, &loop_z);
        float seam =
            (float)((fresh.survey ? fresh.survey->cell * (fresh.survey->side / 2) : 960) - .02f);
        CHECK(fabsf(raw_height(&fresh, seam, -105) - raw_height(&fresh, seam + .04f, -105)) < .4f);
        CHECK(fabsf(raw_height(&fresh, -seam, 105) - raw_height(&fresh, -seam - .04f, 105)) < .4f);
        observe(&g, loop_x, loop_z);
        float before = ground(&g, g.x, g.z);
        /* An arbitrary query outside the active trail window gives the same elevation. */
        CHECK(fabsf(before - height_at(&fresh, (float)loop_x, (float)loop_z)) < .002f);
        observe(&g, -5000000, 4500000);
        observe(&g, loop_x, loop_z);
        CLOSE(before, ground(&g, g.x, g.z));
        CLOSE(trail_station(&g.world, g.x, g.z) / TRAIL_STEP, 783);
    }
    free(map);
    Game g;
    game_init(&g, rules, 4812);
    CHECK(game_action(&g, ACT_CABIN));
    double site_x = g.cabin.site_x, site_z = g.cabin.site_z;
    g.x = WORLD_FRAME + .375f;
    g.z = -2 * WORLD_FRAME - .25f;
    world_track(&g);
    CLOSE(g.x, .375);
    CLOSE(g.z, -.25);
    CLOSE(g.cabin.x + g.world.ox, site_x);
    CLOSE(g.cabin.z + g.world.oz, site_z);
    CHECK(!cabin_in_view(&g));
    g.x -= WORLD_FRAME;
    g.z += 2 * WORLD_FRAME;
    world_track(&g);
    CHECK(g.world.ox == 0 && g.world.oz == 0 && cabin_in_view(&g));
    CLOSE(g.cabin.x, site_x);
    CLOSE(g.cabin.z, site_z);
}
static void expeditions(Script *script) {
    const Rules *r = &script->rules;
    CHECK(r->place_count == 12);
    uint32_t collection = 0;
    Mesh mesh = {0};
    for (int i = 0; i < r->place_count; i++) {
        Game g;
        const Destination *d = &r->places[i];
        game_init(&g, r, d->seed);
        World *w = &g.world;
        CHECK(w->place == d && w->profile == (Profile)i && d->art && *d->stops[1]);
        CHECK(destination_for_seed(r, d->seed) == i);
        for (int j = 0; j < PROP_KINDS * PROP_FORMS; j++)
            CHECK(d->art[j].count > 0);
        g.notes = collection;
        field_refresh(&g);
        CHECK(field_stage(&g) == 0 && !strcmp(field_hint(&g), d->notes[0]));
        Place first = field_site(w, 0), second = field_site(w, 1), last = field_site(w, 2);
        CHECK(hypot(first.x - second.x, first.z - second.z) > 10 &&
              hypot(last.x - second.x, last.z - second.z) > 10);
        observe(&g, last.x, last.z);
        CHECK(!field_read(&g));
        observe(&g, first.x, first.z);
        g.cabin.inside = true;
        CHECK(field_near(&g) == NEAR_NONE);
        g.cabin.inside = false;
        CHECK(game_near(&g) == NEAR_READ && game_action(&g, ACT_USE) && field_stage(&g) == 1);
        CHECK(strstr(field_hint(&g), "Head "));
        observe(&g, second.x, second.z);
        mesh_clear(&mesh);
        field_scenery(&mesh, &g);
        CHECK(mesh.count > 0);
        float yaw = atan2f(last.x - second.x, second.z - last.z);
        if (d->skill == ALIGN) {
            g.yaw = yaw + PI;
            CHECK(!field_read(&g) && strstr(field_hint(&g), "Sight"));
            g.yaw = yaw;
        } else if (d->skill == DUSK) {
            g.hour = 11;
            CHECK(!field_read(&g) && strstr(field_hint(&g), "evening"));
            g.hour = 17.25f;
        } else {
            observe(&g, second.x - 5.5 * sinf(yaw), second.z + 5.5 * cosf(yaw));
            CHECK(!field_read(&g) && strstr(field_hint(&g), "beside"));
            Place tall = field_pipe(w, 2);
            observe(&g, tall.x, tall.z);
            CHECK(!field_read(&g) && g.chimes == 0);
            for (int j = 0; j < 3; j++) {
                Place p = field_pipe(w, j);
                observe(&g, p.x, p.z);
                if (j < 2)
                    CHECK(!field_read(&g) && g.chimes == j + 1);
            }
        }
        CHECK(game_action(&g, ACT_USE) && field_stage(&g) == 2);
        field_refresh(&g);
        CHECK(strstr(field_hint(&g), d->signs[2]));
        observe(&g, last.x, last.z);
        mesh_clear(&mesh);
        field_scenery(&mesh, &g);
        CHECK(mesh.count >= d->art[RELIC * PROP_FORMS].count / 3);
        CHECK(game_near(&g) == NEAR_TREASURE && game_action(&g, ACT_USE));
        CHECK(field_stage(&g) == 3 && field_near(&g) == NEAR_NONE && !field_read(&g));
        field_refresh(&g);
        CHECK(strstr(field_hint(&g), "Discovered"));
        collection = g.notes;
        if (i >= COAST) {
            uint32_t other_seed = d->seed + 500;
            World other;
            world_destination(&other, other_seed, r, i);
            CHECK(other.profile == w->profile && other.route.count == w->route.count);
            bool different = false;
            for (int k = -4; k <= 4; k++) {
                double x = 93456000. + 123 * k, z = -64227009. - 237 * k;
                float h = realm_height(w, x, z);
                CHECK(isfinite(h));
                different |= fabsf(h - realm_height(&other, x, z)) > .3f;
            }
            CHECK(different);
            for (int k = 0; k < w->route.count; k++) {
                Place p = w->route.point[k];
                CHECK(height_global(w, p.x, p.z) > water_global(w, p.x, p.z));
            }
            double tx, tz;
            trail_coordinates(w, 80002, &tx, &tz);
            observe(&g, tx, tz);
            CHECK(trail_distance(w, g.x, g.z) < .002 &&
                  ground(&g, g.x, g.z) > water_at(w, g.x, g.z));
            observe(&g, 0, 0);
            if (i == DUNES) {
                CHECK(realm_height(w, -46, -110) < water_global(w, -46, -110));
                int ci = 4, cj = -3;
                double x = (ci + .25 + .5 * random2(ci, cj, w->seed + 194)) * 640,
                       z = (cj + .25 + .5 * random2(ci, cj, w->seed + 206)) * 640;
                CHECK(oasis_distance(w, x, z) < .001 &&
                      height_global(w, x, z) < water_global(w, x, z));
                CHECK(realm_height(w, x + 110, z) > 20);
            }
            if (i == LUNAR)
                CHECK(water_global(w, 0, 0) < -1000 && !strcmp(weather_label(&g, 2), "Airless"));
            if (i == COAST || i == BASALT)
                CHECK(realm_height(w, -75, 0) < 0 && realm_height(w, 45, 0) > 5);
            Tree tree;
            Animal animals[LOCAL_ANIMALS];
            CHECK(!world_tree(w, 0, 0, &tree) && !fauna_near(&g, animals, LOCAL_ANIMALS));
            mesh_clear(&mesh);
            realm_scenery(&mesh, &g, 0, 0);
            CHECK(mesh.count > 0);
            g.hour = 13;
            mesh_clear(&mesh);
            realm_life(&mesh, &g);
            if (i == LUNAR)
                CHECK(!mesh.count);
            else if (i != DUNES)
                CHECK(mesh.count > 0);
        }
    }
    CHECK(collection == 0xffffff);
    mesh_free(&mesh);
}
static void high_passes(const Rules *rules) {
    World w;
    world_init(&w, 4812, rules);
    for (int n = 0; n < 3; n++) {
        int64_t valley = (int64_t)((1050 + n * 7800) / TRAIL_STEP);
        int64_t pass = valley + 3900 / TRAIL_STEP;
        CHECK(alpine_pass_height(&w, (double)pass * TRAIL_STEP) > 260);
        double x, z;
        trail_coordinates(&w, pass, &x, &z);
        float summit = height_global(&w, x, z);
        CHECK(summit > 235);
        trail_coordinates(&w, valley, &x, &z);
        CHECK(summit - height_global(&w, x, z) > 100);
        /* Sample the profile across both shoulders and its rolling-window seams. */
        float prev = 0;
        for (int64_t i = valley; i <= valley + 320; i++) {
            trail_coordinates(&w, i, &x, &z);
            w.ox = (int64_t)floor(x / WORLD_FRAME + .5) * WORLD_FRAME;
            w.oz = (int64_t)floor(z / WORLD_FRAME + .5) * WORLD_FRAME;
            trail_window(&w, (float)(x - w.ox), (float)(z - w.oz));
            float h = height_global(&w, x, z);
            CHECK(i == valley || fabsf(prev - h) < .32f * TRAIL_STEP + .004f);
            prev = h;
        }
    }
}
static void alpine_footing(const Rules *rules) {
    Game g;
    const uint32_t seeds[] = {4812, 7, 42};
    for (int q = 0; q < 3; q++) {
        game_init(&g, rules, seeds[q]);
        World *w = &g.world;
        const Ascent *a = &w->ascent;
        CHECK(a->count == 23 && a->pass > 1 && a->pass < a->count - 2);
        Place pass = field_site(w, 1), top = field_site(w, 2);
        float summit = height_global(w, top.x, top.z);
        CHECK(summit - a->base > 445 && height_global(w, pass.x, pass.z) > a->base + 160);
        for (int k = 0; k < 8; k++) {
            float angle = k * PI / 4;
            float slope = height_global(w, pass.x + 12 * cosf(angle), pass.z + 12 * sinf(angle)) -
                          height_global(w, pass.x, pass.z);
            CHECK(fabsf(slope) < 17);
        }
        CHECK(a->level[a->pass] < a->level[a->pass - 2] &&
              a->level[a->pass] < a->level[a->pass + 2]);
        for (int i = 1; i < a->count; i++) {
            Place b = a->point[i], p = a->point[i - 1];
            float len = hypotf(b.x - p.x, b.z - p.z);
            float prev = height_global(w, p.x, p.z);
            for (int k = 1; k <= 20; k++) {
                float x = mixf(p.x, b.x, k / 20.f), z = mixf(p.z, b.z, k / 20.f),
                      h = height_global(w, x, z);
                CHECK(ascent_distance(w, x, z) < .002 && fabsf(h - prev) < len / 20 * .61f);
                prev = h;
            }
        }
        observe(&g, top.x, top.z);
        g.safe_x = top.x;
        g.safe_z = top.z;
        g.notes = 2;
        g.yaw = atan2f(a->along.x, -a->along.z);
        bool exposed = false;
        for (int i = 0; i < 105 && !g.fall_time; i++) {
            game_tick(&g, .045f, 1, 0);
            exposed |= g.exposed && !g.fall_time;
        }
        CHECK(exposed && g.fall_time > 0 && !game_action(&g, ACT_CABIN));
        float before = eye(&g).y;
        game_tick(&g, .15f, 0, 0);
        CHECK(eye(&g).y < before);
        for (int i = 0; i < 8; i++)
            game_tick(&g, .17f, 0, 0);
        CHECK(!g.fall_time && g.falls == 1 && g.warning_timer > 0 && g.notes == 2);
        CLOSE(g.world.ox + (double)g.x, g.safe_x);
        CLOSE(g.world.oz + (double)g.z, g.safe_z);
        CHECK(game_action(&g, ACT_CABIN));
        g.safe_x += 250000000.375;
        g.safe_z -= 807000000.125;
        g.fall_time = .01f;
        CHECK(footing_recover(&g, .1f));
        CLOSE(g.world.ox + (double)g.x, g.safe_x);
        CLOSE(g.world.oz + (double)g.z, g.safe_z);
        CHECK(g.cabin.present && g.falls == 2);
    }
}

static void living_world(Script *script) {
    Game g;
    game_init(&g, &script->rules, 4812);
    Tree tree = {0};
    bool found = false;
    for (int j = 2; j < 30 && !found; j++)
        for (int i = 2; i < 30 && !found; i++) {
            if (!world_tree(&g.world, i, j, &tree))
                continue;
            found = true;
            Tree rebuilt;
            g.world.ox = WORLD_FRAME;
            g.world.oz = -WORLD_FRAME;
            CHECK(world_tree(&g.world, i - WORLD_FRAME / 7, j + WORLD_FRAME / 7, &rebuilt));
            CLOSE(tree.x, rebuilt.x + WORLD_FRAME);
            CLOSE(tree.z, rebuilt.z - WORLD_FRAME);
            CHECK(fabsf(tree.y - rebuilt.y) < .005f && tree.form == rebuilt.form);
            CLOSE(tree.height, rebuilt.height);
            CLOSE(tree.angle, rebuilt.angle);
        }
    CHECK(found);
    observe(&g, 0, 0);
    Forest *f = &script->forest;
    forest_prepare(f, &g);
    CHECK(f->atlas && f->seed == g.world.seed && f->season == g.season);
    for (int i = 0; i < TREE_FORMS; i++) {
        CHECK(f->close[i].count > f->middle[i].count && f->middle[i].count > 36);
        if (i % 8)
            CHECK(geometry_hash(&f->close[i]) != geometry_hash(&f->close[i - 1]));
    }
    size_t atlas_size = FOREST_ATLAS * FOREST_ATLAS * 4;
    unsigned char *summer = malloc(atlas_size);
    CHECK(summer);
    memcpy(summer, f->atlas, atlas_size);
    Mesh model = {0};
    voxel_tree(&model, f, tree, 12);
    size_t near_count = model.count;
    mesh_clear(&model);
    voxel_tree(&model, f, tree, 44);
    CHECK(model.count < near_count && model.count > 12);
    mesh_clear(&model);
    voxel_tree(&model, f, tree, 95);
    CHECK(model.count == 12 && model.v[0].material > 1.4f);
    mesh_free(&model);
    g.season = 3;
    forest_prepare(f, &g);
    CHECK(memcmp(summer, f->atlas, atlas_size));
    g.season = 1;
    g.world.seed++;
    forest_prepare(f, &g);
    CHECK(memcmp(summer, f->atlas, atlas_size));
    g.world.seed--;
    forest_prepare(f, &g);
    CHECK(!memcmp(summer, f->atlas, atlas_size));
    free(summer);
    int all = 0, far = 0;
    for (int j = -3; j <= 3; j++)
        for (int i = -3; i <= 3; i++) {
            /* Sample both the wooded valleys and the snowy uplands. */
            observe(&g, 1000000 + i * 1430., -2500000 + j * 1370.);
            game_conditions(&g, 1, 0, 9);
            Animal a[LOCAL_ANIMALS], again[LOCAL_ANIMALS];
            int n = fauna_near(&g, a, LOCAL_ANIMALS);
            CHECK(n == fauna_near(&g, again, LOCAL_ANIMALS) && n <= LOCAL_ANIMALS);
            for (int k = 0; k < n; k++) {
                all |= 1 << a[k].kind;
                far++;
                CHECK(a[k].id == again[k].id && a[k].kind == again[k].kind);
                CLOSE(a[k].pos.x, again[k].pos.x);
                CLOSE(a[k].pos.z, again[k].pos.z);
                CHECK(a[k].pos.y > water_at(&g.world, a[k].pos.x, a[k].pos.z));
            }
            game_conditions(&g, 3, 2, 0);
            n = fauna_near(&g, a, LOCAL_ANIMALS);
            for (int k = 0; k < n; k++)
                CHECK(a[k].kind != MARMOT && a[k].kind != EAGLE);
        }
    CHECK(all == (1 << FAUNA_TYPES) - 1 && far > 100);
    observe(&g, 0, 0);
    game_conditions(&g, 1, 0, 9);
    Animal a[LOCAL_ANIMALS], b[LOCAL_ANIMALS];
    int n = fauna_near(&g, a, LOCAL_ANIMALS);
    CHECK(n > 0 && fauna_near(&g, b, 1) == 1 && fauna_near(&g, b, 0) == 0);
    g.world.ox = WORLD_FRAME;
    g.x -= WORLD_FRAME;
    int m = fauna_near(&g, b, LOCAL_ANIMALS);
    CHECK(m == n);
    for (int k = 0; k < n; k++) {
        CHECK(a[k].id == b[k].id);
        CLOSE(a[k].pos.x, b[k].pos.x + WORLD_FRAME);
        CLOSE(a[k].pos.z, b[k].pos.z);
    }
    observe(&g, 0, 0);
    int first = 0;
    while (first < n && a[first].kind == EAGLE)
        first++;
    CHECK(first < n);
    g.cabin = (Cabin){.present = true, .x = a[first].pos.x, .z = a[first].pos.z};
    m = fauna_near(&g, b, LOCAL_ANIMALS);
    for (int k = 0; k < m; k++)
        CHECK(b[k].id != a[first].id);
    Mesh animals = {0};
    for (int k = 0; k < FAUNA_TYPES; k++) {
        Animal one = {.kind = (AnimalKind)k, .id = 17, .size = 1, .phase = 1.4f, .activity = .8f};
        CHECK(script_animal(script, &animals, &one, g.season));
        CHECK(animals.count > 500);
        mesh_clear(&animals);
    }
    CHECK(luaL_dostring(script->L,
                        "saved_animal=build_animal; function build_animal(k,s,p,a,id,part) "
                        "part(0,0,0,0,0,0,1,1,0); error('bad animal') end") == LUA_OK);
    CHECK(!script_animal(script, &animals, a, g.season) && strstr(script->error, "bad animal"));
    CHECK(luaL_dostring(script->L, "build_animal=saved_animal") == LUA_OK);
    char fauna_file[sizeof script->rules.fauna_script];
    strcpy(fauna_file, script->rules.fauna_script);
    script->rules.fauna_script[0] = 0;
    wildlife(&animals, script, &g);
    CHECK(animals.count > 0);
    strcpy(script->rules.fauna_script, fauna_file);
    mesh_free(&animals);
}
static void destination_shelters(Script *s) {
    uint32_t exterior[DEST_LIMIT] = {0};
    Mesh mesh = {0};
    for (int i = 0; i < s->rules.place_count; i++) {
        Game g;
        game_destination(&g, &s->rules, s->rules.places[i].seed, i);
        const Shelter *skin = destination_shelter(&g);
        CHECK(skin->name[0] && skin->sight[0].name[0] && skin->sight[1].name[0]);
        CHECK(script_room(s, &mesh, (Pose){{0}, 0}, &g.book.hearth, 0, -1, skin->key));
        exterior[i] = geometry_hash(&mesh);
        for (int j = 0; j < i; j++)
            CHECK(exterior[i] != exterior[j]);
        mesh_clear(&mesh);
        CHECK(game_action(&g, ACT_CABIN) && game_near(&g) == NEAR_OPEN);
        CHECK(game_action(&g, ACT_LOOKOUT_PREDATORS) && game_action(&g, ACT_LOOKOUT_GIANTS));
        CHECK(game_action(&g, ACT_LOOKOUT_WEATHER));
        lookout_scenery(&mesh, s, &g);
        CHECK(!mesh.count && lookout_event(&g)->kind == FX_CALM);
        CHECK(game_action(&g, ACT_USE) && game_action(&g, ACT_USE));
        CHECK(g.cabin.lookout[0].valid && g.cabin.lookout[1].valid);
        for (int slot = 0; slot < 2; slot++) {
            struct LookoutSite site = g.cabin.lookout[slot];
            float x = g.cabin.x + site.dx, z = g.cabin.z + site.dz;
            if (skin->sight[slot].habitat == ON_WATER)
                CHECK(height_at(&g.world, x, z) < water_at(&g.world, x, z));
            if (skin->sight[slot].habitat == ON_LAND)
                CHECK(height_at(&g.world, x, z) > water_at(&g.world, x, z));
        }
        g.book.weather = 0;
        lookout_scenery(&mesh, s, &g);
        CHECK(mesh.count > 100 && !mesh.failed);
        uint32_t sights = geometry_hash(&mesh);
        mesh_clear(&mesh);
        position(&g, -3, 1);
        g.yaw += 1.2f;
        lookout_scenery(&mesh, s, &g);
        CHECK(sights == geometry_hash(&mesh) && !g.falls);
        mesh_clear(&mesh);
        g.book.giants = g.book.predators = false;
        for (int mode = 0; mode < 3; mode++) {
            CHECK(game_action(&g, ACT_LOOKOUT_WEATHER) && lookout_event(&g) == &skin->event[mode]);
            const LookoutEvent *event = lookout_event(&g);
            CHECK(event_thunders(&g) ==
                  (event->kind == FX_LIGHTNING || event->kind == FX_DRYLIGHT));
            if (g.world.profile == LUNAR)
                CHECK(event->extinction == 0 && !event_thunders(&g));
            lookout_scenery(&mesh, s, &g);
            if (event->kind != FX_MIST && event->kind != FX_DRYLIGHT && event->kind != FX_HEAT)
                CHECK(mesh.count > 0);
            if (event_thunders(&g)) {
                size_t between_flashes = mesh.count;
                mesh_clear(&mesh);
                double before_flash = g.elapsed;
                g.elapsed = 2.8;
                lookout_scenery(&mesh, s, &g);
                CHECK(mesh.count > between_flashes);
                g.elapsed = before_flash;
            }
            mesh_clear(&mesh);
        }
        g.world.place = NULL;
        CHECK(destination_shelter(&g) == &s->rules.default_shelter);
        cabin_forget(&g.book);
    }
    mesh_free(&mesh);
}
static void composition(Script *s) {
    Game g;
    Mesh m = {0};
    game_init(&g, &s->rules, 41);
    landscape(&m, &s->forest, &g, 0, 0);
    size_t size = m.count;
    uint32_t original = geometry_hash(&m);
    CHECK(size > 65000 && !m.failed);
    landscape(&m, &s->forest, &g, 0, 0);
    CHECK(size == m.count && original == geometry_hash(&m));
    game_conditions(&g, 2, 1, 7);
    landscape(&m, &s->forest, &g, 0, 0);
    CHECK(geometry_hash(&m) != original);
    wildlife(&m, s, &g);
    CHECK(m.count >= 48);
    game_conditions(&g, 3, 0, 0);
    wildlife(&m, s, &g);
    size = m.count;
    g.weather = 2;
    wildlife(&m, s, &g);
    CHECK(m.count == size + 110 * 3);
    cabin_mesh(&m, s, &g);
    CHECK(!m.count);
    game_action(&g, ACT_CABIN);
    landscape(&m, &s->forest, &g, 0, 0);
    cabin_mesh(&m, s, &g);
    CHECK(m.light_count == 2);
    Vec bedside = cabin_world(&g.cabin, -1.06f, 1.36f, -2.72f);
    CLOSE(m.lights[0].position.x, bedside.x);
    CLOSE(m.lights[0].position.z, bedside.z);
    CHECK(m.lights[1].radius > 0 && m.lights[1].colour.r > m.lights[1].colour.b);
    size = m.count;
    CHECK(size > 1000);
    game_action(&g, ACT_USE);
    cabin_mesh(&m, s, &g);
    CHECK(m.count != size && g.book.count == 0);
    game_action(&g, ACT_USE);
    position(&g, 0, -3.5f);
    CHECK(game_action(&g, ACT_USE) && g.book.count == 1);
    cabin_mesh(&m, s, &g);
    CHECK(m.count > size);
    bool portal = false;
    for (size_t i = 0; i < m.count; i++)
        if (m.v[i].material >= 16)
            portal = true;
    CHECK(portal);
    CHECK(game_action(&g, ACT_USE) && cabin_layout(&g)->width == 10);
    position(&g, 0, 0);
    CHECK(eye(&g).y < g.cabin.y + 1);
    g.weather = 0;
    Mesh visitors = {0};
    wildlife(&visitors, s, &g);
    size_t ordinary = visitors.count;
    game_action(&g, ACT_LOOKOUT_PREDATORS);
    game_action(&g, ACT_LOOKOUT_GIANTS);
    wildlife(&visitors, s, &g);
    CHECK(visitors.count > ordinary + 500);
    size_t patrol_n = visitors.count - ordinary;
    Vertex *patrol = malloc(patrol_n * sizeof *patrol);
    CHECK(patrol);
    memcpy(patrol, visitors.v + ordinary, patrol_n * sizeof *patrol);
    float closest = 1000;
    for (size_t i = 0; i < patrol_n; i++)
        closest = fminf(closest, hypotf(patrol[i].x - g.cabin.x, patrol[i].z - g.cabin.z));
    CHECK(closest > hypotf(cabin_layout(&g)->width + 3.2f, cabin_layout(&g)->depth));
    position(&g, -cabin_layout(&g)->width - 2, 0);
    wildlife(&visitors, s, &g);
    CHECK(visitors.count >= patrol_n &&
          !memcmp(patrol, visitors.v + visitors.count - patrol_n, patrol_n * sizeof *patrol));
    free(patrol);
    position(&g, 0, 0);
    for (int i = 0; i < 4; i++) {
        game_action(&g, ACT_LOOKOUT_WEATHER);
        wildlife(&visitors, s, &g);
        CHECK(!visitors.failed);
    }
    mesh_free(&visitors);
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
    float support_x = grove.cabin.anchor.x, support_z = grove.cabin.anchor.z;
    grove.x += WORLD_FRAME * 2;
    grove.z -= WORLD_FRAME;
    world_track(&grove);
    cabin_mesh(&m, s, &grove);
    CHECK(!m.count);
    grove.x -= WORLD_FRAME * 2;
    grove.z += WORLD_FRAME;
    world_track(&grove);
    CLOSE(support_x, grove.cabin.anchor.x);
    CLOSE(support_z, grove.cabin.anchor.z);
    wildlife(&m, s, &g);
    CHECK(m.count > 0);
    game_init(&grove, &s->rules, s->rules.survey.seed);
    landscape(&m, &s->forest, &grove, 0, 0);
    CHECK(m.count > 65000 && !m.failed);
    game_conditions(&grove, 1, 0, 12);
    wildlife(&m, s, &grove);
    uint32_t flight = geometry_hash(&m);
    size_t in_flight = m.count;
    game_tick(&grove, .18f, 0, 0);
    wildlife(&m, s, &grove);
    CHECK(flight != geometry_hash(&m));
    grove.hour = 0;
    wildlife(&m, s, &grove);
    CHECK(m.count < in_flight);
    CHECK(!strcmp(s->rules.layout[0].name, "Hearth Room"));
    Pose pose = {{0}, 0};
    Room example = {.kind = 4, .key = 218, .blinds = 54, .panes = 23};
    mesh_clear(&m);
    CHECK(script_room(s, &m, pose, &example, 6, -1, "alpine"));
    original = geometry_hash(&m);
    size = m.count;
    mesh_clear(&m);
    CHECK(script_room(s, &m, pose, &example, 6, -1, "alpine"));
    CHECK(size == m.count && original == geometry_hash(&m));
    CHECK(luaL_dostring(s->L, "function build_room() error('broken room') end; function "
                              "build_visitor() error('broken visitor') end") == LUA_OK);
    CHECK(!script_room(s, &m, pose, &example, 0, -1, "alpine") && strstr(s->error, "broken room"));
    CHECK(!script_visitor(s, &m, pose, &s->rules.places[0].shelter.sight[0], 0));
    cabin_forget(&g.book);
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
    CHECK(mountain_sightings() & (1 << DEER));
    CHECK(!strcmp(mountain_species(MARMOT), "Yellow-bellied marmot") && !mountain_species(99)[0]);
    int last = (int)mountain_value(20) - 1;
    CLOSE(mountain_extension(0, 0), mountain_path(last, 0));
    CLOSE(mountain_extension(-2, 1), mountain_path(last, 1));
    CHECK(isnan(mountain_extension(INFINITY, 0)) && isnan(mountain_extension(1e30, 0)));
    double remote = 100000001;
    double span = hypot(mountain_extension(remote + 1, 0) - mountain_extension(remote, 0),
                        mountain_extension(remote + 1, 1) - mountain_extension(remote, 1));
    CHECK(span >= TRAIL_STEP && span < 2 * TRAIL_STEP);
    mountain_resize(260, 160);
    mountain_frame();
    uint32_t checksum = display_sample(260, 160);
    mountain_seed(271);
    mountain_select(0, 3);
    mountain_select(1, 2);
    CHECK(!strcmp(mountain_label(2), "Snow") && !strcmp(mountain_option(1, 2), "Snow"));
    mountain_conditions(3, 2, 0);
    mountain_frame();
    CHECK(checksum != display_sample(260, 160));
    mountain_look(.2f, -.2f);
    mountain_input(.7f, .1f);
    mountain_walk(.1f, 1, 0);
    for (int k = 0; k <= 27; k++)
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
    SDL_Keycode keys[] = {SDLK_H,      SDLK_E, SDLK_E, SDLK_R, SDLK_E, SDLK_H, SDLK_T,
                          SDLK_U,      SDLK_Y, SDLK_I, SDLK_O, SDLK_B, SDLK_N, SDLK_F1,
                          SDLK_ESCAPE, SDLK_P, SDLK_M, SDLK_M, SDLK_J};
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++)
        event_key(keys[i]);
    CHECK(mountain_action(ACT_HOME));
    mountain_look(PI, 0);
    for (int i = 0; i < 65; i++)
        mountain_walk(.04f, 1, 0);
    CHECK(mountain_value(13) == NEAR_ROOM_DOOR && mountain_value(38) == 1);
    event_key(SDLK_F);
    CHECK(mountain_value(38) == 2 && !mountain_value(12));
    event_key(SDLK_E);
    CHECK(mountain_value(12) == 1);
    event_key(SDLK_K);
    event_key(SDLK_L);
    event_key(SDLK_X);
    CHECK(mountain_value(39) == 1 && mountain_value(40) && mountain_value(41));
    CHECK(mountain_sound());
    for (int i = 0; i < 110; i++)
        mountain_walk(.20f, 0, 0);
    mountain_frame();
    event_key(SDLK_K);
    mountain_frame();
    event_key(SDLK_K);
    mountain_frame();
    event_key(SDLK_K);
    event_key(SDLK_L);
    event_key(SDLK_X);
    CHECK(!mountain_sound());
    mountain_seed((uint32_t)mountain_value(23));
    CHECK(!strcmp(mountain_label(0), mountain_label(4)));
    CHECK(!strcmp(mountain_label(5), "Wyoming"));
    CHECK(mountain_value(8) > 2100 && mountain_value(8) < 2110);
    CHECK(mountain_wet(-110, -20) && !mountain_wet(0, 0));
    CHECK(mountain_path(-1, 0) == 0 && mountain_path(0, 1) == 0 && mountain_path(0, 0) == 0);
    CHECK(!mountain_visit(-1) && !mountain_visit(16) && !*mountain_entry(40, 0));
    CHECK(isnan(mountain_ascent(-1, 0)) && !mountain_observe(0));
    event_key(SDLK_RIGHTBRACKET);
    event_key(SDLK_LEFTBRACKET);
    CHECK(mountain_sound());
    for (int i = 0; i < 12; i++) {
        CHECK(mountain_visit(i));
        CHECK(!strcmp(mountain_entry(i, 0), mountain_label(0)) &&
              !strcmp(mountain_entry(i, 1), mountain_label(9)));
        for (int k = 6; k <= 11; k++)
            CHECK(*mountain_label(k));
        CHECK(*mountain_entry(i, 2) &&
              !strcmp(mountain_entry(i, 3), i >= LUNAR ? "Beyond Earth" : "Earth"));
        for (int k = 28; k <= 37; k++)
            CHECK(isfinite(mountain_value(k)));
        if (i == ALPINE)
            CHECK(isfinite(mountain_ascent(2, 1)) && isfinite(mountain_ascent(4, 0)));
        if (i == BASALT) {
            for (int k = 1; k <= 7; k++) {
                CHECK(walk_to(mountain_path(k, 0), mountain_path(k, 1)));
                if (k == 3)
                    CHECK(mountain_action(ACT_USE));
            }
            double yaw = atan2(mountain_path(14, 0) - mountain_path(7, 0),
                               mountain_path(7, 1) - mountain_path(14, 1));
            for (int j = 0; j < 3; j++) {
                CHECK(walk_to(mountain_path(7, 0) + 3.5 * cos(yaw) + (j - 1) * 3.2 * sin(yaw),
                              mountain_path(7, 1) + 3.5 * sin(yaw) - (j - 1) * 3.2 * cos(yaw)));
                CHECK(mountain_observe(0) == (j == 2));
            }
            CHECK(mountain_value(30) == 2);
        }
        if (i == DUNES || i == LUNAR || i == REEFS || i == CINDER || i == PRISM) {
            mountain_conditions(3, 2, 17.1f);
            CHECK(*mountain_option(1, 2));
            mountain_frame();
            CHECK(display_sample(260, 160));
        }
    }
    mountain_variant(752);
    CHECK(mountain_value(28) == CINDER && mountain_value(7) == 752);
    mountain_visit(0);
    mountain_variant(3102);
    CHECK(mountain_value(28) == ALPINE && mountain_value(7) == 3102);
    CHECK(!mountain_sound());
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
    CHECK(mountain_open(assets, 180, 120) && mountain_value(38) == 2 && mountain_visit(BASALT));
    CHECK(mountain_value(38) == 2 && !mountain_value(39) && !mountain_value(40) &&
          !mountain_value(41));
    CHECK(mountain_value(30) == 2 && strstr(mountain_label(8), mountain_entry(BASALT, 2)));
    mountain_close();
    remove_test_journal();
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    Script bad = {0}, s = {0};
    CHECK(!script_load(&bad, "/missing-trail"));
    CHECK(bad.error[0]);
    script_close(&bad);
    CHECK(script_load(&s, argv[1]));
    maths(&s.rules);
    lighting(&s.rules);
    hiking(&s.rules);
    marked_hikes(&s.rules, argv[1]);
    endless_terrain(&s.rules);
    expeditions(&s);
    alpine_footing(&s.rules);
    high_passes(&s.rules);
    living_world(&s);
    destination_shelters(&s);
    composition(&s);
    script_close(&s);
    isolated_journal();
    platform(argv[1]);
    printf("%d unit assertions passed.\n", assertions);
    return 0;
}
