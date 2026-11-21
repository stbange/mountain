#include "mountain.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void strings(lua_State *L, const char *field, char *out, size_t stride, int count) {
    lua_getfield(L, 1, field);
    luaL_checktype(L, -1, LUA_TTABLE);
    for (int i = 0; i < count; i++) {
        lua_geti(L, -1, i + 1);
        snprintf(out + i * stride, stride, "%s", luaL_checkstring(L, -1));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}
static float number(lua_State *L, int table, const char *field, float a, float b) {
    lua_getfield(L, table, field);
    float v = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);
    if (!isfinite(v) || v < a || v > b)
        luaL_error(L, "invalid %s", field);
    return v;
}
static int read_rules(lua_State *L) {
    Rules *r = lua_touserdata(L, lua_upvalueindex(1));
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "seed");
    r->seed = (uint32_t)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    r->hour = number(L, 1, "hour", 0, 24);
    r->pace = number(L, 1, "pace", .1f, 20);
    r->clock_rate = number(L, 1, "clock_rate", 0, 1);
    r->relief = number(L, 1, "relief", .3f, 3);
    r->water = number(L, 1, "water", -25, 20);
    r->weather_interval = number(L, 1, "weather_interval", 5, 86400);
    lua_getfield(L, 1, "season_interval");
    r->season_interval = lua_isnil(L, -1) ? 480 : number(L, 1, "season_interval", 5, 86400);
    lua_pop(L, 1);
    lua_getfield(L, 1, "survey");
    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        Survey *s = &r->survey;
        lua_getfield(L, -1, "name");
        snprintf(s->name, sizeof s->name, "%s", luaL_checkstring(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, -1, "region");
        snprintf(s->region, sizeof s->region, "%s", luaL_optstring(L, -1, "Surveyed terrain"));
        lua_pop(L, 1);
        lua_getfield(L, -1, "file");
        snprintf(s->file, sizeof s->file, "%s", luaL_checkstring(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, -1, "seed");
        s->seed = (uint32_t)luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        s->scale = number(L, -1, "scale", 1, 30);
        s->surface = number(L, -1, "surface", -1000, 8000);
        s->bearing = number(L, -1, "bearing", -PI, PI);
    }
    lua_pop(L, 1);
    r->season = (int)number(L, 1, "season", 0, 3);
    r->weather = (int)number(L, 1, "weather", 0, 3);
    strings(L, "trails", &r->trails[0][0], sizeof r->trails[0], TRAILS);
    strings(L, "seasons", &r->seasons[0][0], sizeof r->seasons[0], SEASONS);
    strings(L, "skies", &r->skies[0][0], sizeof r->skies[0], WEATHERS);
    lua_getfield(L, 1, "winter_precipitation");
    snprintf(r->winter_precipitation, sizeof r->winter_precipitation, "%s",
             luaL_optstring(L, -1, "Snow"));
    lua_pop(L, 1);
    lua_getfield(L, 1, "field_tiles");
    for (int i = 0; i < SEASONS; i++) {
        r->field_tiles[i][0] = 4;
        r->field_tiles[i][1] = 8;
        if (!lua_isnil(L, -1)) {
            lua_geti(L, -1, i + 1);
            for (int k = 0; k < 2; k++) {
                lua_geti(L, -1, k + 1);
                lua_Integer tile = luaL_checkinteger(L, -1);
                luaL_argcheck(L, tile >= 0 && tile < 16, 1, "field tile outside atlas");
                r->field_tiles[i][k] = (int)tile;
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    const char *names[] = {"meadow", "forest"};
    Color *palettes[] = {r->meadow, r->forest};
    for (int p = 0; p < 2; p++) {
        lua_getfield(L, 1, names[p]);
        luaL_checktype(L, -1, LUA_TTABLE);
        for (int i = 0; i < SEASONS; i++) {
            lua_geti(L, -1, i + 1);
            palettes[p][i] = rgb((unsigned)luaL_checkinteger(L, -1));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    Color *wood[] = {r->needles, r->bark, r->autumn};
    const char *fields[] = {"needles", "bark", "autumn_canopy"};
    for (int field = 0; field < 3; field++) {
        lua_getfield(L, 1, fields[field]);
        for (int i = 0; i < 4; i++) {
            wood[field][i] = r->forest[field == 2 ? 2 : 1];
            if (!lua_isnil(L, -1)) {
                lua_geti(L, -1, i + 1);
                wood[field][i] = rgb((unsigned)luaL_checkinteger(L, -1));
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }
    lua_getfield(L, 1, "tree_shapes");
    for (int i = 0; i < 4; i++) {
        TreeShape *shape = &r->tree_shapes[i];
        *shape = (TreeShape){
            .first_bough = 4, .bough_variation = 3, .width = 1, .droop = 1, .foliage = 1};
        if (lua_isnil(L, -1))
            continue;
        lua_geti(L, -1, i + 1);
        luaL_checktype(L, -1, LUA_TTABLE);
        static const char *const forms[] = {"whorled", "forked", NULL};
        lua_getfield(L, -1, "form");
        shape->forked = luaL_checkoption(L, -1, NULL, forms) == 1;
        lua_pop(L, 1);
        lua_getfield(L, -1, "deciduous");
        shape->deciduous = lua_toboolean(L, -1);
        lua_pop(L, 1);
        shape->first_bough = number(L, -1, "first_bough", 1, 20);
        shape->bough_variation = number(L, -1, "bough_variation", 0, 6);
        shape->width = number(L, -1, "width", .3f, 1.6f);
        shape->droop = number(L, -1, "droop", 0, 4);
        shape->foliage = number(L, -1, "foliage", .3f, 2);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 1, "destination_file");
    snprintf(r->destination_file, sizeof r->destination_file, "%s", luaL_optstring(L, -1, ""));
    lua_pop(L, 1);
    lua_getfield(L, 1, "fauna_script");
    snprintf(r->fauna_script, sizeof r->fauna_script, "%s", luaL_optstring(L, -1, ""));
    lua_pop(L, 1);
    if (r->fauna_script[0])
        strings(L, "fauna_names", &r->fauna_names[0][0], sizeof r->fauna_names[0], FAUNA_TYPES);
    return 0;
}
static void string_at(lua_State *L, int table, int index, char *out, size_t size) {
    lua_geti(L, table, index);
    snprintf(out, size, "%s", luaL_checkstring(L, -1));
    lua_pop(L, 1);
}
static int read_destinations(lua_State *L) {
    Rules *r = lua_touserdata(L, lua_upvalueindex(1));
    static const char *const profiles[] = {"alpine", "survey", "coast", "dunes", "basalt",
                                           "mesa",   "karst",  "lunar", "prism", "fungal",
                                           "reefs",  "cinder", NULL};
    static const char *const skills[] = {"align", "dusk", "echo", NULL};
    int n = (int)luaL_len(L, 1);
    if (n < 1 || n > DEST_LIMIT)
        return luaL_error(L, "invalid destination count");
    for (int i = 0; i < n; i++) {
        Destination *d = &r->places[i];
        lua_geti(L, 1, i + 1);
        int row = lua_gettop(L);
        lua_getfield(L, row, "profile");
        d->profile = (Profile)luaL_checkoption(L, -1, NULL, profiles);
        lua_pop(L, 1);
        lua_getfield(L, row, "seed");
        d->seed = (uint32_t)luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        d->datum = number(L, row, "datum", -10000, 10000);
        lua_getfield(L, row, "name");
        snprintf(d->name, sizeof d->name, "%s", luaL_checkstring(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, row, "region");
        snprintf(d->region, sizeof d->region, "%s", luaL_checkstring(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, row, "colours");
        for (int k = 0; k < 6; k++) {
            lua_geti(L, -1, k + 1);
            d->colours[k] = rgb((unsigned)luaL_checkinteger(L, -1));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        lua_getfield(L, row, "stops");
        for (int k = 0; k < 2; k++)
            string_at(L, -1, k + 1, d->stops[k], sizeof d->stops[k]);
        lua_pop(L, 1);
        lua_getfield(L, row, "discovery");
        lua_geti(L, -1, 1);
        d->skill = (FieldSkill)luaL_checkoption(L, -1, NULL, skills);
        lua_pop(L, 1);
        for (int k = 0; k < 3; k++) {
            string_at(L, -1, k + 2, d->signs[k], sizeof d->signs[k]);
            string_at(L, -1, k + 5, d->notes[k], sizeof d->notes[k]);
        }
        lua_pop(L, 2);
    }
    r->place_count = n;
    return 0;
}
static bool failure(Script *s) {
    snprintf(s->error, sizeof s->error, "%s", lua_tostring(s->L, -1));
    lua_pop(s->L, 1);
    return false;
}
static int draw_limb(lua_State *L) {
    Mesh *m = lua_touserdata(L, lua_upvalueindex(1));
    Vec p = {luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)};
    Vec q = {luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6)};
    float a = luaL_checknumber(L, 7), b = luaL_checknumber(L, 8);
    int sides = (int)clampf(luaL_checkinteger(L, 9), 3, 20);
    float flat = luaL_checknumber(L, 10);
    Color c = rgb((unsigned)luaL_checkinteger(L, 11));
    float material = luaL_checknumber(L, 12);
    Vec d = {q.x - p.x, q.y - p.y, q.z - p.z};
    float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < .00001f)
        return 0;
    d = (Vec){d.x / len, d.y / len, d.z / len};
    float h = hypotf(d.x, d.z);
    Vec u = h > .001f ? (Vec){-d.z / h, 0, d.x / h} : (Vec){1, 0, 0};
    Vec v = {d.y * u.z, d.z * u.x - d.x * u.z, -d.y * u.x};
    Vec lower[21], upper[21];
    for (int i = 0; i <= sides; i++) {
        float angle = 2 * PI * (i % sides) / sides, co = cosf(angle), si = sinf(angle) * flat;
        Vec n = {u.x * co + v.x * si, u.y * co + v.y * si, u.z * co + v.z * si};
        lower[i] = (Vec){p.x + a * n.x, p.y + a * n.y, p.z + a * n.z};
        upper[i] = (Vec){q.x + b * n.x, q.y + b * n.y, q.z + b * n.z};
    }
    for (int i = 0; i < sides; i++) {
        Color lit = shade(c, .88f + .16f * cosf(2 * PI * i / sides + .4f));
        quad(m, lower[i], upper[i], upper[i + 1], lower[i + 1], lit, material);
        if (b > 0)
            tri(m, q, upper[i], upper[i + 1], c, c, c, material);
        if (a > 0)
            tri(m, p, lower[i + 1], lower[i], lit, lit, lit, material);
    }
    return 0;
}
static bool load_landmarks(Script *s, const char *assets) {
    char path[1024];
    snprintf(path, sizeof path, "%s/scripts/landmarks.lua", assets);
    lua_State *L = s->L;
    if (luaL_dofile(L, path) != LUA_OK)
        return failure(s);
    for (int i = 0; i < s->rules.place_count; i++) {
        Destination *d = &s->rules.places[i];
        d->art = calloc(PROP_KINDS * PROP_FORMS, sizeof *d->art);
        if (!d->art)
            return false;
        Color c = d->colours[3];
        unsigned packed = ((unsigned)lroundf(c.r * 255) << 16) |
                          ((unsigned)lroundf(c.g * 255) << 8) | (unsigned)lroundf(c.b * 255);
        for (int kind = 0; kind < PROP_KINDS; kind++)
            for (int v = 0; v < PROP_FORMS; v++) {
                Mesh *shape = &d->art[kind * PROP_FORMS + v];
                lua_getglobal(L, "build_landmark");
                lua_pushinteger(L, d->profile);
                lua_pushinteger(L, kind);
                lua_pushinteger(L, v);
                lua_pushinteger(L, packed);
                lua_pushlightuserdata(L, shape);
                lua_pushcclosure(L, draw_limb, 1);
                if (lua_pcall(L, 5, 0, 0) != LUA_OK)
                    return failure(s);
                if (shape->failed)
                    return false;
            }
    }
    return true;
}
bool script_load(Script *s, const char *assets) {
    memset(s, 0, sizeof *s);
    s->L = luaL_newstate();
    if (!s->L)
        return false;
    lua_State *L = s->L;
    luaL_openlibs(L);
    char path[1024];
    snprintf(path, sizeof path, "%s/scripts/world.lua", assets);
    lua_pushlightuserdata(L, &s->rules);
    lua_pushcclosure(L, read_rules, 1);
    if (luaL_dofile(L, path) != LUA_OK || lua_pcall(L, 1, 0, 0) != LUA_OK)
        return failure(s);
    if (s->rules.destination_file[0]) {
        snprintf(path, sizeof path, "%s/scripts/%s", assets, s->rules.destination_file);
        lua_pushlightuserdata(L, &s->rules);
        lua_pushcclosure(L, read_destinations, 1);
        if (luaL_dofile(L, path) != LUA_OK || lua_pcall(L, 1, 0, 0) != LUA_OK)
            return failure(s);
    }
    if (s->rules.place_count && !load_landmarks(s, assets))
        return false;
    if (s->rules.survey.file[0] && !survey_load(&s->rules.survey, assets)) {
        snprintf(s->error, sizeof s->error, "Could not read elevation grid %s",
                 s->rules.survey.file);
        return false;
    }
    if (s->rules.fauna_script[0]) {
        snprintf(path, sizeof path, "%s/scripts/%s", assets, s->rules.fauna_script);
        if (luaL_dofile(L, path) != LUA_OK)
            return failure(s);
    }
    snprintf(path, sizeof path, "%s/scripts/cabin.lua", assets);
    if (luaL_dofile(L, path) != LUA_OK)
        return failure(s);
    luaL_checkstack(L, 8, "cabin");
    return true;
}
void script_close(Script *s) {
    if (s->L)
        lua_close(s->L);
    s->L = NULL;
    for (int i = 0; i < s->rules.place_count; i++) {
        Mesh *art = s->rules.places[i].art;
        if (art) {
            for (int j = 0; j < PROP_KINDS * PROP_FORMS; j++)
                mesh_free(&art[j]);
            free(art);
            s->rules.places[i].art = NULL;
        }
    }
    forest_free(&s->forest);
    free(s->rules.survey.data);
    s->rules.survey.data = NULL;
}
typedef struct {
    Mesh *m;
    Pose pose;
} Builder;
static int emit(lua_State *L) {
    Builder *b = lua_touserdata(L, lua_upvalueindex(1));
    Vec c = {(float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2),
             (float)luaL_checknumber(L, 3)};
    Vec size = {(float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5),
                (float)luaL_checknumber(L, 6)};
    Color color = rgb((unsigned)luaL_checkinteger(L, 7));
    int mat = (int)luaL_optinteger(L, 8, 0);
    float turn = (float)luaL_optnumber(L, 9, 0);
    Pose local = {transform(b->pose, c), b->pose.angle + turn};
    tilted_box(b->m, local, (Vec){0}, size, color, (float)mat, (float)luaL_optnumber(L, 10, 0));
    return 0;
}
bool script_room(Script *s, Mesh *m, Pose p, uint64_t index, bool open, bool front) {
    lua_State *L = s->L;
    Builder b = {m, p};
    lua_getglobal(L, "build_room");
    lua_pushinteger(L, (lua_Integer)(index ? 1 + (index - 1) % 2147483646u : 0));
    lua_pushboolean(L, open);
    lua_pushboolean(L, front);
    lua_pushlightuserdata(L, &b);
    lua_pushcclosure(L, emit, 1);
    if (lua_pcall(L, 4, 0, 0) != LUA_OK)
        return failure(s);
    return !m->failed;
}
const char *room_name(Script *s, uint64_t index) {
    lua_State *L = s->L;
    lua_getglobal(L, "room_title");
    lua_pushinteger(L, (lua_Integer)(index ? 1 + (index - 1) % 2147483646u : 0));
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        failure(s);
        return "Cabin";
    }
    const char *title = lua_tostring(L, -1);
    snprintf(s->label, sizeof s->label, "%s", title ? title : "Cabin");
    lua_pop(L, 1);
    return s->label;
}

/* Each animal part sweeps a tapered volume between two anatomical landmarks.
   The loose Lua model articulates those landmarks in the animal's own frame. */
typedef struct {
    Mesh *mesh;
    const Animal *a;
} AnimalBuilder;
static int animal_part(lua_State *L) {
    AnimalBuilder *builder = lua_touserdata(L, lua_upvalueindex(1));
    const Animal *a = builder->a;
    Pose pose = {a->pos, a->yaw};
    Vec p = {luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)};
    Vec q = {luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6)};
    float width = luaL_checknumber(L, 7), depth = luaL_checknumber(L, 8);
    Color col = rgb((unsigned)luaL_checkinteger(L, 9));
    Vec d = {q.x - p.x, q.y - p.y, q.z - p.z};
    float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < .00001f)
        return 0;
    d = (Vec){d.x / len, d.y / len, d.z / len};
    float horizontal = hypotf(d.x, d.z);
    Vec side = horizontal > .001f ? (Vec){-d.z / horizontal, 0, d.x / horizontal} : (Vec){1, 0, 0};
    Vec up = {d.y * side.z - d.z * side.y, d.z * side.x - d.x * side.z,
              d.x * side.y - d.y * side.x};
    static const float sn[6] = {0, .8660254f, .8660254f, 0, -.8660254f, -.8660254f};
    static const float cs[6] = {1, .5f, -.5f, -1, -.5f, .5f};
    static const float fraction[4] = {0, .25f, .68f, 1}, radius[4] = {.14f, 1, .88f, .12f};
    Vec rings[4][6];
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 6; i++) {
            float u = width * radius[j] * cs[i], v = depth * radius[j] * sn[i], t = fraction[j];
            Vec local = {mixf(p.x, q.x, t) + u * side.x + v * up.x,
                         mixf(p.y, q.y, t) + u * side.y + v * up.y,
                         mixf(p.z, q.z, t) + u * side.z + v * up.z};
            local = (Vec){local.x * a->size, local.y * a->size, local.z * a->size};
            rings[j][i] = transform(pose, local);
        }
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 6; i++) {
            int k = (i + 1) % 6;
            Color lit = shade(col, .90f + .14f * (side.y * cs[i] + up.y * sn[i]) + .035f * cs[i]);
            quad(builder->mesh, rings[j][i], rings[j + 1][i], rings[j + 1][k], rings[j][k], lit, 0);
        }
    return 0;
}
bool script_animal(Script *s, Mesh *m, const Animal *a, int season) {
    lua_State *L = s->L;
    AnimalBuilder builder = {m, a};
    lua_getglobal(L, "build_animal");
    lua_pushinteger(L, a->kind + 1);
    lua_pushinteger(L, season);
    lua_pushnumber(L, a->phase);
    lua_pushnumber(L, a->activity);
    lua_pushinteger(L, a->id);
    lua_pushlightuserdata(L, &builder);
    lua_pushcclosure(L, animal_part, 1);
    if (lua_pcall(L, 6, 0, 0) != LUA_OK)
        return failure(s);
    return !m->failed;
}
