#include "mountain.h"
#include <math.h>
#include <string.h>

void game_init(Game *g, const Rules *r, uint32_t seed) {
    memset(g, 0, sizeof *g);
    g->rules = r;
    world_init(&g->world, seed, r);
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
        float edge = fmaxf(fabsf(p.x) - 4.4f, fabsf(p.z) - 4.3f);
        h = mixf(fminf(h, g->cabin.y - .13f), h, smooth(-.2f, 2.9f, edge));
        if (fabsf(p.x) < 1.55f && p.z > 3.8f && p.z < 6.3f)
            h = mixf(g->cabin.y, h, smooth(3.8f, 6.3f, p.z));
    }
    return h;
}
Vec eye(const Game *g) {
    float h = g->cabin.inside
                  ? g->cabin.y
                  : fmaxf(water_at(&g->world, g->x, g->z) - .25f, ground(g, g->x, g->z));
    return (Vec){g->x, h + 1.65f, g->z};
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
        return NEAR_NONE;
    Vec p = cabin_local(c, g->x, g->z);
    if (c->inside) {
        if (fabsf(p.x) > 2.f)
            return NEAR_NONE;
        if (p.z < -1.7f)
            return NEAR_NEXT;
        if (p.z > 1.7f)
            return c->room == 0 ? NEAR_EXIT : NEAR_BACK;
        return NEAR_NONE;
    }
    if (fabsf(p.x) > 2.1f || p.z < 3.65f || p.z > 11.35f)
        return NEAR_NONE;
    return c->open ? NEAR_ENTER : NEAR_OPEN;
}
static void cross_door(Game *g, Interaction action) {
    Cabin *c = &g->cabin;
    switch (action) {
    case NEAR_OPEN:
        c->open = true;
        break;
    case NEAR_ENTER:
        c->inside = true;
        c->room = 0;
        place(g, 0, 1.55f, false);
        g->yaw -= .16f;
        g->pitch = -.045f;
        break;
    case NEAR_EXIT:
        c->inside = false;
        place(g, 0, 5.5f, true);
        break;
    case NEAR_NEXT:
        if (c->room < UINT64_MAX)
            c->room++;
        place(g, 0, 1.55f, false);
        break;
    case NEAR_BACK:
        if (c->room)
            c->room--;
        place(g, 0, -1.55f, true);
        break;
    default:
        break;
    }
}
static void grove_support(Game *g) {
    Cabin *c = &g->cabin;
    int cx = (int)floorf(c->x / 7), cz = (int)floorf(c->z / 7);
    float closest = 2.f;
    for (int j = cz - 2; j <= cz + 2; j++)
        for (int i = cx - 2; i <= cx + 2; i++) {
            Tree tree;
            if (!world_tree(&g->world, i, j, &tree) || tree.height < 8)
                continue;
            Vec p = cabin_local(c, tree.x, tree.z);
            float distance = fabsf(fabsf(p.x) - 4.83f);
            if (distance < closest && fabsf(p.x) > 4.55f && fabsf(p.x) < 5.55f &&
                fabsf(p.z) < 3.9f) {
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
        c->x = g->x + sinf(g->yaw) * 10.8f;
        c->z = g->z - cosf(g->yaw) * 10.8f;
        c->site_x = g->world.ox + (double)c->x;
        c->site_z = g->world.oz + (double)c->z;
        {
            Vec threshold = cabin_world(c, 0, 0, 4);
            c->y = fmaxf(water_at(&g->world, threshold.x, threshold.z) + .24f,
                         height_at(&g->world, threshold.x, threshold.z) + .17f);
        }
        grove_support(g);
        return true;
    case ACT_USE: {
        Interaction action_here = game_near(g);
        if (action_here == NEAR_NONE)
            return false;
        cross_door(g, action_here);
        return true;
    }
    case ACT_HOME:
        if (!c->inside)
            return false;
        c->room = 0;
        place(g, 0, 2.65f, true);
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
        if (c->inside) {
            if (gap && p.z <= -3.85f) {
                cross_door(g, NEAR_NEXT);
                return;
            }
            if (gap && p.z >= 3.85f) {
                cross_door(g, c->room ? NEAR_BACK : NEAR_EXIT);
                return;
            }
            p.x = clampf(p.x, -4.05f, 4.05f);
            if (!gap)
                p.z = clampf(p.z, -3.65f, 3.65f);
            Vec v = cabin_world(c, p.x, 0, p.z);
            x = v.x;
            z = v.z;
        } else if (fabsf(p.x) < 4.65f && fabsf(p.z) < 4.23f) {
            if (gap && c->open && old.z >= 4.20f && p.z > 3.65f)
                cross_door(g, NEAR_ENTER);
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
void world_track(Game *g) {
    int64_t dx = (int64_t)floor(((double)g->x + WORLD_FRAME * .5) / WORLD_FRAME) * WORLD_FRAME;
    int64_t dz = (int64_t)floor(((double)g->z + WORLD_FRAME * .5) / WORLD_FRAME) * WORLD_FRAME;
    if (dx || dz) {
        g->world.ox += dx;
        g->world.oz += dz;
        g->x = (float)((double)g->x - dx);
        g->z = (float)((double)g->z - dz);
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
void game_tick(Game *g, float dt, float f, float r) {
    dt = clampf(dt, 0, .20f);
    g->elapsed += dt;
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
    float n = hypotf(f, r);
    if (n < .001f)
        return;
    float speed = g->rules->pace * dt / fmaxf(1, n);
    if (g->cabin.inside)
        speed *= .70f;
    float x = g->x + speed * (sinf(g->yaw) * f + cosf(g->yaw) * r);
    float z = g->z + speed * (-cosf(g->yaw) * f + sinf(g->yaw) * r);
    move(g, x, z);
    world_track(g);
}
