#include "mountain.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Search two connected legs on the same terrain the player hikes. A min-heap
   keeps seed changes cheap, and each undirected slope is sampled only once. */
enum { HALF_GRID = 34, GRID = 2 * HALF_GRID + 1, CELLS = GRID * GRID, STEP = 6 };
static const int dx[] = {1, 0, 1, -1, -1, 0, -1, 1};
static const int dz[] = {0, 1, 1, 1, 0, -1, -1, -1};
typedef struct {
    float height[CELLS], edge[CELLS][4], cost[CELLS];
    int parent[CELLS], heap[CELLS], position[CELLS], size;
    int shore[CELLS], queue[CELLS];
    bool wet[CELLS], used[CELLS];
} Search;

static Place location(int k) {
    return (Place){(k % GRID - HALF_GRID) * STEP, (k / GRID - HALF_GRID) * STEP};
}
static int neighbor(int k, int d) {
    int x = k % GRID + dx[d], z = k / GRID + dz[d];
    return x >= 0 && z >= 0 && x < GRID && z < GRID ? z * GRID + x : -1;
}
static void prepare(Search *s, const World *w) {
    int begin = 0, end = 0;
    for (int k = 0; k < CELLS; k++) {
        Place p = location(k);
        s->height[k] = height_at(w, p.x, p.z);
        s->wet[k] = s->height[k] < water_at(w, p.x, p.z) + .18f;
        s->shore[k] = CELLS;
        if (s->wet[k]) {
            s->shore[k] = 0;
            s->queue[end++] = k;
        }
    }
    while (begin < end) {
        int k = s->queue[begin++];
        for (int d = 0; d < 4; d++) {
            int n = neighbor(k, d < 2 ? d : d + 2);
            if (n >= 0 && s->shore[n] > s->shore[k] + 1) {
                s->shore[n] = s->shore[k] + 1;
                s->queue[end++] = n;
            }
        }
    }
    for (int k = 0; k < CELLS; k++) {
        Place a = location(k);
        for (int d = 0; d < 4; d++) {
            int n = neighbor(k, d);
            if (n < 0 || s->wet[k] || s->wet[n])
                continue;
            Place b = location(n);
            float length = d < 2 ? STEP : STEP * 1.414214f;
            const int steps = 6;
            float last = s->height[k], grade = 0, step = length / steps;
            if (fabsf(last - s->height[n]) > length * .57f)
                continue;
            for (int j = 1; j <= steps; j++) {
                float t = (float)j / steps, x = mixf(a.x, b.x, t), z = mixf(a.z, b.z, t);
                float h = j == steps ? s->height[n] : height_at(w, x, z);
                grade = fmaxf(grade, fabsf(h - last) / step);
                if (grade > .57f || h < water_at(w, x, z) + .16f)
                    break;
                last = h;
                if (j == steps)
                    s->edge[k][d] = length * (1 + grade * grade * 16 +
                                              .38f * noise2(a.x * .045f, a.z * .045f, w->seed + 9));
            }
        }
    }
}
static void put(Search *s, int slot, int node) {
    s->heap[slot] = node;
    s->position[node] = slot;
}
static void offer(Search *s, int n) {
    int i = s->position[n] < 0 ? s->size++ : s->position[n];
    while (i && s->cost[n] < s->cost[s->heap[(i - 1) / 2]]) {
        put(s, i, s->heap[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
    put(s, i, n);
}
static int take(Search *s) {
    int root = s->heap[0], last = s->heap[--s->size], i = 0;
    while (2 * i + 1 < s->size) {
        int child = 2 * i + 1;
        if (child + 1 < s->size && s->cost[s->heap[child + 1]] < s->cost[s->heap[child]])
            child++;
        if (s->cost[last] <= s->cost[s->heap[child]])
            break;
        put(s, i, s->heap[child]);
        i = child;
    }
    put(s, i, last);
    s->position[root] = -1;
    return root;
}
static void paths(Search *s, int start) {
    for (int i = 0; i < CELLS; i++) {
        s->cost[i] = FLT_MAX;
        s->position[i] = s->parent[i] = -1;
    }
    s->cost[start] = 0;
    offer(s, start);
    while (s->size) {
        int k = take(s);
        for (int d = 0; d < 8; d++) {
            int n = neighbor(k, d);
            if (n < 0)
                continue;
            float edge = d < 4 ? s->edge[k][d] : s->edge[n][d - 4];
            float cost = s->cost[k] + edge * (s->used[n] ? 4 : 1);
            if (edge && cost < s->cost[n]) {
                s->cost[n] = cost;
                s->parent[n] = k;
                offer(s, n);
            }
        }
    }
}
static int viewpoint(const Search *s, const World *w, int start, bool ridge) {
    Place a = location(start);
    float best = -FLT_MAX;
    int chosen = start;
    float facing = w->survey ? w->survey->bearing : w->phase * .24f;
    float vx = sinf(facing), vz = -cosf(facing);
    for (int k = 0; k < CELLS; k++) {
        if (s->parent[k] < 0)
            continue;
        Place p = location(k);
        float distance = hypotf(p.x - a.x, p.z - a.z), score;
        if (ridge) {
            /* Look for a shoulder with a drop towards the open valley. */
            float fore = height_at(w, p.x + 32 * vx, p.z + 32 * vz);
            score = 1.35f * s->height[k] + .8f * (s->height[k] - fore) -
                    .70f * fabsf(distance - 157) - .10f * s->cost[k] + .12f * hypotf(p.x, p.z);
        } else {
            float forward = p.x * vx + p.z * vz;
            score = -.65f * fabsf(distance - 88) - 6 * abs(s->shore[k] - 2) + .51f * forward -
                    .13f * s->cost[k] + .10f * s->height[k];
        }
        if (score > best) {
            best = score;
            chosen = k;
        }
    }
    return chosen;
}
static void append(Route *r, Place p) {
    if (r->count && r->count < ROUTE_POINTS) {
        Place a = r->point[r->count - 1];
        r->length += hypotf(p.x - a.x, p.z - a.z);
    }
    if (r->count < ROUTE_POINTS)
        r->point[r->count++] = p;
}
static void leg(Search *s, World *w, int start, int end) {
    int count = 0;
    for (int k = end; k != start; k = s->parent[k]) {
        s->used[k] = true;
        s->queue[count++] = k;
    }
    for (int i = count - 1; i >= 0; i--) {
        int k = s->queue[i];
        Place p = location(k);
        if (i > 0) {
            Place a = location(s->parent[k]), b = location(s->queue[i - 1]);
            if (b.x - p.x == p.x - a.x && b.z - p.z == p.z - a.z)
                continue;
            /* Round the corner inside the two validated grid edges. */
            Place enter = {mixf(p.x, a.x, .32f), mixf(p.z, a.z, .32f)};
            Place leave = {mixf(p.x, b.x, .32f), mixf(p.z, b.z, .32f)};
            Place q = {p.x + .08f * (a.x + b.x - 2 * p.x), p.z + .08f * (a.z + b.z - 2 * p.z)};
            float level = height_at(w, q.x, q.z);
            if (level >= water_at(w, q.x, q.z) + .18f &&
                fabsf(level - height_at(w, enter.x, enter.z)) <
                    .58f * hypotf(q.x - enter.x, q.z - enter.z) &&
                fabsf(level - height_at(w, leave.x, leave.z)) <
                    .58f * hypotf(q.x - leave.x, q.z - leave.z)) {
                append(&w->route, enter);
                append(&w->route, q);
                p = leave;
            }
        }
        append(&w->route, p);
    }
}
/* On open ground a long straight can bend around a glade. Only accept the
   curve when a metre-by-metre walk still clears water and steep terrain. */
static void shape_tread(World *w) {
    Route shaped = {0};
    const Route *r = &w->route;
    append(&shaped, r->point[0]);
    for (int i = 1; i < r->count; i++) {
        Place a = r->point[i - 1], b = r->point[i];
        float dx = b.x - a.x, dz = b.z - a.z, len = hypotf(dx, dz);
        if (len > 15 && shaped.count + 5 + (r->count - i) < ROUTE_POINTS) {
            float side = (random2((int)a.x, (int)a.z, w->seed + 313) > .5f ? 1.f : -1.f);
            float bend = side * fminf(3.4f, len * .09f);
            Place curve[6];
            curve[0] = a;
            bool dry = true;
            float last = height_at(w, a.x, a.z);
            for (int j = 1; j <= 5 && dry; j++) {
                float t = j / 5.f, shift = bend * sinf(PI * t);
                curve[j] = (Place){mixf(a.x, b.x, t) - shift * dz / len,
                                   mixf(a.z, b.z, t) + shift * dx / len};
                Place prev = curve[j - 1], next = curve[j];
                float distance = hypotf(next.x - prev.x, next.z - prev.z);
                int steps = (int)ceilf(distance);
                for (int k = 1; k <= steps; k++) {
                    float u = (float)k / steps, x = mixf(prev.x, next.x, u),
                          z = mixf(prev.z, next.z, u);
                    float h = height_at(w, x, z);
                    if (h < water_at(w, x, z) + .19f || fabsf(h - last) > .52f * distance / steps) {
                        dry = false;
                        break;
                    }
                    last = h;
                }
            }
            if (dry)
                for (int j = 1; j < 5; j++)
                    append(&shaped, curve[j]);
        }
        append(&shaped, b);
        if (i == r->overlook)
            shaped.overlook = shaped.count - 1;
    }
    w->route = shaped;
}
void route_generate(World *w) {
    Route *r = &w->route;
    *r = (Route){0};
    append(r, (Place){0, 0});
    Search *s = calloc(1, sizeof *s);
    if (!s)
        return;
    prepare(s, w);
    int start = CELLS / 2;
    paths(s, start);
    int shore = viewpoint(s, w, start, false);
    leg(s, w, start, shore);
    r->overlook = r->count - 1;
    paths(s, shore);
    leg(s, w, shore, viewpoint(s, w, shore, true));
    shape_tread(w);
    Place lake = {0}, stop = location(shore);
    for (int k = 0; k < CELLS; k++) {
        if (!s->wet[k])
            continue;
        Place p = location(k);
        float dx = p.x - stop.x, dz = p.z - stop.z;
        float weight = fmaxf(0, 1 - (dx * dx + dz * dz) / (115.f * 115.f));
        lake.x += weight * dx;
        lake.z += weight * dz;
    }
    float n = fmaxf(1, hypotf(lake.x, lake.z));
    r->sight[0] = (Place){lake.x / n, lake.z / n};
    float bearing = w->survey ? w->survey->bearing : w->phase * .24f;
    r->sight[1] = (Place){sinf(bearing), -cosf(bearing)};
    free(s);
}
float trail_bearing(const World *w) {
    if (w->survey)
        return w->survey->bearing;
    if (w->route.count < 2)
        return 0;
    Place p = w->route.point[1];
    return clampf(.58f * atan2f(p.x, -p.z), -.77f, .77f);
}
static float segment_squared(Place a, Place b, float x, float z) {
    float dx = b.x - a.x, dz = b.z - a.z;
    float t = clampf(((x - a.x) * dx + (z - a.z) * dz) / (dx * dx + dz * dz), 0, 1);
    dx = x - a.x - t * dx;
    dz = z - a.z - t * dz;
    return dx * dx + dz * dz;
}
float route_distance(const Route *r, float x, float z) {
    float best = 32 * 32;
    if (fabsf(x) <= HALF_GRID * STEP + 32 && fabsf(z) <= HALF_GRID * STEP + 32)
        for (int i = 1; i < r->count; i++)
            best = fminf(best, segment_squared(r->point[i - 1], r->point[i], x, z));
    return sqrtf(best);
}
float trail_distance(const World *w, float x, float z) {
    float best = route_distance(&w->route, (float)(w->ox + (double)x), (float)(w->oz + (double)z));
    double s = trail_station(w, x, z);
    if (s < -32)
        return best;
    Place end = w->route.point[w->route.count - 1];
    double cross =
        -(w->ox + (double)x - end.x) * w->trail.dz + (w->oz + (double)z - end.z) * w->trail.dx;
    if (fabs(cross) > 4000)
        return best;
    int64_t k = (int64_t)floor(s / TRAIL_STEP);
    int64_t from = k > 2 ? k - 2 : 0;
    Place a = trail_knot(w, from);
    for (int64_t i = from; i <= k + 1; i++) {
        Place b = trail_knot(w, i + 1);
        best = fminf(best, sqrtf(segment_squared(a, b, x, z)));
        a = b;
    }
    return best;
}
static void paint(unsigned char *pixels, Place a, Place b, float cx, float cz) {
    int side = TRAIL_TEXTURE_SIZE;
    a.x -= cx;
    a.z -= cz;
    b.x -= cx;
    b.z -= cz;
    float lo_x = fmaxf(-side / 2, floorf(fminf(a.x, b.x)) - 6),
          hi_x = fminf(side / 2 - 1, ceilf(fmaxf(a.x, b.x)) + 6);
    float lo_z = fmaxf(-side / 2, floorf(fminf(a.z, b.z)) - 6),
          hi_z = fminf(side / 2 - 1, ceilf(fmaxf(a.z, b.z)) + 6);
    if (lo_x > hi_x || lo_z > hi_z)
        return;
    for (int z = (int)lo_z; z <= (int)hi_z; z++)
        for (int x = (int)lo_x; x <= (int)hi_x; x++) {
            unsigned char d =
                (unsigned char)fminf(255, sqrtf(segment_squared(a, b, x + .5f, z + .5f)) * 42.5f);
            unsigned char *cell = &pixels[(z + side / 2) * side + x + side / 2];
            if (d < *cell)
                *cell = d;
        }
}
void route_texture(const World *w, float cx, float cz, unsigned char *pixels) {
    memset(pixels, 255, TRAIL_TEXTURE_SIZE * TRAIL_TEXTURE_SIZE);
    const Route *r = &w->route;
    double ax = w->ox + (double)cx, az = w->oz + (double)cz;
    if (fabs(ax) < 512 && fabs(az) < 512)
        for (int i = 1; i < r->count; i++)
            paint(pixels, r->point[i - 1], r->point[i], (float)ax, (float)az);
    int64_t k = (int64_t)floor(trail_station(w, cx, cz) / TRAIL_STEP);
    int radius = TRAIL_TEXTURE_SIZE / (2 * TRAIL_STEP) + 2;
    int64_t first = k > radius ? k - radius : 0;
    Place a = trail_knot(w, first);
    for (int64_t i = first; i <= k + radius; i++) {
        Place b = trail_knot(w, i + 1);
        paint(pixels, a, b, cx, cz);
        a = b;
    }
}
