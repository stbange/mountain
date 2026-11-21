#include "mountain.h"
#include <math.h>

/* Broad uplands alternate with lower valleys along the mountain chain. Even the
   tallest 400-metre uplift changes by less than 25 metres per 144-metre station. */
float alpine_pass_height(const World *w, double along) {
    if (w->profile != ALPINE)
        return 0;
    double distance = fabs(along) - 1050;
    if (distance <= 0)
        return 0;
    const double span = 7800;
    int64_t region = (int64_t)floor(distance / span);
    float phase = (float)((distance - region * span) / span);
    float height = 265 + 135 * random2(region, along < 0 ? -11 : 11, w->seed + 127);
    return height * smooth(0, .48f, phase) * (1 - smooth(.53f, 1, phase));
}
/* Coordinates of the starter mountain are fixed. All distant ridges are
   addressed by their own lattice cell, including on the negative axes. */
static float ranges(const World *w, double x, double z) {
    uint32_t seed = w->seed;
    double u = x + (noise2(x / 430, z / 430, seed + 71) - .5) * 118;
    double v = z + (noise2(x / 360, z / 360, seed + 121) - .5) * 130;
    int64_t i = (int64_t)floor(u / 385), j = (int64_t)floor(v / 385);
    float fx = (float)(u - i * 385.), fz = (float)(v - j * 385.), high = 0;
    for (int b = -1; b <= 1; b++)
        for (int a = -1; a <= 1; a++) {
            uint32_t key = cell_hash(i + a, j + b, seed + 723);
            float dx = fx - (a + .18f + .64f * random2(0, 0, key)) * 385;
            float dz = fz - (b + .18f + .64f * random2(1, 0, key)) * 385;
            float width = 200 + 155 * random2(2, 0, key);
            float d = hypotf(dx, dz) / width;
            if (d > 1.30f)
                continue;
            float angle = atan2f(dz, dx) + 6 * random2(3, 0, key);
            float buttress =
                .78f + .38f * fabsf(sinf(3 * angle + .8f * d)) + .10f * fabsf(sinf(5 * angle));
            float summit = (125 + 330 * random2(4, 0, key)) * w->relief;
            high = fmaxf(high, summit * (1 - d * buttress));
        }
    float broad = noise2(x / 300, z / 300, seed + 35), detail = noise2(x / 37, z / 37, seed + 85);
    float uplift = 0;
    if (w->profile == ALPINE && (w->trail.dx || w->trail.dz)) {
        Place fork = w->route.point[w->route.count - 1];
        uplift = alpine_pass_height(w, (x - fork.x) * w->trail.dx + (z - fork.z) * w->trail.dz);
    }
    return uplift + w->water - 26 + 67 * broad + 9 * detail + high +
           smooth(12, 125, high) *
               (37 * (detail - .5f) + 12 * (noise2(x / 11, z / 11, seed + 311) - .5f));
}
float raw_height(const World *w, double x, double z) {
    if (w->profile > SURVEYED)
        return realm_ground(w, x, z);
    double distance = fmax(fabs(x), fabs(z));
    float edge = w->survey ? w->survey->cell * (w->survey->side / 2) : 960.f;
    float inner = edge - (w->survey ? fminf(256, edge * .15f) : 300);
    if (distance <= inner)
        return starter_height(w, (float)x, (float)z);
    float mountain = ranges(w, x, z);
    if (distance >= edge)
        return mountain;
    return mixf(starter_height(w, (float)x, (float)z), mountain,
                smooth(inner, edge, (float)distance));
}

static Place finish(const World *w) { return w->route.point[w->route.count - 1]; }
double trail_station(const World *w, float x, float z) {
    Place end = finish(w);
    return (w->ox + (double)x - end.x) * w->trail.dx + (w->oz + (double)z - end.z) * w->trail.dz;
}
static float base_bend(const World *w, int64_t index) {
    if (index <= 0)
        return 0;
    double t = (double)index;
    uint32_t s = w->seed;
    float offset = (noise2(t / 39, 1, s) - .5f) * 338 + (noise2(t / 11, 2, s) - .5f) * 126 +
                   (noise2(t / 3.7, 3, s) - .5f) * 32;
    /* Long valleys can turn the trail sideways for kilometres while its station
       number still gives any section a stable address. */
    float region =
        noise2(t / 128, 4, s) * 880 + noise2(t / 530, 5, s) * 2600 - w->trail.regional_origin;
    return region + offset * smooth(0, 8, (float)index);
}
static void coordinates(const World *w, int64_t i, float side, double *x, double *z) {
    Place p = finish(w);
    double along = (double)i * TRAIL_STEP;
    *x = p.x + along * w->trail.dx - side * w->trail.dz;
    *z = p.z + along * w->trail.dz + side * w->trail.dx;
}
/* Look for a dry shoulder with an open side every 288 metres. Compare the
   natural ground before grading the tread so the result is independent of
   which part of the route has already been generated. */
enum { SHELF_STEPS = 12 };
static float choose_shelf(const World *w, int64_t sector) {
    if (sector <= 0)
        return 0;
    int64_t i = sector * SHELF_STEPS;
    float baseline = base_bend(w, i), best = -1e9f, offset = 0;
    for (int option = -2; option <= 2; option++) {
        float shift = option * 36.f;
        double x, z;
        coordinates(w, i, baseline + shift, &x, &z);
        float h = raw_height(w, x, z);
        float across = raw_height(w, x - 22 * w->trail.dz, z + 22 * w->trail.dx);
        float ahead = raw_height(w, x + 22 * w->trail.dx, z + 22 * w->trail.dz);
        float left = raw_height(w, x - 95 * w->trail.dz, z + 95 * w->trail.dx);
        float right = raw_height(w, x + 95 * w->trail.dz, z - 95 * w->trail.dx);
        float outlook = clampf(h - fminf(left, right), 0, 48);
        float score =
            .32f * outlook - fabsf(h - ahead) - .8f * fabsf(h - across) - .053f * fabsf(shift);
        if (h < w->water + 2)
            score -= 60;
        if (score > best) {
            best = score;
            offset = shift;
        }
    }
    return offset;
}
static float shelf(const World *w, int64_t sector) {
    int64_t j = sector - w->trail.first_shelf;
    return j >= 0 && j < TRAIL_SHELVES ? w->trail.shelves[j] : choose_shelf(w, sector);
}
static float bend(const World *w, int64_t index) {
    int64_t sector = index / SHELF_STEPS;
    float t = smooth(0, SHELF_STEPS, (float)(index % SHELF_STEPS));
    return base_bend(w, index) + mixf(shelf(w, sector), shelf(w, sector + 1), t);
}
/* Sample the terrain at spaced survey stations. A rolling minimum envelope
   limits the grade over passes. Shared stations make each section independent
   of travel direction and of the order in which sections are requested. */
enum { PROFILE_STRIDE = 6, PROFILE_REACH = 6 };
static float station_height(const World *w, int64_t coarse) {
    if (coarse <= 0)
        return w->trail.start_y;
    /* An anchor beyond the sampled horizon cannot lower this cap. This keeps
       the grade bound valid when a low valley leaves the sampling window. */
    float height = fminf(w->trail.start_y, w->water + 3) + PROFILE_REACH * 28;
    if (w->profile == ALPINE) {
        double station = (double)coarse * PROFILE_STRIDE * TRAIL_STEP;
        double reach = PROFILE_REACH * PROFILE_STRIDE * TRAIL_STEP;
        /* Even the lowest raw ground is above water + uplift - 52. This
           lower bound on both window edges keeps adjacent grades continuous. */
        float low =
            fminf(alpine_pass_height(w, station - reach), alpine_pass_height(w, station + reach));
        height += fmaxf(0, low - 55);
    }
    for (int j = -PROFILE_REACH; j <= PROFILE_REACH; j++) {
        int64_t k = coarse + j;
        if (k < 0)
            continue;
        double x, z;
        coordinates(w, k * PROFILE_STRIDE, bend(w, k * PROFILE_STRIDE), &x, &z);
        float h = k ? fmaxf(w->water + 3, raw_height(w, x, z)) : w->trail.start_y;
        height = fminf(height, h + fabsf((float)j) * 28);
    }
    /* The join must also be reachable from a high starting belvedere. */
    return fmaxf(w->trail.start_y - (float)coarse * 28, height);
}
static float side_at(const World *w, int64_t i) {
    int64_t j = i - w->trail.first;
    return j >= 0 && j < TRAIL_WINDOW ? w->trail.knots[j].side : bend(w, i);
}
static float profile_at(const World *w, int64_t i) {
    int64_t j = i - w->trail.first;
    if (j >= 0 && j < TRAIL_WINDOW)
        return w->trail.knots[j].height;
    int64_t k = i / PROFILE_STRIDE;
    float t = smooth(0, PROFILE_STRIDE, (float)(i % PROFILE_STRIDE));
    return mixf(station_height(w, k), station_height(w, k + 1), t);
}
void trail_window(World *w, float x, float z) {
    int64_t i = (int64_t)floor(trail_station(w, x, z) / TRAIL_STEP);
    if (i < 0)
        i = 0;
    int64_t start = (i / 16) * 16 - TRAIL_WINDOW / 2;
    if (start < 0)
        start = 0;
    if (start == w->trail.first)
        return;
    w->trail.first = start;
    int64_t first_shelf = start / SHELF_STEPS - 4;
    if (first_shelf != w->trail.first_shelf) {
        /* Retain overlapping lookouts as the small window moves. */
        float previous[TRAIL_SHELVES];
        for (int j = 0; j < TRAIL_SHELVES; j++)
            previous[j] = w->trail.shelves[j];
        for (int j = 0; j < TRAIL_SHELVES; j++) {
            int64_t id = first_shelf + j, old = id - w->trail.first_shelf;
            w->trail.shelves[j] =
                old >= 0 && old < TRAIL_SHELVES ? previous[old] : choose_shelf(w, id);
        }
        w->trail.first_shelf = first_shelf;
    }
    int64_t prev = -1;
    float a = 0, b = 0;
    for (int j = 0; j < TRAIL_WINDOW; j++) {
        int64_t at = start + j, k = at / PROFILE_STRIDE;
        if (k != prev) {
            a = (prev >= 0 ? b : station_height(w, k));
            b = station_height(w, k + 1);
            prev = k;
        }
        w->trail.knots[j] = (TrailKnot){
            bend(w, at), mixf(a, b, smooth(0, PROFILE_STRIDE, (float)(at % PROFILE_STRIDE)))};
    }
}
void trail_coordinates(const World *w, int64_t i, double *x, double *z) {
    coordinates(w, i, side_at(w, i), x, z);
}
Place trail_knot(const World *w, int64_t i) {
    double x, z;
    trail_coordinates(w, i, &x, &z);
    return (Place){(float)(x - w->ox), (float)(z - w->oz)};
}
float height_global(const World *w, double x, double z) {
    float native = raw_height(w, x, z);
    if (w->ascent.count)
        native = ascent_ground(w, x, z, native);
    if (!w->trail.dx && !w->trail.dz)
        return native; /* Planning the opening hike. */
    Place end = finish(w);
    double dx = x - end.x, dz = z - end.z;
    double along = dx * w->trail.dx + dz * w->trail.dz,
           across = -dx * w->trail.dz + dz * w->trail.dx;
    if (along < 0 || fabs(across) > 4800)
        return native;
    int64_t i = (int64_t)floor(along / TRAIL_STEP);
    float t = (float)(along / TRAIL_STEP - i);
    float side = side_at(w, i), next = side_at(w, i + 1);
    float slope = (next - side) / TRAIL_STEP;
    float distance = (float)fabs(across - mixf(side, next, t)) / sqrtf(1 + slope * slope);
    if (distance >= 163)
        return native;
    float bench = mixf(profile_at(w, i), profile_at(w, i + 1), t);
    /* Widen the natural shelf in open, generated country. Keep the measured
       lake and the original lookouts intact on the approach. */
    float width = 28 + fminf(135, fabsf(native - bench) * 1.45f);
    if (w->survey) {
        double boundary = w->survey->cell * (w->survey->side / 2);
        double dist = fmax(fabs(x), fabs(z));
        width = mixf(11, width, smooth((float)boundary - 256, (float)boundary, (float)dist));
    }
    float shelf = 1 - smooth(2.35f, width, distance);
    float join = smooth(0, 10, (float)along);
    return mixf(native, bench, join * shelf);
}
float height_at(const World *w, float x, float z) {
    return height_global(w, w->ox + (double)x, w->oz + (double)z);
}
