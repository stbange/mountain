#include "mountain.h"
#include <math.h>

static void prop(Mesh *m, const World *w, Prop kind, unsigned variant, float x, float y, float z,
                 float yaw, float size) {
    instance(m, &w->place->art[kind * PROP_FORMS + variant % PROP_FORMS], (Pose){{x, y, z}, yaw},
             size);
}
static bool occupied(const Game *g, float x, float z) {
    return g->cabin.present && hypotf(x - g->cabin.x, z - g->cabin.z) < 8;
}
static void scenery_cell(Mesh *m, const Game *g, float x, float z, uint32_t id, float dist) {
    const World *w = &g->world;
    if (occupied(g, x, z) || trail_distance(w, x, z) < 3.6f)
        return;
    float y = height_at(w, x, z), liquid = water_at(w, x, z);
    float angle = random2(1, 1, id) * 2 * PI, size = .58f + 1.1f * random2(2, 1, id);
    Prop kind = (id % 5 == 0) ? BOULDER : GROWTH;
    switch (w->profile) {
    case DUNES: {
        float d = oasis_distance(w, w->ox + (double)x, w->oz + (double)z);
        if (d > 65 || d < 18) {
            if (id % 31 || dist > 100)
                return;
            kind = BOULDER;
            size *= .47f;
        } else {
            kind = GROWTH;
            size *= 1.1f;
        }
        break;
    }
    case COAST:
        if (id % 31 == 0)
            kind = FEATURE;
        else if (id % 3 == 1)
            kind = BOULDER;
        if (kind == FEATURE && (y > 5 || dist < 25))
            kind = BOULDER;
        if (kind == GROWTH && y < 4)
            return;
        break;
    case BASALT:
        kind = id % 4 == 0 ? FEATURE : BOULDER;
        if (kind == BOULDER && id % 2 == 0)
            return;
        break;
    case MESA:
        if (id % 23 == 0 && dist > 32)
            kind = FEATURE;
        break;
    case KARST:
        if (y > 72)
            kind = BOULDER;
        break;
    case LUNAR:
        kind = id % 17 == 0 ? FEATURE : BOULDER;
        size *= .65f;
        break;
    case PRISM:
        if (id % 5 == 0) {
            kind = FEATURE;
            size *= 2.7f;
        }
        break;
    case FUNGAL:
        if (id % 3 == 0) {
            kind = FEATURE;
            size *= 1.7f;
        } else
            size *= .73f;
        break;
    case REEFS:
        if (id % 8 == 0 && dist > 27) {
            kind = FEATURE;
            size *= 1.7f;
        }
        break;
    case CINDER:
        if (id % 11 == 0) {
            kind = FEATURE;
            size *= 1.3f;
        }
        break;
    default:
        return;
    }
    if (y < liquid - .5f && kind != FEATURE)
        return;
    prop(m, w, kind, id, x, y - .08f, z, angle, size);
}
void realm_scenery(Mesh *m, const Game *g, float cx, float cz) {
    const World *w = &g->world;
    if (!w->place || !w->place->art)
        return;
    int ci = (int)floorf(cx / 21),
        cj = (int)floorf(cz / 21); /* 21 divides the floating-origin frame. */
    for (int j = cj - 11; j <= cj + 11; j++)
        for (int i = ci - 11; i <= ci + 11; i++) {
            uint32_t id = cell_hash(w->ox / 21 + i, w->oz / 21 + j, w->seed + 963);
            float x = (i + .12f + .76f * random2(1, 0, id)) * 21,
                  z = (j + .12f + .76f * random2(0, 1, id)) * 21;
            float dist = hypotf(x - cx, z - cz);
            if (dist < 220)
                scenery_cell(m, g, x, z, id, dist);
        }
    /* A characteristic formation frames the opening vista. All other sites stream by cell. */
    static const Place vista[] = {{0, 0},     {0, 0},     {-46, -71}, {-36, -82},
                                  {-51, -62}, {34, -68},  {-18, -32}, {-19, -43},
                                  {13, -38},  {-15, -28}, {-24, -53}, {16, -33}};
    if (w->profile == FUNGAL || w->profile == REEFS) {
        Place clearing = w->route.point[w->route.overlook];
        for (int j = 0; j < 7; j++) {
            float angle = 1.7f + j * .60f, span = 12 + (j % 3) * 7;
            float sx = clearing.x + sinf(angle) * span - w->ox,
                  sz = clearing.z - cosf(angle) * span - w->oz;
            if (hypotf(sx - cx, sz - cz) < 240 && !occupied(g, sx, sz) &&
                trail_distance(w, sx, sz) > 3.6f)
                prop(m, w, FEATURE, j, sx, height_at(w, sx, sz), sz, angle, .95f + .43f * (j % 3));
        }
    }
    Place p = vista[w->profile];
    float x = p.x - w->ox, z = p.z - w->oz;
    if (hypotf(cx - x, cz - z) < 310 && !occupied(g, x, z)) {
        float h = height_at(w, x, z);
        prop(m, w, FEATURE, 1, x, h - .1f, z, w->profile == COAST ? -.35f : .15f,
             w->profile == COAST ? 2.1f : 1.48f);
    }
}
void field_scenery(Mesh *m, const Game *g) {
    const World *w = &g->world;
    if (!w->place || !w->place->art)
        return;
    int progress = field_stage(g);
    for (int stage = 0; stage <= 2; stage++) {
        if (stage == 2 && progress < 2)
            continue;
        Place p = field_site(w, stage);
        float x = p.x - w->ox, z = p.z - w->oz;
        if (hypotf(g->x - x, g->z - z) > 145 || occupied(g, x, z))
            continue;
        Place next = field_site(w, stage == 2 ? 1 : stage + 1);
        float yaw = atan2f(next.x - p.x, p.z - next.z);
        if (stage == 2 || (stage == 1 && w->place->skill == DUSK)) {
            x += 1.9f * cosf(yaw);
            z += 1.9f * sinf(yaw);
        }
        float y = height_at(w, x, z);
        if (stage == 1) {
            if (w->place->skill == DUSK) {
                prop(m, w, NEEDLE, 0, x, y, z, yaw, 1);
                /* The upright casts the solar shadow. The engraved bearing remains readable at
                 * dusk. */
                Color mark = w->profile == FUNGAL ? w->place->colours[3] : rgb(0xc99e68);
                Vec left = {x - .05f * cosf(yaw), y + .966f, z - .05f * sinf(yaw)},
                    tip = {x + .76f * sinf(yaw), y + .966f, z - .76f * cosf(yaw)},
                    right = {x + .05f * cosf(yaw), y + .966f, z + .05f * sinf(yaw)};
                bool glowing = w->profile == FUNGAL && g->hour >= 16.5f && g->hour <= 19.5f;
                tri(m, left, tip, right, mark, mark, mark, glowing ? 6 : 0);
                continue;
            }
            bool echo = w->place->skill == ECHO;
            for (int j = 0; j < 3; j++) {
                if (!echo && j == 1)
                    continue;
                Place site = field_pipe(w, j);
                if (!echo) {
                    float offset = (j - 1) * 1.65f;
                    site = (Place){p.x + offset * sinf(yaw), p.z - offset * cosf(yaw)};
                }
                float sx = site.x - w->ox, sz = site.z - w->oz;
                prop(m, w, NEEDLE, j, sx, height_at(w, sx, sz), sz, yaw,
                     echo ? .58f + .35f * j : 1.05f);
            }
        } else if (stage == 2) {
            prop(m, w, RELIC, w->destination, x, y + .12f, z, (float)fmod(g->elapsed * .22, 2 * PI),
                 1.13f);
        } else
            prop(m, w, TRACK, 0, x, y + .03f, z, yaw, 1.25f);
    }
}
void realm_life(Mesh *m, const Game *g) {
    const World *w = &g->world;
    if (!w->place || !w->place->art || w->profile == LUNAR)
        return;
    int cx = (int)floorf(g->x / 84), cz = (int)floorf(g->z / 84);
    for (int j = cz - 1; j <= cz + 1; j++)
        for (int i = cx - 1; i <= cx + 1; i++) {
            uint32_t key = cell_hash(w->ox / 84 + i, w->oz / 84 + j, w->seed + 73);
            float t = (float)fmod(g->elapsed * .14 + random2(0, 1, key) * 2 * PI, 2 * PI);
            float x = (i + .5f) * 84 + 13 * cosf(t), z = (j + .5f) * 84 + 9 * sinf(t);
            float h = height_at(w, x, z);
            if (w->profile <= KARST) {
                if (g->hour < 6 || g->hour > 19 ||
                    (w->profile == DUNES &&
                     oasis_distance(w, w->ox + (double)x, w->oz + (double)z) > 80))
                    continue;
                for (int bird = 0; bird < 3; bird++)
                    prop(m, w, WANDERER, bird, x + bird * 2.5f, h + 10 + 2 * sinf(t + bird),
                         z + bird, t, .55f);
            } else {
                for (int k = 0; k < 5; k++) {
                    float dx = 5 * sinf(t + .9f * k), dy = 2 + 2 * sinf(2 * t + k);
                    Vec p = {x + dx, h + dy, z + k}, q = {p.x + .09f, p.y + .19f, p.z},
                        r = {p.x - .09f, p.y + .11f, p.z + .09f};
                    Color glow = w->place->colours[3];
                    tri(m, p, q, r, glow, glow, glow, 6);
                }
            }
        }
}
