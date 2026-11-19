#include "mountain.h"
#include <math.h>
#include <stdlib.h>

typedef struct {
    Vec p;
    float patch;
} Sample;
static Color terrain_colour(const Game *g, Sample v, float ny, float light) {
    float y = v.p.y;
    Color meadow = g->rules->meadow[g->season], forest = g->rules->forest[g->season];
    float cover = smooth(.24f, .8f, v.patch);
    Color col = blend(meadow, forest, cover * .76f);
    float exposed = clampf((1 - ny) * 1.8f + .40f * smooth(42, 115, y), 0, 1);
    Color rock = blend(rgb(0x536474), rgb(0xa89786), v.patch);
    col = blend(col, rock, exposed);
    float snowline = (g->season == 3 ? 10.f : (g->season == 0 ? 91.f : 119.f));
    float snow = smooth(snowline, snowline + 19, y + (ny - .6f) * 77 + (v.patch - .5f) * 44);
    Color ice = blend(rgb(0x92acc5), rgb(0xfff5e2), smooth(.63f, 1.05f, light));
    col = blend(col, ice, snow);
    if (y < water_at(&g->world, v.p.x, v.p.z) + 2.8f)
        col = blend(col, rgb(0xb1a785), .6f);
    return shade(col, light);
}
static void facet(Mesh *m, const Game *g, Sample a, Sample b, Sample c) {
    float ux = b.p.x - a.p.x, uy = b.p.y - a.p.y, uz = b.p.z - a.p.z;
    float vx = c.p.x - a.p.x, vy = c.p.y - a.p.y, vz = c.p.z - a.p.z;
    float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
    float inv = 1 / sqrtf(nx * nx + ny * ny + nz * nz);
    nx *= inv;
    ny *= inv;
    nz *= inv;
    float lit = clampf(.65f + .49f * (-.78f * nx + .51f * ny + .32f * nz), .48f, 1.14f);
    size_t first = m->count;
    tri(m, a.p, b.p, c.p, terrain_colour(g, a, ny, lit), terrain_colour(g, b, ny, lit),
        terrain_colour(g, c, ny, lit), 4);
    for (size_t i = first; i < m->count; i++) {
        m->v[i].u = nx;
        m->v[i].v = nz;
    }
}
/* Concentric grids keep triangles well proportioned on distant summits.
   Odd outer-edge heights lie on the adjoining coarse edge. */
#define HALF 36
#define SIDE (HALF * 2 + 1)
static Sample terrain_sample(const Game *g, float x, float z) {
    return (Sample){{x, ground(g, x, z), z},
                    noise2(x * .065f, z * .065f, g->world.seed + 7) * .50f +
                        noise2(x * .021f, z * .021f, g->world.seed + 79) * .50f};
}
static void ground_mesh(Mesh *m, const Game *g, float cx, float cz) {
    Sample rows[2][SIDE];
    for (int lod = 0; lod < 7; lod++) {
        float step = (float)(1 << lod);
        for (int j = -HALF; j <= HALF; j++) {
            Sample *row = rows[(j + HALF) % 2], *last = rows[(j + HALF + 1) % 2];
            for (int i = -HALF; i <= HALF; i++) {
                float x = cx + i * step, z = cz + j * step;
                bool edge = abs(i) == HALF || abs(j) == HALF;
                bool inner = lod && (abs(i) == HALF / 2 || abs(j) == HALF / 2);
                if (!edge && !inner) {
                    x += (random2((int)x, (int)z, g->world.seed) - .5f) * step * .35f;
                    z += (random2((int)x, (int)z, g->world.seed + 11) - .5f) * step * .35f;
                }
                int k = i + HALF;
                row[k] = terrain_sample(g, x, z);
                if (edge && ((abs(i) == HALF && j % 2) || (abs(j) == HALF && i % 2))) {
                    float dx = abs(j) == HALF ? step : 0, dz = abs(i) == HALF ? step : 0;
                    row[k].p.y = (ground(g, x - dx, z - dz) + ground(g, x + dx, z + dz)) * .5f;
                }
                if (i == -HALF || j == -HALF)
                    continue;
                if (lod && i > -HALF / 2 && i <= HALF / 2 && j > -HALF / 2 && j <= HALF / 2)
                    continue;
                Sample a = last[k - 1], b = row[k - 1], c = row[k], d = last[k];
                if ((i + j) % 2) {
                    facet(m, g, a, b, c);
                    facet(m, g, a, c, d);
                } else {
                    facet(m, g, a, b, d);
                    facet(m, g, b, c, d);
                }
            }
        }
    }
}
static bool cleared(const Game *g, float x, float z) {
    if (!g->cabin.present)
        return false;
    Vec p = cabin_local(&g->cabin, x, z);
    return fabsf(p.x) < 5.6f && p.z > -5.3f && p.z < 6.9f;
}
static void stone(Mesh *m, float x, float y, float z, float r, uint32_t seed) {
    Vec bottom[7], ring[7], top = {x + r * .04f, y + r * .75f, z - r * .19f};
    Color moss = rgb(0x8d9476), stone = rgb(0x8d8c81);
    for (int i = 0; i < 7; i++) {
        float a = 2 * PI * i / 7, rr = r * (.73f + .35f * random2(i, 8, seed));
        bottom[i] = (Vec){x + cosf(a) * rr, y - r * .26f, z + sinf(a) * rr * .78f};
        ring[i] = (Vec){x + cosf(a) * rr * .78f, y + r * (.27f + .25f * random2(i, 6, seed)),
                        z + sinf(a) * rr * .70f};
    }
    for (int i = 0; i < 7; i++) {
        int j = (i + 1) % 7;
        float light = .82f - .24f * cosf(i * 2 * PI / 7);
        quad(m, bottom[i], bottom[j], ring[j], ring[i], shade(stone, light), 0);
        Color c = shade(moss, light * 1.12f);
        tri(m, ring[i], ring[j], top, c, c, c, 0);
    }
}
static void trees(Mesh *m, const Game *g, float cx, float cz) {
    int cell_x = (int)floorf(cx / 7), cell_z = (int)floorf(cz / 7);
    for (int j = cell_z - 62; j <= cell_z + 62; j++)
        for (int i = cell_x - 62; i <= cell_x + 62; i++) {
            Tree tree;
            if (!world_tree(&g->world, i, j, &tree) || hypotf(tree.x - cx, tree.z - cz) > 425 ||
                cleared(g, tree.x, tree.z))
                continue;
            int tile = tree.tile;
            if (g->season == 2 && tile == 3)
                tile = 9;
            if (g->season == 3)
                tile = 10;
            Color tint = shade(rgb(0xf2f2dc), tree.tint);
            sprite(m, tree.x, tree.y - .27f, tree.z, tree.height * .72f, tree.height, tree.angle,
                   tile, tint);
            sprite(m, tree.x, tree.y - .27f, tree.z, tree.height * .72f, tree.height,
                   tree.angle + PI * .5f, tile, shade(tint, .97f));
        }
    /* Old firs at the trailhead frame the first view. */
    const float anchors[][3] = {
        {-8.6f, -10.5f, 13.2f}, {13.4f, -12.8f, 11.5f}, {-15.2f, -17.6f, 12.0f}};
    float bearing = trail_bearing(&g->world);
    for (int i = 0; i < 3; i++) {
        float ax = anchors[i][0] + g->world.phase, az = anchors[i][1], h = anchors[i][2];
        float x = cosf(bearing) * ax - sinf(bearing) * az,
              z = sinf(bearing) * ax + cosf(bearing) * az;
        if (g->world.survey || cleared(g, x, z) || route_distance(&g->world.route, x, z) < 3.5f ||
            hypotf(x - cx, z - cz) > 410)
            continue;
        float y = height_at(&g->world, x, z) - .12f;
        int tile = g->season == 3 ? 10 : i % 2;
        Color tint = rgb(0xe5e9da);
        sprite(m, x, y, z, h * .74f, h, .15f, tile, tint);
        sprite(m, x, y, z, h * .74f, h, 1.72f, tile, shade(tint, .92f));
    }
}
/* Flower islands and their pollinators share world-space anchors on every visit. */
static bool flower_patch(const Game *g, int i, int j, Vec *pos) {
    uint32_t seed = g->world.seed;
    float x = (i + .17f + .64f * random2(i, j, seed + 383)) * 12;
    float z = (j + .17f + .64f * random2(i, j, seed + 362)) * 12;
    if (i == 0 && j == -1) {
        float yaw = trail_bearing(&g->world);
        x = sinf(yaw) * 4.8f - cosf(yaw) * 3.1f;
        z = -cosf(yaw) * 4.8f - sinf(yaw) * 3.1f;
        if (route_distance(&g->world.route, x, z) < 2.45f) {
            x += cosf(yaw) * 6.2f;
            z += sinf(yaw) * 6.2f;
        }
    }
    float y = height_at(&g->world, x, z);
    if (y <= water_at(&g->world, x, z) + .13f || y > 112 || cleared(g, x, z) ||
        route_distance(&g->world.route, x, z) < 2.45f)
        return false;
    *pos = (Vec){x, y, z};
    return true;
}
static void flower_islands(Mesh *m, const Game *g, float cx, float cz) {
    if (g->season == 3)
        return;
    int ci = (int)floorf(cx / 12), cj = (int)floorf(cz / 12);
    for (int j = cj - 5; j <= cj + 5; j++)
        for (int i = ci - 5; i <= ci + 5; i++) {
            Vec p;
            if (!flower_patch(g, i, j, &p))
                continue;
            for (int n = 0; n < 5; n++) {
                float a = n * 2.41f, x = p.x + cosf(a) * n * .24f, z = p.z + sinf(a) * n * .24f;
                float y = height_at(&g->world, x, z);
                if (route_distance(&g->world.route, x, z) < 1.9f || y <= water_at(&g->world, x, z))
                    continue;
                int tile = g->rules->field_tiles[g->season][(i + j + n) & 1];
                float h = .64f + .27f * random2(i + n, j, g->world.seed);
                sprite(m, x, y - .045f, z, h * 1.35f, h, a, tile, rgb(0xfff8e3));
                sprite(m, x, y - .045f, z, h * 1.35f, h, a + PI * .5f, tile, rgb(0xf5f0da));
            }
        }
}
bool insects_active(const Game *g) {
    return g->season != 3 && g->weather < 2 && g->hour > 7 && g->hour < 18.75f;
}
static void pollinators(Mesh *m, const Game *g) {
    if (!insects_active(g))
        return;
    int ci = (int)floorf(g->x / 12), cj = (int)floorf(g->z / 12);
    for (int j = cj - 2; j <= cj + 2; j++)
        for (int i = ci - 2; i <= ci + 2; i++) {
            Vec p;
            if (!flower_patch(g, i, j, &p) || hypotf(p.x - g->x, p.z - g->z) > 21)
                continue;
            float phase = random2(i, j, g->world.seed + 12) * 2 * PI;
            bool bee = (i + j) % 3 == 0;
            if (g->season == 2 && !bee && phase > 2)
                continue;
            float t = g->elapsed * (bee ? 1.45f : 1.05f) + phase;
            float flight = smooth(-.78f, .12f, sinf(t * .47f));
            float x = p.x + flight * (1.45f * sinf(t) + .4f * sinf(2.4f * t));
            float z = p.z + flight * (.9f * cosf(1.6f * t) + .36f * cosf(t));
            float y = height_at(&g->world, x, z) + .42f + flight * (.56f + .38f * sinf(1.9f * t));
            if (cleared(g, x, z))
                continue;
            Pose body = {{x, y, z}, atan2f(cosf(t), sinf(t * 1.6f))};
            if (bee) {
                box(m, body, (Vec){0, 0, 0}, (Vec){.052f, .055f, .104f}, rgb(0xcaa640));
                box(m, body, (Vec){0, 0, .013f}, (Vec){.056f, .057f, .034f}, rgb(0x453d31));
                for (int side = -1; side <= 1; side += 2) {
                    Vec a = transform(body, (Vec){0, .016f, 0}),
                        b = transform(body,
                                      (Vec){side * .075f, .04f + .018f * sinf(t * 32), -.02f});
                    Vec c = transform(body, (Vec){side * .054f, .04f, .048f});
                    Color wing = rgb(0xd6d8c9);
                    tri(m, a, b, c, wing, wing, wing, 0);
                }
            } else {
                float beat = .62f + .59f * sinf(t * 18) * (.28f + .72f * flight);
                for (int side = -1; side <= 1; side += 2) {
                    float width = side * .145f * cosf(beat), lift = .145f * sinf(beat);
                    Vec a = transform(body, (Vec){0, 0, .067f}),
                        b = transform(body, (Vec){width, lift, .067f});
                    Vec c = transform(body, (Vec){width, lift, -.085f}),
                        d = transform(body, (Vec){0, 0, -.085f});
                    tex_quad(m, a, b, c, d, .751f, .751f, .999f, .999f, rgb(0xfff7e2), 1);
                }
                box(m, body, (Vec){0, 0, 0}, (Vec){.014f, .014f, .086f}, rgb(0x443a31));
            }
        }
}

static void meadow(Mesh *m, const Game *g, float cx, float cz) {
    uint32_t s = g->world.seed;
    int ox = (int)floorf(cx / 1.6f), oz = (int)floorf(cz / 1.6f);
    for (int j = oz - 45; j <= oz + 45; j++)
        for (int i = ox - 45; i <= ox + 45; i++) {
            float k = random2(i, j, s + 201);
            float x = (i + random2(i, j, s + 131)) * 1.6f, z = (j + random2(i, j, s + 198)) * 1.6f;
            float dist = hypotf(cx - x, cz - z);
            if (dist > 66 || k < .21f || cleared(g, x, z))
                continue;
            float h = height_at(&g->world, x, z);
            if (h < water_at(&g->world, x, z) + .12f || h > 128)
                continue;
            float path = route_distance(&g->world.route, x, z);
            if (k > .986f && path > 3.3f) {
                stone(m, x, h - .09f, z, .28f + 1.48f * random2(i, j, s), s + (unsigned)i);
                continue;
            }
            if (path < 1.9f || g->season == 3)
                continue;
            float cluster = noise2(x * .11f, z * .11f, s + 21);
            int tile = cluster > .58f ? (k > .65f ? g->rules->field_tiles[g->season][k > .87f] : 8)
                                      : (k > .73f ? 6 : 5);
            float tall = .44f + .71f * random2(i, j, s + 91);
            float a = random2(i, j, s + 74) * PI;
            Color tint = g->season == 2 ? rgb(0xe4cfa4) : rgb(0xf5f3df);
            sprite(m, x, h - .10f, z, tall * 1.40f, tall, a, tile, tint);
            if (dist < 23)
                sprite(m, x, h - .10f, z, tall * 1.40f, tall, a + PI * .5f, tile, tint);
        }
}
static void waymarks(Mesh *m, const Game *g) {
    const Route *r = &g->world.route;
    float next = 16, walked = 0;
    for (int i = 1; i < r->count; i++) {
        Place a = r->point[i - 1], b = r->point[i];
        float dx = b.x - a.x, dz = b.z - a.z, len = hypotf(dx, dz);
        while (next <= walked + len) {
            float t = (next - walked) / len;
            float x = mixf(a.x, b.x, t) - 2.65f * dz / len,
                  z = mixf(a.z, b.z, t) + 2.65f * dx / len;
            float y = ground(g, x, z);
            if (!cleared(g, x, z) && y > water_at(&g->world, x, z) &&
                route_distance(r, x, z) > 1.8f) {
                Pose p = {{x, y, z}, atan2f(dx, -dz)};
                box_material(m, p, (Vec){0, .58f, 0}, (Vec){.13f, 1.20f, .15f}, rgb(0x88714e), 3);
                box(m, p, (Vec){0, .99f, 0}, (Vec){.14f, .11f, .16f}, rgb(0xf6e4b9));
                box(m, p, (Vec){0, .89f, 0}, (Vec){.14f, .09f, .16f}, rgb(0xa95032));
            }
            next += 36;
        }
        walked += len;
        if (i == r->overlook || i == r->count - 1) {
            float x = b.x + 2.5f * dz / len, z = b.z - 2.5f * dx / len, y = ground(g, x, z);
            if (cleared(g, x, z) || y < water_at(&g->world, x, z))
                continue;
            stone(m, x, y, z, .72f, g->world.seed);
            stone(m, x + .05f, y + .43f, z, .44f, g->world.seed + 1);
            stone(m, x, y + .68f, z - .04f, .26f, g->world.seed + 2);
        }
    }
}

void landscape(Mesh *m, const Game *g, float cx, float cz) {
    mesh_clear(m);
    ground_mesh(m, g, cx, cz);
    float w = g->world.water, r = 2400;
    const Survey *s = g->world.survey;
    if (s) {
        for (int j = s->shore[2] - 1; j <= s->shore[3]; j++)
            for (int i = s->shore[0] - 1; i <= s->shore[1]; i++) {
                float x = (i - (int)(s->side / 2)) * s->cell;
                float z = (j - (int)(s->side / 2)) * s->cell, d = s->cell;
                if (survey_lake(s, x + d * .5f, z + d * .5f))
                    quad(m, (Vec){x, w, z}, (Vec){x + d, w, z}, (Vec){x + d, w, z + d},
                         (Vec){x, w, z + d}, rgb(0x568e92), 2);
            }
    } else
        quad(m, (Vec){-r, w, -r}, (Vec){r, w, -r}, (Vec){r, w, r}, (Vec){-r, w, r}, rgb(0x568e92),
             2);
    trees(m, g, cx, cz);
    meadow(m, g, cx, cz);
    flower_islands(m, g, cx, cz);
    waymarks(m, g);
}
static void foundations(Mesh *m, const Game *g, Pose pose) {
    const Cabin *c = &g->cabin;
    for (int i = -2; i <= 2; i++)
        for (int j = -1; j <= 1; j += 2) {
            float x = i * 2.05f, z = j * 3.82f;
            Vec point = cabin_world(c, x, 0, z);
            float depth = fmaxf(.45f, c->y - height_at(&g->world, point.x, point.z) + .38f);
            bool timber = depth > 1.15f || c->tree_supported;
            box_material(m, pose, (Vec){x, -depth * .5f, z}, (Vec){.44f, depth, .46f},
                         rgb(timber ? 0x72533a : 0x807c6d), timber ? 3 : 0);
            box(m, pose, (Vec){x, -depth + .18f, z}, (Vec){.64f, .38f, .66f}, rgb(0x7e7c6d));
        }
    if (c->tree_supported) {
        Tree t = c->anchor;
        Vec local = cabin_local(c, t.x, t.z);
        float foot = t.y - .28f, top = fmaxf(ground(g, t.x, t.z) + t.height, c->y + 6.5f);
        /* A tapered, faceted living trunk carries the outer end of the floor. */
        for (int i = 0; i < 9; i++) {
            float a = 2 * PI * i / 9, b = 2 * PI * (i + 1) / 9, rad = .32f + t.height * .009f;
            Vec p = {t.x + cosf(a) * rad, foot, t.z + sinf(a) * rad},
                q = {t.x + cosf(b) * rad, foot, t.z + sinf(b) * rad};
            Vec u = {t.x + cosf(b) * .13f, top, t.z + sinf(b) * .13f},
                v = {t.x + cosf(a) * .13f, top, t.z + sinf(a) * .13f};
            quad(m, p, q, u, v, shade(rgb(0x846447), .78f + .22f * sinf(a)), 3);
        }
        box_material(m, pose, (Vec){local.x * .56f, -.22f, local.z},
                     (Vec){fabsf(local.x) * .96f, .35f, .44f}, rgb(0x805a3c), 3);
        float crown = fmaxf(c->y + 3.92f, foot + t.height * .43f), h = top - crown;
        int tile = g->season == 3 ? 10 : (g->season == 2 && t.tile == 3 ? 9 : t.tile);
        sprite(m, t.x, crown, t.z, h * .88f, h, t.angle, tile, rgb(0xe4e7ca));
        sprite(m, t.x, crown, t.z, h * .88f, h, t.angle + PI * .5f, tile, rgb(0xdce0c8));
    }
}
void cabin_mesh(Mesh *m, Script *s, const Game *g) {
    mesh_clear(m);
    const Cabin *c = &g->cabin;
    if (!c->present)
        return;
    Pose p = {{c->x, c->y, c->z}, c->angle};
    if (!c->inside || !c->room)
        foundations(m, g, p);
    uint64_t index = c->inside ? c->room : 0;
    script_room(s, m, p, index, c->open, index == 0);
    if (c->inside || c->open) {
        p.origin = cabin_world(c, 0, 0, -8.16f);
        if (index < UINT64_MAX)
            script_room(s, m, p, index + 1, true, false);
        if (index > 0) {
            p.origin = cabin_world(c, 0, 0, 8.16f);
            script_room(s, m, p, index - 1, true, index == 1);
        }
    }
}
void wildlife(Mesh *m, const Game *g) {
    mesh_clear(m);
    pollinators(m, g);
    float t = g->elapsed;
    uint32_t s = g->world.seed;
    float flight_level = height_at(&g->world, 62, -108) + 38;
    /* Flocks wheel above the ridgeline; deer browse at the meadow edge. */
    for (int i = 0; i < 8; i++) {
        float a = t * .062f + i * .19f;
        float x = 44 + sinf(a) * 33 + i * 2.4f, z = -108 + cosf(a) * 16 + i * 1.7f,
              y = flight_level + sinf(a * .8f + i) * 3.6f;
        float flap = sinf(t * 3.6f + i * 1.93f) * .43f;
        Vec body = {x, y, z}, tail = {x + .06f, y - .09f, z + .32f},
            l = {x - .84f, y + flap, z + .14f}, r = {x + .84f, y + flap, z + .14f};
        Color ink = rgb(0x494f4c);
        tri(m, body, l, tail, ink, ink, ink, 0);
        tri(m, body, r, tail, ink, ink, ink, 0);
    }
    for (int i = 0; i < 7; i++) {
        float x = 13 + i * 14 + 3 * sinf(t * .038f + i), z = -13 + 11 * cosf(i * 3.2f + t * .022f);
        float h = height_at(&g->world, x, z);
        if (h > water_at(&g->world, x, z) && !cleared(g, x, z)) {
            float yaw = atan2f(g->z - z, g->x - x) - PI * .5f;
            sprite(m, x, h - .03f, z, 2.1f, 1.68f, yaw, 7, shade(rgb(0xffecd1), .86f));
        }
    }
    if (g->weather == 2) {
        Vec p = eye(g);
        Color rain = g->season == 3 ? rgb(0xe3e7e5) : rgb(0xb2c5c0);
        for (int i = 0; i < 110; i++) {
            float x = p.x + (random2(i, 1, s) - .5f) * 23, z = p.z + (random2(i, 2, s) - .5f) * 23;
            float speed = g->season == 3 ? 1.4f : 8.5f;
            float y = p.y + 8 - fmodf(t * speed + random2(i, 3, s) * 14, 14);
            if (g->cabin.inside && cleared(g, x, z))
                continue;
            float width = g->season == 3 ? .022f : .011f, length = g->season == 3 ? .05f : .29f;
            Vec a = {x, y, z}, b = {x + width, y, z}, c = {x + .035f, y - length, z};
            tri(m, a, b, c, rain, rain, rain, 0);
        }
    }
}
