#include "mountain.h"
#include <math.h>

/* Regenerate only occupied habitat cells near the observer. The stable home
   range and continuously sampled motion avoid a population history or respawn timer. */
static bool in_cabin(const Game *g, float x, float z) {
    if (!g->cabin.present)
        return false;
    if (g->cabin.inside) {
        Cabin frame = cabin_frame(g);
        Vec p = cabin_local(&frame, x, z);
        const Layout *l = cabin_layout(g);
        return p.x < l->width + 1 && p.x > -l->width - (l->balcony ? 4.2f : 1.f) &&
               fabsf(p.z) < l->depth + 1;
    }
    Vec p = cabin_local(&g->cabin, x, z);
    return fabsf(p.x) < g->rules->layout[0].width + 1.3f &&
           fabsf(p.z) < g->rules->layout[0].depth + 2.8f;
}
static bool occupant(const Game *g, int i, int j, int member, Animal *a) {
    const World *w = &g->world;
    int64_t ix = w->ox / 48 + i, iz = w->oz / 48 + j;
    uint32_t home = cell_hash(ix, iz, w->seed + 615);
    bool meadow = ix == 0 && iz == 0;
    if (!meadow && (home & 7) < 3)
        return false;
    float x = (i + .13f + .74f * random2(0, 2, home)) * 48,
          z = (j + .13f + .74f * random2(0, 3, home)) * 48;
    if (meadow) {
        float b = trail_bearing(w);
        x = (float)(-w->ox + sin(b) * 18 - cos(b) * 6.5);
        z = (float)(-w->oz - cos(b) * 18 - sin(b) * 6.5);
    }
    if (hypotf(g->x - x, g->z - z) > 137)
        return false;
    float h = height_at(w, x, z),
          rise = fabsf(height_at(w, x + 5, z) - h) + fabsf(height_at(w, x, z + 5) - h);
    float cover = noise2((w->ox + (double)x) / 38, (w->oz + (double)z) / 38, w->seed + 61);
    AnimalKind kind;
    if (meadow)
        kind = DEER;
    else if (home % 11 == 0)
        kind = EAGLE;
    else if (h > w->water + 105 || rise > 4.6f)
        kind = MARMOT;
    else if (home % 4 == 1)
        kind = FOX;
    else
        kind = cover > .53f ? HARE : DEER;
    if (member && (kind != DEER || member > 1 + (int)(home % 2)))
        return false;
    if (kind == MARMOT && (g->season == 3 || g->hour < 7 || g->hour > 19 || g->weather == 2))
        return false;
    if (kind == EAGLE && (g->hour < 6 || g->hour > 20 || g->weather == 2))
        return false;
    if (kind == FOX && g->hour > 10 && g->hour < 15 && (home & 16))
        return false;
    uint32_t id = cell_hash(member, 4, home);
    float offset = random2(0, 7, id) * 2 * PI;
    if (member) {
        x += 3.7f * member * cosf(offset);
        z += 3.7f * member * sinf(offset);
    }
    float ground_y = height_at(w, x, z);
    if (kind != EAGLE && ground_y < water_at(w, x, z) + .25f)
        return false;
    double time = g->elapsed;
    float t = (float)fmod(time * .105 + offset, 2 * 3.141592653589793);
    float activity = smooth(-.4f, .67f, sinf(t + offset));
    float phase = (float)fmod(time * (kind == HARE    ? 5.8
                                      : kind == EAGLE ? 2.1
                                                      : 3.4) +
                                  offset,
                              2 * 3.141592653589793);
    float yaw = offset, y = ground_y;
    if (kind == EAGLE) {
        x += 21 * cosf(t);
        z += 15 * sinf(t);
        y = fmaxf(ground_y, height_at(w, x, z)) + 23 + 4 * sinf(t);
        yaw = atan2f(-21 * sinf(t), -15 * cosf(t));
    } else {
        float dx = x - g->x, dz = z - g->z, r = fmaxf(.1f, hypotf(dx, dz));
        float shy = g->cabin.inside ? 0 : 1 - smooth(2.5f, 10, r);
        float roam = kind == MARMOT ? .55f : (kind == FOX ? 5.2f : 2.5f);
        float nx = x + roam * sinf(t) + shy * 4 * dx / r,
              nz = z + roam * .65f * cosf(t) + shy * 4 * dz / r;
        float level = height_at(w, nx, nz);
        if (level > water_at(w, nx, nz) + .2f && fabsf(level - ground_y) < 2.4f) {
            x = nx;
            z = nz;
            y = level;
        }
        if (in_cabin(g, x, z))
            return false;
        yaw = shy > .50f ? atan2f(dx, -dz) : atan2f(cosf(t), .65f * sinf(t));
        activity = fmaxf(shy, activity);
        if (kind == HARE)
            y += activity * .25f * powf(fmaxf(0, sinf(phase)), 2);
    }
    *a = (Animal){kind, id, {x, y, z}, yaw, .84f + .34f * random2(7, 1, id), phase, activity};
    return true;
}
int fauna_near(const Game *g, Animal *out, int capacity) {
    if (g->world.profile > SURVEYED)
        return 0;
    int cx = (int)floorf(g->x / 48), cz = (int)floorf(g->z / 48), n = 0;
    for (int j = cz - 3; j <= cz + 3; j++)
        for (int i = cx - 3; i <= cx + 3; i++)
            for (int member = 0; member < 3 && n < capacity; member++) {
                if (!occupant(g, i, j, member, &out[n]))
                    break;
                n++;
            }
    return n;
}
