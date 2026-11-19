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
                int across = g.world.trail.dx ? 24 * TRAIL_TEXTURE_SIZE : 24;
                CHECK(map[center + across] == 255 || map[center - across] == 255);
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
            observe(&g, 1000000 + i * 173., -2500000 + j * 139.);
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
    mountain_seed((uint32_t)mountain_value(23));
    CHECK(!strcmp(mountain_label(0), mountain_label(4)));
    CHECK(!strcmp(mountain_label(5), "Wyoming"));
    CHECK(mountain_value(8) > 2100 && mountain_value(8) < 2110);
    CHECK(mountain_wet(-110, -20) && !mountain_wet(0, 0));
    CHECK(mountain_path(-1, 0) == 0 && mountain_path(0, 1) == 0 && mountain_path(0, 0) == 0);
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
    marked_hikes(&s.rules, argv[1]);
    endless_terrain(&s.rules);
    living_world(&s);
    composition(&s);
    script_close(&s);
    platform(argv[1]);
    printf("%d unit assertions passed.\n", assertions);
    return 0;
}
