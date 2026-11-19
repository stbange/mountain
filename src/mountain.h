#ifndef MOUNTAIN_H
#define MOUNTAIN_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PI 3.14159265359f
#define RIDGES 36
#define SEASONS 4
#define WEATHERS 4
#define TRAILS 8
#define ROUTE_POINTS 512
#define TRAIL_TEXTURE_SIZE 512

typedef struct {
    float x, y, z;
} Vec;
typedef struct {
    float r, g, b;
} Color;
typedef struct {
    float m[16];
} Mat;
typedef struct {
    float x, z, h, rx, rz, angle;
} Peak;
typedef struct {
    unsigned char *data;
    unsigned side;
    float cell, scale, surface, bearing;
    uint32_t seed;
    char name[48], region[48], file[96];
    int shore[4];
} Survey;
typedef struct {
    float x, z;
} Place;
typedef struct {
    Place point[ROUTE_POINTS];
    int count, overlook;
    Place sight[2];
    float length;
} Route;
typedef struct {
    uint32_t seed;
    const Survey *survey;
    Route route;
    float phase, water, relief, meadow_level;
    Peak peaks[RIDGES];
} World;
typedef struct {
    float hour, clock_rate, pace, relief, water, weather_interval, season_interval;
    Survey survey;
    uint32_t seed;
    int season, weather;
    char trails[TRAILS][48], seasons[SEASONS][24], skies[WEATHERS][24];
    int field_tiles[SEASONS][2];
    char winter_precipitation[24];
    Color meadow[SEASONS], forest[SEASONS];
} Rules;
typedef struct {
    float x, y, z, height, angle, tint;
    int tile;
} Tree;
typedef struct {
    bool present, open, inside, tree_supported;
    Tree anchor;
    uint64_t room;
    float x, y, z, angle;
} Cabin;
typedef struct {
    World world;
    const Rules *rules;
    Cabin cabin;
    float x, z, yaw, pitch, hour, elapsed, weather_clock, season_clock, walked;
    bool cycle_seasons, changing_weather;
    int season, weather;
} Game;
typedef enum {
    ACT_CABIN = 1,
    ACT_USE,
    ACT_TIME,
    ACT_SEASON,
    ACT_WEATHER,
    ACT_HOME,
    ACT_CYCLE_SEASONS,
    ACT_CHANGING_WEATHER
} Action;
typedef enum { NEAR_NONE, NEAR_OPEN, NEAR_ENTER, NEAR_EXIT, NEAR_NEXT, NEAR_BACK } Interaction;

float clampf(float v, float lo, float hi);
float mixf(float a, float b, float t);
float smooth(float a, float b, float x);
uint32_t hash32(uint32_t x);
float random2(int x, int z, uint32_t seed);
float noise2(float x, float z, uint32_t seed);
Color rgb(unsigned hex);
Color blend(Color a, Color b, float t);
Color shade(Color c, float light);
void world_init(World *w, uint32_t seed, const Rules *rules);
bool world_tree(const World *w, int i, int j, Tree *tree);
float height_at(const World *w, float x, float z);
bool survey_load(Survey *s, const char *root);
float survey_height(const Survey *s, float x, float z);
bool survey_lake(const Survey *s, float x, float z);
float water_at(const World *w, float x, float z);
void route_generate(World *w);
float trail_bearing(const World *w);
float route_distance(const Route *r, float x, float z);
void route_texture(const Route *r, unsigned char *pixels);
void select_season(Game *g, int season);
void select_weather(Game *g, int weather);
bool insects_active(const Game *g);
float sunlight_factor(float hour);
float alpenglow(float hour);
void game_init(Game *g, const Rules *rules, uint32_t seed);
float ground(const Game *g, float x, float z);
Vec eye(const Game *g);
Vec cabin_world(const Cabin *c, float x, float y, float z);
Vec cabin_local(const Cabin *c, float x, float z);
void game_look(Game *g, float dx, float dy);
void game_tick(Game *g, float dt, float forward, float right);
Interaction game_near(const Game *g);
bool game_action(Game *g, Action action);
void game_conditions(Game *g, int season, int weather, float hour);
const char *trail_name(const Game *g);

typedef struct {
    float x, y, z, r, g, b, u, v, material;
} Vertex;
typedef struct {
    Vertex *v;
    size_t count, capacity;
    bool failed;
} Mesh;
typedef struct {
    Vec origin;
    float angle;
} Pose;
void mesh_clear(Mesh *m);
void mesh_free(Mesh *m);
void tri(Mesh *m, Vec a, Vec b, Vec c, Color ca, Color cb, Color cc, float material);
void quad(Mesh *m, Vec a, Vec b, Vec c, Vec d, Color color, float material);
void tilted_box(Mesh *m, Pose p, Vec center, Vec size, Color color, float material, float roll);
void box_material(Mesh *m, Pose p, Vec center, Vec size, Color color, float material);
void box(Mesh *m, Pose p, Vec center, Vec size, Color color);
void sprite(Mesh *m, float x, float y, float z, float width, float height, float angle, int tile,
            Color tint);
void tex_quad(Mesh *m, Vec a, Vec b, Vec c, Vec d, float u0, float v0, float u1, float v1,
              Color tint, float material);
Vec transform(Pose p, Vec v);
Mat identity(void);
Mat multiply(Mat a, Mat b);
Mat view_matrix(Vec pos, float yaw, float pitch);
Mat perspective(float fovy, float aspect, float near_z, float far_z);

struct lua_State;
typedef struct {
    struct lua_State *L;
    Rules rules;
    char error[240], label[64];
} Script;
bool script_load(Script *s, const char *assets);
void script_close(Script *s);
bool script_room(Script *s, Mesh *m, Pose pose, uint64_t index, bool open, bool main_front);
const char *room_name(Script *s, uint64_t index);
void landscape(Mesh *m, const Game *g, float cx, float cz);
void cabin_mesh(Mesh *m, Script *s, const Game *g);
void wildlife(Mesh *m, const Game *g);
#endif
