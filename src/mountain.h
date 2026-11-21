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
#define WORLD_FRAME 1344
#define TRAIL_STEP 24
#define TRAIL_WINDOW 257
#define TRAIL_SHELVES 32
#define FAUNA_TYPES 5
#define TREE_FORMS 32
#define FOREST_ATLAS 512

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
    float side, height;
} TrailKnot;
typedef struct {
    int dx, dz;
    float start_y, regional_origin;
    int64_t first, first_shelf;
    float shelves[TRAIL_SHELVES];
    TrailKnot knots[TRAIL_WINDOW];
} LongTrail;
typedef enum {
    ALPINE,
    SURVEYED,
    COAST,
    DUNES,
    BASALT,
    MESA,
    KARST,
    LUNAR,
    PRISM,
    FUNGAL,
    REEFS,
    CINDER
} Profile;
typedef enum { ALIGN, DUSK, ECHO } FieldSkill;
typedef enum { BOULDER, GROWTH, FEATURE, TRACK, NEEDLE, RELIC, WANDERER, PROP_KINDS } Prop;
#define PROP_FORMS 3
typedef struct Mesh Mesh;
#define DEST_LIMIT 16
#define SUMMIT_POINTS 23
#define REALM_POINTS 15
typedef struct {
    Profile profile;
    FieldSkill skill;
    uint32_t seed;
    float datum;
    char name[48], region[48], stops[2][36], signs[3][44], notes[3][144];
    Color colours[6];
    Mesh *art;
} Destination;
typedef struct {
    Place point[SUMMIT_POINTS], start, along, across;
    float level[SUMMIT_POINTS], rise, base;
    int count, pass;
} Ascent;
typedef struct {
    uint32_t seed;
    const Survey *survey;
    const Destination *place;
    Profile profile;
    int destination;
    float approach[REALM_POINTS];
    Ascent ascent;
    Route route;
    LongTrail trail;
    int64_t ox, oz;
    float phase, water, relief, meadow_level;
    Peak peaks[RIDGES];
} World;
typedef struct {
    bool forked, deciduous;
    float first_bough, bough_variation, width, droop, foliage;
} TreeShape;
typedef struct {
    float hour, clock_rate, pace, relief, water, weather_interval, season_interval;
    Survey survey;
    uint32_t seed;
    int season, weather;
    char trails[TRAILS][48], seasons[SEASONS][24], skies[WEATHERS][24];
    int field_tiles[SEASONS][2];
    char winter_precipitation[24];
    Color meadow[SEASONS], forest[SEASONS];
    Color needles[4], bark[4], autumn[4];
    TreeShape tree_shapes[4];
    char destination_file[96];
    Destination places[DEST_LIMIT];
    int place_count;
    char fauna_script[96], fauna_names[FAUNA_TYPES][28];
} Rules;
typedef struct {
    float x, y, z, height, angle, tint;
    int tile;
    uint32_t form;
} Tree;
typedef struct {
    bool present, open, inside, tree_supported;
    Tree anchor;
    uint64_t room;
    double site_x, site_z;
    float anchor_dx, anchor_dz;
    float x, y, z, angle;
} Cabin;
typedef struct {
    World world;
    const Rules *rules;
    Cabin cabin;
    float x, z, yaw, pitch, hour, weather_clock, season_clock;
    double walked, elapsed, safe_x, safe_z;
    uint32_t notes;
    int chimes, falls;
    float fall_time, fall_y, footing_timer, warning_timer, note_timer;
    bool exposed;
    char field_note[240];
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
typedef enum {
    NEAR_NONE,
    NEAR_OPEN,
    NEAR_ENTER,
    NEAR_EXIT,
    NEAR_NEXT,
    NEAR_BACK,
    NEAR_READ,
    NEAR_TREASURE
} Interaction;

void world_destination(World *w, uint32_t seed, const Rules *r, int destination);
void game_destination(Game *g, const Rules *r, uint32_t seed, int destination);
int destination_for_seed(const Rules *r, uint32_t seed);
float realm_height(const World *w, double x, double z);
float oasis_distance(const World *w, double x, double z);
void realm_route(World *w);
float realm_ground(const World *w, double x, double z);
void ascent_generate(World *w);
float ascent_ground(const World *w, double x, double z, float natural);
float ascent_distance(const World *w, double x, double z);
Place field_site(const World *w, int stage);
Place field_pipe(const World *w, int which);
int field_stage(const Game *g);
Interaction field_near(const Game *g);
void field_refresh(Game *g);
bool field_read(Game *g);
const char *field_hint(const Game *g);
void footing_move(Game *g, float x, float z, float dt);
bool footing_recover(Game *g, float dt);
float clampf(float v, float lo, float hi);
float mixf(float a, float b, float t);
float smooth(float a, float b, float x);
uint32_t hash32(uint32_t x);
uint32_t cell_hash(int64_t x, int64_t z, uint32_t seed);
float random2(int64_t x, int64_t z, uint32_t seed);
float noise2(double x, double z, uint32_t seed);
float raw_height(const World *w, double x, double z);
float height_global(const World *w, double x, double z);
float water_global(const World *w, double x, double z);
float starter_height(const World *w, float x, float z);
float trail_distance(const World *w, float x, float z);
void trail_window(World *w, float x, float z);
float alpine_pass_height(const World *w, double along);
double trail_station(const World *w, float x, float z);
Place trail_knot(const World *w, int64_t index);
void trail_coordinates(const World *w, int64_t index, double *x, double *z);
void world_track(Game *g);
void world_position(Game *g, double x, double z);
const char *weather_label(const Game *g, int index);
bool cabin_in_view(const Game *g);
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
void route_texture(const World *w, float cx, float cz, unsigned char *pixels);
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
struct Mesh {
    Vertex *v;
    size_t count, capacity;
    bool failed;
};
typedef enum { DEER, FOX, HARE, MARMOT, EAGLE } AnimalKind;
typedef struct {
    AnimalKind kind;
    uint32_t id;
    Vec pos;
    float yaw, size, phase, activity;
} Animal;
#define LOCAL_ANIMALS 72
int fauna_near(const Game *g, Animal *out, int capacity);
typedef struct {
    Vec origin;
    float angle;
} Pose;
void instance(Mesh *out, const Mesh *shape, Pose pose, float scale);
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

typedef struct {
    Mesh close[TREE_FORMS], middle[TREE_FORMS];
    unsigned char *atlas;
    uint32_t seed;
    int season;
} Forest;
void forest_prepare(Forest *forest, const Game *g);
void forest_free(Forest *forest);
void voxel_tree(Mesh *m, const Forest *forest, Tree t, float distance);
void mesh_tree_instance(Mesh *out, const Mesh *model, const Tree *tree, float width, float lean);
struct lua_State;
typedef struct {
    struct lua_State *L;
    Rules rules;
    Forest forest;
    char error[240], label[64];
} Script;
bool script_load(Script *s, const char *assets);
void script_close(Script *s);
bool script_room(Script *s, Mesh *m, Pose pose, uint64_t index, bool open, bool main_front);
const char *room_name(Script *s, uint64_t index);
void landscape(Mesh *m, Forest *forest, const Game *g, float cx, float cz);
void cabin_mesh(Mesh *m, Script *s, const Game *g);
void realm_scenery(Mesh *m, const Game *g, float cx, float cz);
void field_scenery(Mesh *m, const Game *g);
void realm_life(Mesh *m, const Game *g);
bool script_animal(Script *s, Mesh *m, const Animal *a, int season);
void wildlife(Mesh *m, Script *s, const Game *g);
#endif
