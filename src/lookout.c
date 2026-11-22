#include "mountain.h"
#include <math.h>

/* Sites belong to the shelter, in metres relative to its rebased origin. A viewpoint or open
   casement never enters the patrol calculation. Water sightings need a real body of water. */
static bool suitable(const World *w, Habitat habitat, float x, float z) {
    if (habitat == IN_AIR)
        return true;
    float dry = height_at(w, x, z) - water_at(w, x, z);
    return habitat == ON_WATER ? dry < -.42f : dry > .16f;
}
static bool visible_from_shelter(const Game *g, Vec p, Habitat habitat, int slot) {
    float dx = p.x - g->cabin.x, dz = p.z - g->cabin.z, range = hypotf(dx, dz);
    float eye_y = g->cabin.y + 1.65f;
    float target = fmaxf(height_at(&g->world, p.x, p.z), water_at(&g->world, p.x, p.z)) + .45f;
    if (habitat == IN_AIR)
        target += slot ? 25 : 12;
    for (float d = 6; d < range - 2; d += 2.5f) {
        float t = d / range;
        if (height_at(&g->world, g->cabin.x + t * dx, g->cabin.z + t * dz) > mixf(eye_y, target, t))
            return false;
    }
    return true;
}
void lookout_sites(Game *g) {
    float extent = 0;
    for (int i = 0; i < g->rules->layouts; i++) {
        const Layout *l = &g->rules->layout[i];
        extent = fmaxf(extent, hypotf(l->width + (l->balcony ? 3.2f : 0), l->depth));
    }
    for (int slot = 0; slot < 2; slot++) {
        Habitat habitat = destination_shelter(g)->sight[slot].habitat;
        struct LookoutSite *site = &g->cabin.lookout[slot];
        struct LookoutSite nearest = {0};
        bool visible = false;
        for (int ring = 0; ring < 7 && !visible; ring++) {
            float radius = extent + (slot ? 51 : 19) + 10 * ring * ring;
            for (int j = 0; j < 32; j++) {
                float a = (slot ? -.54f : .46f) + ((j + 1) / 2) * (j % 2 ? 1 : -1) * (2 * PI / 32);
                Vec p = cabin_world(&g->cabin, radius * sinf(a), 0, -radius * cosf(a));
                if (!suitable(&g->world, habitat, p.x, p.z))
                    continue;
                float pad = habitat == ON_WATER ? 3.f : 2.f;
                if (!suitable(&g->world, habitat, p.x + pad, p.z) ||
                    !suitable(&g->world, habitat, p.x - pad, p.z))
                    continue;
                struct LookoutSite candidate = {p.x - g->cabin.x, p.z - g->cabin.z, true};
                if (!nearest.valid)
                    nearest = candidate;
                if (visible_from_shelter(g, p, habitat, slot)) {
                    *site = candidate;
                    visible = true;
                    break;
                }
            }
        }
        if (!visible)
            *site = nearest;
    }
}
static Vec site_position(const Game *g, int slot) {
    const struct LookoutSite *s = &g->cabin.lookout[slot];
    return (Vec){g->cabin.x + s->dx, g->cabin.y, g->cabin.z + s->dz};
}
static void sighting(Mesh *m, Script *s, const Game *g, int slot) {
    const Sighting *v = &destination_shelter(g)->sight[slot];
    if (!g->cabin.lookout[slot].valid)
        return;
    Vec home = site_position(g, slot);
    for (int i = 0; i < v->count; i++) {
        float t = (float)fmod(g->elapsed * (slot ? .083 : .19) + i * 1.65, 2 * PI);
        float dx = 6 * sinf(t), dz = 3 * sinf(2 * t) + i * .95f;
        Vec point = {home.x + dx, 0, home.z + dz};
        if (!suitable(&g->world, v->habitat, point.x, point.z))
            point = home;
        float floor = height_at(&g->world, point.x, point.z),
              water = water_at(&g->world, point.x, point.z);
        if (v->habitat == ON_WATER)
            point.y = water + .14f * sinf(t * 2);
        else if (v->habitat == IN_AIR)
            point.y = fmaxf(floor, water) + (slot ? 25 : 12) + 2 * sinf(t);
        else
            point.y = floor;
        Pose pose = {point, atan2f(6 * cosf(t), -6 * cosf(2 * t))};
        script_visitor(s, m, pose, v, (float)g->elapsed * (slot ? .79f : 2.7f) + 1.2f * i);
    }
}
static void fleck(Mesh *m, Vec p, float width, float length, float drift, float azimuth, Color c,
                  float material) {
    float dx = width * cosf(azimuth), dz = width * sinf(azimuth);
    Vec a = {p.x - dx, p.y, p.z - dz}, b = {p.x + dx, p.y, p.z + dz},
        tip = {p.x + drift, p.y + length, p.z};
    tri(m, a, b, tip, c, c, c, material);
}
static void airborne(Mesh *m, const Game *g, const LookoutEvent *event) {
    const Layout *l = cabin_layout(g);
    float inner = hypotf(l->width + 3.3f, l->depth) + 4;
    float lift = g->cabin.room ? 9.f + 2.f * (g->cabin.room % 13) : 1.f;
    EventKind k = event->kind;
    float speed = 3, wind = .24f, width = .065f, length = -.10f, material = 0;
    switch (k) {
    case FX_RAIN:
    case FX_LIGHTNING:
        speed = 10;
        width = .022f;
        length = -.51f;
        break;
    case FX_SNOW:
        speed = 2.3f;
        break;
    case FX_SPRAY:
        speed = 5;
        width = .056f;
        wind = .52f;
        length = .20f;
        break;
    case FX_SAND:
        speed = 6;
        width = .085f;
        wind = .72f;
        length = -.04f;
        break;
    case FX_SHARDS:
        speed = 6.8f;
        width = .09f;
        length = -.39f;
        break;
    case FX_EMBERS:
        speed = -3.4f;
        material = 6;
        length = .20f;
        break;
    case FX_UPDRAFT:
        speed = -6.2f;
        wind = .14f;
        length = .39f;
        break;
    case FX_SPORES:
    case FX_BLOOM:
        speed = -1.15f;
        width = k == FX_BLOOM ? .17f : .10f;
        material = 6;
        break;
    case FX_ASH:
        speed = 1.7f;
        width = .09f;
        wind = .38f;
        break;
    default:
        break;
    }
    uint32_t seed = g->world.seed;
    for (int i = 0; i < 800; i++) {
        float a = random2(i, 1, seed) * 2 * PI, r = inner + random2(i, 2, seed) * 21;
        float y = (float)(g->elapsed * speed / 22 + random2(i, 3, seed));
        y = (y - floorf(y)) * 22;
        float sway = sinf(y * .61f + a * 3);
        float offset = wind * (11 - y);
        Vec p =
            cabin_world(&g->cabin, r * cosf(a) + offset, lift + 15 - y, r * sinf(a) + sway * .55f);
        Color c = shade(event->tint, .78f + .22f * random2(i, 4, seed));
        fleck(m, p, width, length, wind * .4f, a + g->cabin.angle + PI / 2, c, material);
    }
}
static void vortices(Mesh *m, const Game *g, Color tint) {
    Vec site = site_position(g, 0);
    for (int n = 0; n < 3; n++) {
        float x = site.x + 9 * n, z = site.z + 4 * sinf(n * 2.1f);
        float floor = height_at(&g->world, x, z);
        for (int j = 0; j < 84; j++) {
            float u = (j + .5f) / 84, a = u * 23 + (float)g->elapsed * 1.6f + n * 2.1f;
            float r = .35f + u * (1.3f + n * .47f);
            Vec p = {x + cosf(a) * r + u * n, floor + u * (6 + 2 * n), z + sinf(a) * r};
            fleck(m, p, .20f + .22f * u, .32f, -sinf(a) * .50f, a + PI / 2, tint, 0);
        }
    }
}
static void lightning(Mesh *m, const Game *g, Color tint) {
    float time = (float)fmod(g->elapsed, 19);
    if (time < 2.70f || time > 3.05f)
        return;
    /* The same slow pulse lights the sky shader. The discharge stays on the far ridge. */
    int strike = (int)fmod(g->elapsed / 19, 65536);
    Vec foot = cabin_world(&g->cabin, 28 + random2(strike, 0, g->world.seed) * 42, 0, -125);
    foot.y = height_at(&g->world, foot.x, foot.z);
    Vec a = foot;
    for (int i = 1; i <= 8; i++) {
        Vec b = {foot.x + i * .65f + (random2(strike, i, g->world.seed) - .5f) * 6, foot.y + i * 5,
                 foot.z + i * .75f};
        Vec left = {a.x - .22f, a.y, a.z}, right = {b.x - .30f, b.y, b.z};
        quad(m, a, b, right, left, tint, 6);
        if (i == 4) {
            Vec end = {b.x + 7, b.y - 8, b.z - 1};
            tri(m, b, right, end, tint, tint, tint, 6);
        }
        a = b;
    }
}
/* Lunar ejecta follows low-gravity ballistic arcs above local ground. The sky stays airless. */
static void ejecta(Mesh *m, const Game *g, const LookoutEvent *event) {
    Vec site = site_position(g, 0);
    for (int fan = 0; fan < 4; fan++) {
        float x = site.x + fan * 7.8f, z = site.z + sinf(fan * 2.1f) * 9;
        float ground = height_at(&g->world, x, z);
        for (int i = 0; i < 75; i++) {
            float a = 2 * PI * random2(i, fan, g->world.seed),
                  v = random2(i, fan + 9, g->world.seed);
            float t = (float)fmod(g->elapsed * (event->kind == FX_DUSTHOP ? .46 : .29) + fan * .26 +
                                      (event->kind == FX_IMPACTS ? v * .10 : v),
                                  1);
            float reach = event->kind == FX_DUSTHOP ? 3 : 7;
            float height = event->kind == FX_DUSTHOP ? 1.2f : event->kind == FX_EJECTA ? 8 : 4.8f;
            float dx = cosf(a) * (1 + v) * reach * t, dz = sinf(a) * (1 + v) * reach * t;
            if (event->kind == FX_EJECTA)
                dx = dx * .36f + t * 5;
            Vec p = {x + dx, ground + .12f + 4 * t * (1 - t) * height * (.65f + .35f * v), z + dz};
            fleck(m, p, .045f + v * .10f, .09f + v * .14f, 0, a, event->tint, 0);
        }
    }
}
static void swells(Mesh *m, const Game *g, const LookoutEvent *event) {
    bool air = event->kind == FX_SKYTIDE;
    int slot = destination_shelter(g)->sight[0].habitat == ON_WATER || air ? 0 : 1;
    if (!g->cabin.lookout[slot].valid)
        return;
    Vec site = site_position(g, slot);
    for (int row = 0; row < 4; row++) {
        float phase = (float)fmod(g->elapsed * .14 + row * .25, 1), z = site.z + phase * 18 - 9;
        float amp = (air ? 3.3f : 1.7f) * sinf(PI * phase);
        for (int i = -12; i < 12; i++) {
            float x = site.x + i * 1.3f;
            if (!air && (!suitable(&g->world, ON_WATER, x, z) ||
                         !suitable(&g->world, ON_WATER, x + 1.3f, z + .5f)))
                continue;
            float h =
                air ? fmaxf(site.y, height_at(&g->world, x, z)) + 17 : water_at(&g->world, x, z);
            float y = h + amp * (.76f + .24f * cosf(i * .23f));
            Vec a = {x, h + .10f, z - 1.3f}, b = {x + 1.31f, h + .10f, z - 1.3f};
            Vec c = {b.x, y + .12f, z + .36f}, d = {x, y + .12f, z + .36f};
            quad(m, a, b, c, d, shade(event->tint, .58f), 0);
            quad(m, d, c, (Vec){b.x, y + .04f, z + .65f}, (Vec){x, y + .04f, z + .65f}, event->tint,
                 0);
        }
    }
}
void lookout_scenery(Mesh *m, Script *s, const Game *g) {
    if (!g->cabin.inside)
        return;
    if (g->book.predators)
        sighting(m, s, g, 0);
    if (g->book.giants)
        sighting(m, s, g, 1);
    const LookoutEvent *e = lookout_event(g);
    switch (e->kind) {
    case FX_CALM:
    case FX_MIST:
    case FX_HEAT:
        return;
    case FX_DRYLIGHT:
    case FX_LIGHTNING:
        lightning(m, g, e->tint);
        if (e->kind == FX_LIGHTNING)
            airborne(m, g, e);
        break;
    case FX_SURF:
    case FX_SKYTIDE:
        swells(m, g, e);
        break;
    case FX_VORTEX:
        vortices(m, g, e->tint);
        break;
    case FX_IMPACTS:
    case FX_EJECTA:
    case FX_DUSTHOP:
        ejecta(m, g, e);
        break;
    default:
        airborne(m, g, e);
        break;
    }
}
