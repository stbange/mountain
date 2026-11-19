#include "mountain.h"
#include <math.h>

float clampf(float v, float lo, float hi) {
    return fmaxf(lo, fminf(hi, v));
}
float mixf(float a, float b, float t) {
    return a + (b - a) * t;
}
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
float random2(int x, int z, uint32_t seed) {
    return (hash32((uint32_t)x * 0x9e3779b9u ^ (uint32_t)z * 0x85ebca6bu ^ seed) >> 8) *
           (1.0f / 16777216.0f);
}
float noise2(float x, float z, uint32_t seed) {
    int ix = (int)floorf(x), iz = (int)floorf(z);
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
Color shade(Color c, float l) {
    return (Color){c.r * l, c.g * l, c.b * l};
}

void world_init(World *w, uint32_t seed, const Rules *r) {
    w->seed = seed;
    w->relief = r->relief;
    w->water = r->water;
    w->phase = (random2(4, 8, seed) - .5f) * 1.6f;
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
}

float trail_x(const World *w, float z) {
    return 2.8f + sinf(z * .067f + w->phase) * 2.4f + sinf(z * .022f + w->phase) * 7.f;
}
float height_at(const World *w, float x, float z) {
    uint32_t s = w->seed;
    float broad = noise2(x * .011f, z * .011f, s), detail = noise2(x * .050f, z * .050f, s + 29);
    float base = -13 + 8 * broad + 2.5f * detail;
    /* A meadow spur makes an overlook above the glacial lake. */
    float sx = (x + 24 + sinf(z * .013f) * 18) / 77, sz = (z - 24) / 74;
    base += 48 * expf(-.85f * (sx * sx + sz * sz));
    float valley = x - 19 * sinf(z * .013f + w->phase);
    base += 17 * smooth(38, 127, fabsf(valley)) + 18 * smooth(8, 155, z);
    float high = 0;
    float warp_x = (noise2(x * .013f, z * .013f, s + 8) - .5f) * 33;
    float warp_z = (noise2(x * .016f, z * .016f, s + 18) - .5f) * 27;
    for (int i = 0; i < RIDGES; i++) {
        const Peak *p = &w->peaks[i];
        float dx = (x + warp_x - p->x) / p->rx, dz = (z + warp_z - p->z) / p->rz;
        if (fabsf(dx) > 1.36f || fabsf(dz) > 1.36f)
            continue;
        float d = sqrtf(dx * dx + dz * dz);
        float flank = 1 - d * (1 + .105f * sinf(atan2f(dz, dx) * 3 + p->angle));
        high = fmaxf(high, p->h * fmaxf(0, flank));
    }
    float rugged = smooth(8, 100, high);
    high += rugged * ((broad - .5f) * 43 + (detail - .5f) * 16 +
                      (noise2(x * .13f, z * .13f, s + 9) - .5f) * 5);
    if (fabsf(x) > 800 || fabsf(z) > 800)
        high += smooth(800, 1050, fmaxf(fabsf(x), fabsf(z))) * (38 + 120 * broad);
    return base + high;
}
float sunlight_factor(float hour) {
    return smooth(-.16f, .33f, sinf((hour - 6) * PI / 12));
}
float alpenglow(float hour) {
    return 1 - smooth(.03f, .71f, fabsf(sinf((hour - 6) * PI / 12)));
}

bool world_tree(const World *w, int i, int j, Tree *tree) {
    uint32_t s = w->seed;
    float x = (i + random2(i, j, s + 1)) * 7, z = (j + random2(i, j, s + 2)) * 7;
    float h = height_at(w, x, z), patch = noise2(x * .032f, z * .032f, s + 61);
    if (h < w->water + 1.7f || h > 112 + 17 * patch)
        return false;
    float gap = smooth(6, 28, fabsf(x - trail_x(w, z)));
    float meadow = smooth(8, 44, hypotf(x, z));
    if (random2(i, j, s + 83) > (.20f + patch * .86f) * gap * (.32f + meadow * .68f))
        return false;
    float species = random2(i, j, s + 145);
    *tree = (Tree){x,
                   h,
                   z,
                   (6 + 11 * random2(i, j, s + 5)) * (1 - .32f * smooth(74, 124, h)),
                   random2(i, j, s + 7) * PI,
                   .78f + .24f * patch,
                   species < .14f ? 3 : (species < .23f ? 2 : (i % 3 == 0 ? 1 : 0))};
    return true;
}
