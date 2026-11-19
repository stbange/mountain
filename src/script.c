#include "mountain.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
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
static float number(lua_State *L, const char *field, float a, float b) {
    lua_getfield(L, 1, field);
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
    r->hour = number(L, "hour", 0, 24);
    r->pace = number(L, "pace", .1f, 20);
    r->clock_rate = number(L, "clock_rate", 0, 1);
    r->relief = number(L, "relief", .3f, 3);
    r->water = number(L, "water", -25, 20);
    r->weather_interval = number(L, "weather_interval", 5, 86400);
    r->season = (int)number(L, "season", 0, 3);
    r->weather = (int)number(L, "weather", 0, 3);
    strings(L, "trails", &r->trails[0][0], sizeof r->trails[0], TRAILS);
    strings(L, "seasons", &r->seasons[0][0], sizeof r->seasons[0], SEASONS);
    strings(L, "skies", &r->skies[0][0], sizeof r->skies[0], WEATHERS);
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
    return 0;
}
static bool failure(Script *s) {
    snprintf(s->error, sizeof s->error, "%s", lua_tostring(s->L, -1));
    lua_pop(s->L, 1);
    return false;
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
