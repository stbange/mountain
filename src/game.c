#include "mountain.h"
#include <math.h>
#include <string.h>

void game_init(Game *g, const Rules *r, uint32_t seed) {
    game_destination(g, r, seed, destination_for_seed(r, seed));
}
void game_destination(Game *g, const Rules *r, uint32_t seed, int destination) {
    memset(g, 0, sizeof *g);
    g->rules = r;
    world_destination(&g->world, seed, r, destination);
    g->changing_weather = true;
    g->yaw = trail_bearing(&g->world);
    g->pitch = g->world.survey ? -.008f : .04f;
    game_conditions(g, r->season, r->weather, r->hour);
}
void game_conditions(Game *g, int season, int weather, float hour) {
    g->season = ((season % SEASONS) + SEASONS) % SEASONS;
    g->weather = ((weather % WEATHERS) + WEATHERS) % WEATHERS;
    g->hour = fmodf(fmodf(hour, 24) + 24, 24);
}
void select_season(Game *g, int season) {
    g->season = ((season % SEASONS) + SEASONS) % SEASONS;
    g->cycle_seasons = false;
    g->season_clock = 0;
}
void select_weather(Game *g, int weather) {
    g->weather = ((weather % WEATHERS) + WEATHERS) % WEATHERS;
    g->changing_weather = false;
    g->weather_clock = 0;
}
const char *trail_name(const Game *g) {
    if (g->world.survey)
        return g->world.survey->name;
    if (g->world.place && g->world.profile > SURVEYED)
        return g->world.place->name;
    return g->rules->trails[g->world.seed % TRAILS];
}
Vec cabin_world(const Cabin *c, float x, float y, float z) {
    return (Vec){c->x + cosf(c->angle) * x - sinf(c->angle) * z, c->y + y,
                 c->z + sinf(c->angle) * x + cosf(c->angle) * z};
}
Vec cabin_local(const Cabin *c, float x, float z) {
    float dx = x - c->x, dz = z - c->z;
    return (Vec){dx * cosf(c->angle) + dz * sinf(c->angle), 0,
                 -dx * sinf(c->angle) + dz * cosf(c->angle)};
}
float ground(const Game *g, float x, float z) {
    float h = height_at(&g->world, x, z);
    if (g->cabin.present) {
        Vec p = cabin_local(&g->cabin, x, z);
        const Layout *l = g->rules->layout;
        float edge = fmaxf(fabsf(p.x) - l->width, fabsf(p.z) - l->depth - .3f);
        h = mixf(fminf(h, g->cabin.y - .13f), h, smooth(-.2f, 2.9f, edge));
        if (fabsf(p.x) < 1.55f && p.z > l->depth - .2f && p.z < l->depth + 2.3f)
            h = mixf(g->cabin.y, h, smooth(l->depth - .2f, l->depth + 2.3f, p.z));
    }
    return h;
}
Vec eye(const Game *g) {
    float h = g->cabin.inside
                  ? g->cabin.y
                  : fmaxf(water_at(&g->world, g->x, g->z) - .25f, ground(g, g->x, g->z));
    if (g->cabin.inside && cabin_room(&g->book, g->cabin.room)->kind == 1) {
        Cabin frame = cabin_frame(g);
        Vec p = cabin_local(&frame, g->x, g->z);
        const Layout *l = cabin_layout(g);
        float edge = fminf(l->width * .55f - fabsf(p.x), l->depth * .55f - fabsf(p.z));
        h -= .92f * smooth(.04f, 1.65f, edge);
    }
    return (Vec){g->x, (g->fall_time > 0 ? g->fall_y : h) + 1.65f, g->z};
}
void game_look(Game *g, float dx, float dy) {
    g->yaw = remainderf(g->yaw + dx, 2 * PI);
    g->pitch = clampf(g->pitch + dy, -1.33f, 1.33f);
}
static void place(Game *g, float lx, float lz, bool face_back) {
    Vec p = cabin_world(&g->cabin, lx, 0, lz);
    g->x = p.x;
    g->z = p.z;
    g->yaw = g->cabin.angle + (face_back ? PI : 0);
    g->pitch = 0;
}
Interaction game_near(const Game *g) {
    const Cabin *c = &g->cabin;
    if (!c->present)
        return field_near(g);
    Vec p = cabin_local(c, g->x, g->z);
    if (c->inside)
        return cabin_near(g);
    float d = g->rules->layout[0].depth;
    if (fabsf(p.x) > 2.1f || p.z < d - .35f || p.z > d + 7.35f)
        return field_near(g);
    return c->open ? NEAR_ENTER : NEAR_OPEN;
}
static void enter(Game *g) {
    g->cabin.inside = true;
    g->cabin.room = 0;
    place(g, 0, g->rules->layout[0].depth - 2.45f, false);
    g->yaw -= .16f;
    g->pitch = -.045f;
}
static void grove_support(Game *g) {
    Cabin *c = &g->cabin;
    int cx = (int)floorf(c->x / 7), cz = (int)floorf(c->z / 7);
    const Layout *l = g->rules->layout;
    float closest = 2.f;
    for (int j = cz - 2; j <= cz + 2; j++)
        for (int i = cx - 2; i <= cx + 2; i++) {
            Tree tree;
            if (!world_tree(&g->world, i, j, &tree) || tree.height < 8)
                continue;
            Vec p = cabin_local(c, tree.x, tree.z);
            float distance = fabsf(fabsf(p.x) - (l->width + .43f));
            if (distance < closest && fabsf(p.x) > l->width + .15f &&
                fabsf(p.x) < l->width + 1.15f && fabsf(p.z) < l->depth - .1f) {
                closest = distance;
                c->anchor = tree;
                c->anchor_dx = tree.x - c->x;
                c->anchor_dz = tree.z - c->z;
                c->tree_supported = true;
            }
        }
    if (c->tree_supported)
        c->y += .9f;
}
bool game_action(Game *g, Action action) {
    Cabin *c = &g->cabin;
    if (g->fall_time > 0 && (action == ACT_CABIN || action == ACT_USE || action == ACT_DOOR))
        return false;
    switch (action) {
    case ACT_CABIN:
        if (c->inside)
            return false;
        if (c->present) {
            *c = (Cabin){0};
            return true;
        }
        c->present = true;
        c->angle = g->yaw;
        c->x = g->x + sinf(g->yaw) * (g->rules->layout[0].depth + 6.8f);
        c->z = g->z - cosf(g->yaw) * (g->rules->layout[0].depth + 6.8f);
        c->site_x = g->world.ox + (double)c->x;
        c->site_z = g->world.oz + (double)c->z;
        {
            Vec threshold = cabin_world(c, 0, 0, g->rules->layout[0].depth);
            c->y = fmaxf(water_at(&g->world, threshold.x, threshold.z) + .24f,
                         height_at(&g->world, threshold.x, threshold.z) + .17f);
        }
        grove_support(g);
        return true;
    case ACT_BLINDS:
        return c->inside && cabin_use(g, action);
    case ACT_DOOR:
    case ACT_USE: {
        if (c->inside)
            return cabin_use(g, action);
        Interaction here = game_near(g);
        if (here == NEAR_OPEN || here == NEAR_ENTER) {
            if (here == NEAR_ENTER && action == ACT_USE)
                enter(g);
            else
                c->open = !c->open;
            return true;
        }
        return action == ACT_USE && (here == NEAR_READ || here == NEAR_TREASURE) && field_read(g);
    }
    case ACT_HOME:
        if (g->fall_time > 0)
            return false;
        if (!c->present)
            game_action(g, ACT_CABIN);
        world_position(g, c->site_x, c->site_z);
        cabin_home(g);
        g->exposed = false;
        return true;
    case ACT_LOOKOUT_WEATHER:
        g->book.weather = (g->book.weather + 1) % 4;
        g->book.revision++;
        return true;
    case ACT_LOOKOUT_PREDATORS:
        g->book.predators = !g->book.predators;
        g->book.revision++;
        return true;
    case ACT_LOOKOUT_GIANTS:
        g->book.giants = !g->book.giants;
        g->book.revision++;
        return true;
    case ACT_TIME:
        g->hour = fmodf(g->hour + 4, 24);
        return true;
    case ACT_SEASON:
        select_season(g, g->season + 1);
        return true;
    case ACT_WEATHER:
        select_weather(g, g->weather + 1);
        return true;
    case ACT_CYCLE_SEASONS:
        g->cycle_seasons = !g->cycle_seasons;
        g->season_clock = 0;
        return true;
    case ACT_CHANGING_WEATHER:
        g->changing_weather = !g->changing_weather;
        g->weather_clock = 0;
        return true;
    default:
        return false;
    }
}
static void move(Game *g, float x, float z) {
    Cabin *c = &g->cabin;
    if (c->present) {
        Vec p = cabin_local(c, x, z), old = cabin_local(c, g->x, g->z);
        bool gap = fabsf(p.x) < .79f;
        const Layout *l = g->rules->layout;
        if (c->inside) {
            float previous_x = g->x, previous_z = g->z;
            uint32_t previous_room = c->room;
            cabin_walk(g, (Vec){x, 0, z});
            if (previous_room == c->room && c->inside)
                g->walked += hypotf(g->x - previous_x, g->z - previous_z);
            return;
        } else if (fabsf(p.x) < l->width + .25f && fabsf(p.z) < l->depth + .23f) {
            if (gap && c->open && old.z >= l->depth + .20f && p.z > l->depth - .35f)
                enter(g);
            return;
        }
    }
    g->walked += hypotf(x - g->x, z - g->z);
    g->x = x;
    g->z = z;
}
bool cabin_in_view(const Game *g) {
    return g->cabin.present &&
           (g->cabin.inside || hypotf(g->cabin.x - g->x, g->cabin.z - g->z) <= 1000);
}
/* A multiple of every placement grid keeps small local coordinates in the
   renderer, while authoritative sites and terrain cells retain their identity. */
void world_position(Game *g, double x, double z) {
    int64_t ox = (int64_t)floor(x / WORLD_FRAME + .5) * WORLD_FRAME;
    int64_t oz = (int64_t)floor(z / WORLD_FRAME + .5) * WORLD_FRAME;
    g->x = (float)(x - ox);
    g->z = (float)(z - oz);
    if (ox != g->world.ox || oz != g->world.oz) {
        g->world.ox = ox;
        g->world.oz = oz;
        if (g->cabin.present) {
            g->cabin.x = (float)(g->cabin.site_x - g->world.ox);
            g->cabin.z = (float)(g->cabin.site_z - g->world.oz);
            if (g->cabin.tree_supported) {
                g->cabin.anchor.x = g->cabin.x + g->cabin.anchor_dx;
                g->cabin.anchor.z = g->cabin.z + g->cabin.anchor_dz;
            }
        }
    }
    trail_window(&g->world, g->x, g->z);
}
void world_track(Game *g) {
    world_position(g, g->world.ox + (double)g->x, g->world.oz + (double)g->z);
}
void game_tick(Game *g, float dt, float f, float r) {
    dt = clampf(dt, 0, .20f);
    g->elapsed += dt;
    g->note_timer = fmaxf(0, g->note_timer - dt);
    g->hour = fmodf(g->hour + dt * g->rules->clock_rate, 24);
    if (g->cycle_seasons)
        g->season_clock += dt;
    if (g->cycle_seasons && g->season_clock >= g->rules->season_interval) {
        g->season = (g->season + 1) % SEASONS;
        g->season_clock = 0;
    }
    if (g->changing_weather)
        g->weather_clock += dt;
    if (g->changing_weather && g->weather_clock >= g->rules->weather_interval) {
        unsigned pick = hash32(g->world.seed + (uint32_t)(uint64_t)g->elapsed);
        g->weather = (g->weather + 1 + (int)(pick % 3)) % WEATHERS;
        g->weather_clock = 0;
    }
    if (footing_recover(g, dt))
        return;
    float n = hypotf(f, r);
    if (n < .001f)
        return;
    float speed = g->rules->pace * dt / fmaxf(1, n);
    if (g->cabin.inside)
        speed *= .70f;
    float x = g->x + speed * (sinf(g->yaw) * f + cosf(g->yaw) * r);
    float z = g->z + speed * (-cosf(g->yaw) * f + sinf(g->yaw) * r);
    float old_x = g->x, old_z = g->z;
    move(g, x, z);
    footing_move(g, old_x, old_z, dt);
    world_track(g);
}

const char *weather_label(const Game *g, int index) {
    if (g->world.profile == LUNAR)
        return "Airless";
    if (index == 2 && g->world.profile == DUNES)
        return "Dust";
    if (index == 2 && g->world.profile >= PRISM)
        return g->world.profile == CINDER ? "Ashfall" : "Drift";
    if (g->season == 3 && index == 2 && g->world.profile <= SURVEYED)
        return g->rules->winter_precipitation;
    return g->rules->skies[(unsigned)index % WEATHERS];
}
