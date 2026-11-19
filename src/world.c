#include "mountain.h"
#include <math.h>

float clampf(float v, float lo, float hi) { return fmaxf(lo, fminf(hi, v)); }
float mixf(float a, float b, float t) { return a + (b - a) * t; }
float smooth(float a, float b, float x) {
    float t = clampf((x - a) / (b - a), 0, 1);
    return t * t * (3 - 2 * t);
}
uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    return x ^ (x >> 16);
}
uint32_t cell_hash(int64_t x, int64_t z, uint32_t seed) {
    uint64_t a = (uint64_t)x, b = (uint64_t)z;
    uint32_t hx = (uint32_t)(a >> 32) ^ (x < 0 ? UINT32_MAX : 0);
    uint32_t hz = (uint32_t)(b >> 32) ^ (z < 0 ? UINT32_MAX : 0);
    return hash32((uint32_t)a * 0x9e3779b9u ^ (uint32_t)b * 0x85ebca6bu ^
                  hash32(hx * 0x9e3779b9u ^ hz * 0x85ebca6bu) ^ seed);
}
float random2(int64_t x, int64_t z, uint32_t seed) {
    return (cell_hash(x, z, seed) >> 8) * (1.0f / 16777216.0f);
}
float noise2(double x, double z, uint32_t seed) {
    int64_t ix = (int64_t)floor(x), iz = (int64_t)floor(z);
    float fx = x - ix, fz = z - iz;
    fx = fx * fx * (3 - 2 * fx);
    fz = fz * fz * (3 - 2 * fz);
    return mixf(mixf(random2(ix, iz, seed), random2(ix + 1, iz, seed), fx),
                mixf(random2(ix, iz + 1, seed), random2(ix + 1, iz + 1, seed), fx), fz);
}
Color rgb(unsigned n) {
    return (Color){((n >> 16) & 255) / 255.f, ((n >> 8) & 255) / 255.f, (n & 255) / 255.f};
}
Color blend(Color a, Color b, float t) {
    return (Color){mixf(a.r, b.r, t), mixf(a.g, b.g, t), mixf(a.b, b.b, t)};
}
Color shade(Color c, float l) { return (Color){c.r * l, c.g * l, c.b * l}; }

void world_init(World *w, uint32_t seed, const Rules *r) {
    w->ox = w->oz = 0;
    w->trail = (LongTrail){0};
    w->seed = seed;
    w->relief = r->relief;
    w->survey = r->survey.data && seed == r->survey.seed ? &r->survey : NULL;
    w->water = w->survey ? 0 : r->water;
    w->phase = (random2(4, 8, seed) - .5f) * 1.6f;
    w->meadow_level = -13 + 8 * random2(0, 0, seed) + 2.5f * random2(0, 0, seed + 29) +
                      48 * expf(-.85f * (24.f * 24 / (77 * 77) + 24.f * 24 / (74 * 74)));
    for (int i = 0; i < RIDGES; i++) {
        float a = random2(i, 1, seed), b = random2(i, 2, seed), c = random2(i, 3, seed);
        Peak p = {0};
        if (i < 10) {
            p.x = (i - 4.5f) * 105 + (a - .5f) * 78;
            p.z = -355 - b * 190;
            p.h = 125 + c * 98;
            p.rx = 140 + a * 110;
            p.rz = 155 + b * 100;
            if (i == 5) {
                p.h += 67;
                p.rz += 32;
            }
        } else if (i < 20) {
            float side = i % 2 ? 1.f : -1.f;
            p.x = side * (215 + a * 155);
            p.z = -275 + (i - 10) * 62 + (b - .5f) * 45;
            p.h = 85 + c * 107;
            p.rx = 122 + b * 90;
            p.rz = 144 + c * 90;
        } else {
            float angle = (i - 20) * 2 * PI / 16 + a * .16f;
            p.x = sinf(angle) * (680 + 110 * b);
            p.z = cosf(angle) * (650 + 110 * c);
            p.h = 180 + a * 164;
            p.rx = 205 + b * 125;
            p.rz = 224 + c * 125;
        }
        p.h *= w->relief;
        p.angle = a * 4.7f;
        w->peaks[i] = p;
    }
    route_generate(w);
    Place end = w->route.point[w->route.count - 1];
    if (fabsf(end.x) >= fabsf(end.z))
        w->trail.dx = end.x < 0 ? -1 : 1;
    else
        w->trail.dz = end.z < 0 ? -1 : 1;
    w->trail.start_y = raw_height(w, end.x, end.z);
    w->trail.regional_origin = noise2(0, 4, seed) * 880 + noise2(0, 5, seed) * 2600;
    w->trail.first = -TRAIL_WINDOW - 1;
    w->trail.first_shelf = -2 * TRAIL_SHELVES;
    trail_window(w, 0, 0);
}

float starter_height(const World *w, float x, float z) {
    if (w->survey)
        return survey_height(w->survey, x, z);
    uint32_t s = w->seed;
    float broad = noise2(x * .011f, z * .011f, s), detail = noise2(x * .050f, z * .050f, s + 29);
    float base = -13 + 8 * broad + 2.5f * detail;
    /* A meadow spur makes an overlook above the glacial lake. */
    float sx = (x + 24 + sinf(z * .013f) * 18) / 77, sz = (z - 24) / 74;
    base += 48 * expf(-.85f * (sx * sx + sz * sz));
    float valley = x - 19 * sinf(z * .013f + w->phase);
    base += 17 * smooth(38, 127, fabsf(valley)) + 18 * smooth(8, 155, z);
    float opening = x * x + z * z;
    if (opening < 38 * 38) {
        float bench = w->meadow_level + .055f * z - .019f * x;
        base = mixf(bench, base, smooth(10, 38, sqrtf(opening)));
    }
    float high = 0;
    float warp_x = (noise2(x * .013f, z * .013f, s + 8) - .5f) * 33;
    float warp_z = (noise2(x * .016f, z * .016f, s + 18) - .5f) * 27;
    for (int i = 0; i < RIDGES; i++) {
        const Peak *p = &w->peaks[i];
        float dx = (x + warp_x - p->x) / p->rx, dz = (z + warp_z - p->z) / p->rz;
        if (fabsf(dx) > 1.36f || fabsf(dz) > 1.36f)
            continue;
        float d = sqrtf(dx * dx + dz * dz);
        float aspect = atan2f(dz, dx) + p->angle;
        float ribs = fabsf(sinf(3 * aspect + .65f * d)) + .43f * fabsf(sinf(5 * aspect - 1.4f * d));
        float flank = 1 - d * (.78f + .34f * ribs);
        if (i < 10)
            flank += .095f * (1 - smooth(.03f, .70f, d)) * cosf(2 * aspect) * d;
        high = fmaxf(high, p->h * fmaxf(0, flank));
    }
    float rugged = smooth(8, 100, high);
    high += rugged * ((broad - .5f) * 43 + (detail - .5f) * 16 +
                      (noise2(x * .13f, z * .13f, s + 9) - .5f) * 5);
    if (fabsf(x) > 800 || fabsf(z) > 800)
        high += smooth(800, 1050, fmaxf(fabsf(x), fabsf(z))) * (38 + 120 * broad);
    return base + high;
}
float sunlight_factor(float hour) { return smooth(-.16f, .33f, sinf((hour - 6) * PI / 12)); }
float alpenglow(float hour) { return 1 - smooth(.03f, .71f, fabsf(sinf((hour - 6) * PI / 12))); }

bool world_tree(const World *w, int i, int j, Tree *tree) {
    uint32_t s = w->seed;
    int64_t ix = w->ox / 7 + i, iz = w->oz / 7 + j;
    float x = (i + random2(ix, iz, s + 1)) * 7, z = (j + random2(ix, iz, s + 2)) * 7;
    float h = height_at(w, x, z),
          patch = noise2((w->ox + (double)x) * .032, (w->oz + (double)z) * .032, s + 61);
    if (h < water_at(w, x, z) + 1.7f || h > 112 + 17 * patch)
        return false;
    float gap = smooth(3.15f, 13.f, trail_distance(w, x, z));
    float meadow = smooth(8, 44, (float)hypot(w->ox + (double)x, w->oz + (double)z));
    if (random2(ix, iz, s + 83) > (.20f + patch * .86f) * gap * (.32f + meadow * .68f))
        return false;
    const Route *route = &w->route;
    for (int stop = 0; stop < 2; stop++) {
        Place p = route->point[stop ? route->count - 1 : route->overlook],
              sight = route->sight[stop];
        float dx = (float)(w->ox + (double)x - p.x), dz = (float)(w->oz + (double)z - p.z),
              along = dx * sight.x + dz * sight.z;
        if (dx * dx + dz * dz < 13 * 13 ||
            (along > 0 && along < 62 && fabsf(dx * sight.z - dz * sight.x) < 4 + .28f * along))
            return false;
    }
    if (w->survey) {
        /* The eastern shore opens onto a glacial lake and its mountain skyline. */
        double ax = w->ox + (double)x, az = w->oz + (double)z;
        double right = ax * cosf(w->survey->bearing) + az * sinf(w->survey->bearing);
        double ahead = ax * sinf(w->survey->bearing) - az * cosf(w->survey->bearing);
        if (ahead > 0 && ahead < 62 && fabs(right) < 3.5f + .15f * ahead)
            return false;
    }
    float species = random2(ix, iz, s + 145);
    *tree = (Tree){x,
                   h,
                   z,
                   (6 + 11 * random2(ix, iz, s + 5)) * (w->survey ? .68f : 1.f) *
                       (1 - .32f * smooth(74, 124, h)),
                   random2(ix, iz, s + 7) * PI,
                   .78f + .24f * patch,
                   species < .14f ? 3 : (species < .23f ? 2 : (ix % 3 == 0 ? 1 : 0)),
                   cell_hash(ix, iz, s + 818)};
    return true;
}
