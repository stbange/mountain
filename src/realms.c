#include "mountain.h"
#include <math.h>

/* A local landform has an address independent of the loaded hiking window. */
static float wave(double angle) { return (float)sin(angle); }
static float shore(const World *w, double z) {
    return -32 + 17 * wave(z / 87 + w->phase) + 9 * wave(z / 43) - 7 * wave(z / 210);
}
float oasis_distance(const World *w, double x, double z) {
    const double span = 640;
    int64_t i = (int64_t)floor(x / span), j = (int64_t)floor(z / span);
    double cx = (i + .25 + .5 * random2(i, j, w->seed + 194)) * span;
    double cz = (j + .25 + .5 * random2(i, j, w->seed + 206)) * span;
    double local = hypot((x - cx) * .86, (z - cz) * 1.12);
    return (float)fmin(local, hypot((x + 46) * .86, (z + 110) * 1.12));
}
static float knobs(const World *w, double x, double z, float spacing, bool craters) {
    int64_t i = (int64_t)floor(x / spacing), j = (int64_t)floor(z / spacing);
    float fx = (float)(x - i * (double)spacing), fz = (float)(z - j * (double)spacing), out = 0;
    for (int b = -1; b <= 1; b++)
        for (int a = -1; a <= 1; a++) {
            uint32_t id = cell_hash(i + a, j + b, w->seed + 19);
            float dx = fx - (a + .20f + .6f * random2(0, 1, id)) * spacing;
            float dz = fz - (b + .20f + .6f * random2(0, 2, id)) * spacing;
            float radius = (.22f + .27f * random2(1, 1, id)) * spacing;
            float d = hypotf(dx, dz) / radius;
            if (craters) {
                if (d < 1.32f)
                    out += radius * (.21f * (1 - smooth(0, .32f, fabsf(d - 1))) -
                                     .34f * (1 - smooth(.35f, 1, d)));
            } else {
                float height = spacing * (.52f + .94f * random2(2, 3, id));
                out = fmaxf(out, height * (1 - smooth(.13f, 1, d)));
            }
        }
    return out;
}
float realm_height(const World *w, double x, double z) {
    uint32_t s = w->seed;
    float broad = noise2(x / 179, z / 179, s + 11), grain = noise2(x / 28, z / 28, s + 77);
    float d = (float)x - shore(w, z);
    switch (w->profile) {
    case COAST: {
        float shelf = 3 + 19 * broad + 10 * grain;
        float headland = smooth(16, 65, d) * (18 + 38 * broad);
        float islands = knobs(w, x, z, 180, false) * smooth(48, 130, -d);
        return -9 + 13 * smooth(-28, 13, d) + shelf * smooth(3, 69, d) + headland * .54f +
               islands * .32f + 1.8f * grain;
    }
    case DUNES: {
        double angle =
            x / 103 + z / 261 + 2.0 * broad + .8 * noise2(x / 450, z / 410, s + 14) + w->phase;
        float fold = wave(angle) + .21f * wave(2 * angle - .85) + .07f * wave(3 * angle - 1.65);
        float dune = 29 + 21 * fold + 18 * broad + 2 * grain;
        float o = oasis_distance(w, x, z);
        return mixf(-2.4f + 5 * smooth(18, 40, o), dune, smooth(32, 102, o));
    }
    case BASALT:
        return -12 + 19 * smooth(-12, 14, d) + grain * 3 + smooth(29, 48, d) * (19 + 55 * broad) +
               knobs(w, x, z, 165, false) * smooth(32, 62, -d) * .54f;
    case MESA: {
        float n = noise2(x / 240, z / 180, s + 32) * .67f + .33f * broad;
        float mesa =
            49 * smooth(.37f, .405f, n) + 60 * smooth(.51f, .54f, n) + 43 * smooth(.64f, .67f, n);
        float river = (float)fabs(x + 95 + 56 * wave(z / 150 + w->phase));
        return -4 + smooth(5, 50, river) * (11 + mesa) + grain * 4;
    }
    case KARST: {
        double bank = x + 44 + 24 * wave(z / 115 + w->phase);
        float h = knobs(w, x + 12 * grain, z, 159, false);
        return -3 + smooth(12, 36, (float)fabs(bank)) * (7 + 14 * broad + h) + grain * 2;
    }
    case LUNAR:
        return 42 + 43 * broad + 4 * grain + knobs(w, x, z, 158, true) + knobs(w, x, z, 46, true) +
               48 * smooth(.53f, .89f, noise2(x / 310, z / 400, s + 85));
    case PRISM: {
        float ribs = fabsf(wave(x / 41 + z / 85 + 4 * broad));
        return 20 + 21 * broad + 13 * ribs + knobs(w, x, z, 193, false) * (.46f + .54f * ribs);
    }
    case FUNGAL:
        return 4 + 37 * broad + 10 * grain + 12 * wave(z / 83 + 3 * broad) +
               39 * smooth(.62f, .8f, grain);
    case REEFS:
        return -4 + 38 * broad + 16 * wave(x / 79 + z / 136) + 42 * smooth(.48f, .59f, grain);
    case CINDER: {
        float fissure = fabsf(wave(x / 67 + z / 115 + 3 * broad));
        float basalt = -12 + smooth(.04f, .28f, fissure) * (20 + 35 * broad) +
                       knobs(w, x, z, 210, true) * .86f + 14 * grain;
        float vent = (float)hypot((x - 31) * .81, z + 67);
        float rim = 12 * (1 - smooth(0, 9, fabsf(vent - 22)));
        return mixf(-5, basalt, smooth(14, 27, vent)) + rim;
    }
    default:
        return 0;
    }
}
void realm_route(World *w) {
    Route *r = &w->route;
    *r = (Route){.count = REALM_POINTS, .overlook = 7};
    float phase = w->phase;
    for (int i = 0; i < r->count; i++) {
        float z = -14.f * i;
        float x = 22 * (sinf(i * .30f + phase) - sinf(phase)) + 6 * sinf(i * .6f);
        if (w->profile == DUNES)
            x -= 26 * smooth(0, 7, (float)i) * (1 - smooth(8, 14, (float)i));
        if (w->profile == COAST || w->profile == BASALT)
            x = shore(w, z) - shore(w, 0) + 6 * sinf(i * .41f) - 12 * smooth(0, 6, (float)i);
        r->point[i] = (Place){x, z};
        float h = fmaxf(2.3f, realm_height(w, x, z));
        if (i) {
            float length = hypotf(x - r->point[i - 1].x, 14);
            r->length += length;
            h = clampf(h, w->approach[i - 1] - .26f * length, w->approach[i - 1] + .26f * length);
        }
        w->approach[i] = h;
    }
    r->sight[0] = (Place){-1, 0};
    r->sight[1] = (Place){0, -1};
}
float realm_ground(const World *w, double x, double z) {
    float native = realm_height(w, x, z);
    double station = -z / 14;
    if (station < -.9 || station > REALM_POINTS - .1 || fabs(x) > 115)
        return native;
    int i = (int)clampf((float)floor(station), 0, REALM_POINTS - 2);
    Place a = w->route.point[i], b = w->route.point[i + 1];
    float dx = b.x - a.x, dz = b.z - a.z;
    float t = clampf(((x - a.x) * dx + (z - a.z) * dz) / (dx * dx + dz * dz), 0, 1);
    float dist = hypotf((float)x - mixf(a.x, b.x, t), (float)z - mixf(a.z, b.z, t));
    float bench = mixf(w->approach[i], w->approach[i + 1], t);
    return mixf(bench, native, smooth(2.4f, 14 + fminf(34, fabsf(bench - native)), dist));
}
