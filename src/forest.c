#include "mountain.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* A tree is an occupied volume with woody and leafy cells. The close mesh,
   coarse mesh and the two distant projections all come from that same volume. */
enum { W = 31, H = 48, TILE = 64, BARK = 1, LEAF = 2 };
typedef struct {
    unsigned char v[H][W][W];
} Crown;
static void dab(Crown *v, float x, float y, float z, float rx, float ry, float rz, int material) {
    /* The branching grammar uses a 30-unit leader. Sample its sprays on a finer grid. */
    const float scale = H / 30.f;
    x = (x + .5f) * scale - .5f;
    y = (y + .5f) * scale - .5f;
    z = (z + .5f) * scale - .5f;
    rx *= scale;
    ry *= scale;
    rz *= scale;
    for (int j = (int)floorf(y - ry); j <= y + ry; j++)
        for (int k = (int)floorf(z - rz); k <= z + rz; k++)
            for (int i = (int)floorf(x - rx); i <= x + rx; i++) {
                if ((unsigned)i >= W || (unsigned)k >= W || (unsigned)j >= H)
                    continue;
                float dx = (i - x) / rx, dy = (j - y) / ry, dz = (k - z) / rz;
                if (dx * dx + dy * dy + dz * dz <= 1 && (material == LEAF || !v->v[j][k][i]))
                    v->v[j][k][i] = (unsigned char)material;
            }
}
static void branch(Crown *v, Vec a, Vec b, float radius) {
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    int steps = 1 + (int)(sqrtf(dx * dx + dy * dy + dz * dz) * 2.6f);
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        dab(v, a.x + t * dx, a.y + t * dy, a.z + t * dz, radius * (1 - .38f * t), .67f,
            radius * (1 - .38f * t), BARK);
    }
}
static void grow(Crown *v, const TreeShape *shape, uint32_t seed, int season) {
    float lean = (random2(1, 1, seed) - .5f) * 3,
          width = shape->width * (.81f + .26f * random2(15, 3, seed));
    bool leafy = season != 3 || !shape->deciduous;
    for (int y = 0; y < 30; y++) {
        float trunk = .38f + .70f * (1 - y / 30.f);
        dab(v, 9 + lean * y / 30, y, 9, trunk, .78f, trunk, BARK);
    }
    float first_bough = shape->first_bough + (int)(random2(32, 2, seed) * shape->bough_variation);
    if (shape->forked) {
        for (int i = 0; i < 19; i++) {
            float a = i * 2.4f + random2(i, 1, seed),
                  h = first_bough + (27 - first_bough) * random2(i, 2, seed);
            float r = (2 + 3.8f * random2(i, 3, seed)) * width * (1 - smooth(23, 30, h) * .64f);
            Vec base = {9 + lean * .45f, h - 6, 9},
                tip = {9 + lean * h / 30 + r * cosf(a), h, 9 + r * sinf(a)};
            branch(v, base, tip, .69f);
            if (leafy) {
                float breadth = (2.1f + 1.7f * random2(i, 4, seed)) * shape->foliage;
                dab(v, tip.x, tip.y, tip.z, breadth, 2.8f + random2(i, 8, seed), breadth * .88f,
                    LEAF);
            }
        }
    } else {
        int first = (int)first_bough;
        for (int y = first; y < 30; y += 3) {
            float crown = 1 - (float)(y - first) / (30 - first);
            int rays = 5 + (int)(4 * crown);
            for (int i = 0; i < rays; i++) {
                float angle = y * 1.7f + (i + .52f * random2(i, y, seed)) * 2 * PI / rays;
                float length = (.6f + 8.1f * powf(crown, .77f)) * width *
                               (.74f + .36f * random2(y, i, seed + 4));
                float ca = cosf(angle), sa = sinf(angle), stem = 9 + lean * y / 30;
                Vec root = {stem, (float)y, 9},
                    tip = {stem + ca * length, y - .25f, 9 + sa * length};
                branch(v, root, tip, .58f);
                if (!leafy)
                    continue;
                float droop = shape->droop;
                for (int j = 0; j <= 8; j++) {
                    float t = j / 8.f;
                    float by = y + .22f - droop * sinf(PI * t) + .34f * t;
                    float bx = stem + ca * t * length, bz = 9 + sa * t * length;
                    float spray = (.87f + .37f * crown) * (1 - .51f * t) * shape->foliage;
                    dab(v, bx, by, bz, spray, .69f + .26f * (1 - t), spray, LEAF);
                    if (j == 0 || j == 8)
                        continue;
                    for (int side = -1; side <= 1; side += 2) {
                        float spread = side * (.58f + 1.04f * crown) * (1 - .66f * t);
                        float reach = .14f + .52f * crown;
                        dab(v, bx + ca * reach - sa * spread, by - .34f,
                            bz + sa * reach + ca * spread, spray * .74f, .54f + .28f * crown,
                            spray * .74f, LEAF);
                    }
                }
            }
        }
        if (leafy)
            dab(v, 9 + lean, 28.5f, 9, 1.05f, 1.7f, 1.05f, LEAF);
    }
}
static int get(const Crown *v, int x, int y, int z) {
    return (unsigned)x < W && (unsigned)y < H && (unsigned)z < W ? v->v[y][z][x] : 0;
}
static int coating(const Game *g, const Crown *v, int x, int y, int z, uint32_t seed) {
    int material = get(v, x, y, z);
    if (material == LEAF && g->season == 3 && !get(v, x, y + 1, z) &&
        random2(x + z * W, y, seed) > .13f)
        return 3; /* Snow on an exposed spray. */
    return material;
}
static Color pigment(const Game *g, const Crown *v, int k, int x, int y, int z, uint32_t seed) {
    bool leaf = get(v, x, y, z) == LEAF;
    Color c =
        leaf ? (g->season == 2 ? g->rules->autumn[k] : g->rules->needles[k]) : g->rules->bark[k];
    float grain = random2(x + z * W, y, seed);
    if (coating(g, v, x, y, z, seed) == 3)
        c = rgb(0xd9e2dc);
    if (leaf && g->season == 0)
        c = blend(c, rgb(0xb0ba74), .10f);
    return shade(c, .76f + .27f * grain + .18f * (float)y / H);
}
/* Merge adjacent coplanar faces of the same material. Branch silhouettes and
   snow boundaries still follow the occupied cells, with far fewer triangles. */
static void surface_plane(Mesh *mesh, const Crown *volume, const Game *g, int species,
                          uint32_t seed, int stride, const int dims[3], int axis, int sign,
                          int layer) {
    int u = (axis + 1) % 3, v = (axis + 2) % 3;
    unsigned char mask[H][H];
    int cell[3] = {0};
    cell[axis] = layer;
    for (int j = 0; j < dims[v]; j++)
        for (int i = 0; i < dims[u]; i++) {
            cell[u] = i;
            cell[v] = j;
            int material = get(volume, cell[0], cell[1], cell[2]);
            cell[axis] += sign;
            bool exposed = material && !get(volume, cell[0], cell[1], cell[2]);
            cell[axis] -= sign;
            mask[j][i] =
                exposed ? (unsigned char)coating(g, volume, cell[0], cell[1], cell[2], seed) : 0;
        }
    static const float light[3][2] = {{1, .85f}, {.62f, 1.23f}, {.92f, .78f}};
    for (int j = 0; j < dims[v]; j++)
        for (int i = 0; i < dims[u]; i++) {
            int material = mask[j][i];
            if (!material)
                continue;
            int width = 1, height = 1;
            while (i + width < dims[u] && mask[j][i + width] == material)
                width++;
            while (j + height < dims[v] && !memcmp(&mask[j + height][i], &mask[j][i], width))
                height++;
            for (int row = 0; row < height; row++)
                memset(&mask[j + row][i], 0, width);
            cell[u] = i;
            cell[v] = j;
            Color tint = shade(pigment(g, volume, species, cell[0], cell[1], cell[2], seed),
                               light[axis][sign > 0]);
            Vec corners[4];
            for (int k = 0; k < 4; k++) {
                int point[3];
                point[axis] = layer + (sign > 0);
                point[u] = i + ((k == 1 || k == 2) ? width : 0);
                point[v] = j + (k >= 2 ? height : 0);
                corners[k] =
                    (Vec){(point[0] * stride - W * .5f) / H, (float)(point[1] * stride) / H,
                          (point[2] * stride - W * .5f) / H};
            }
            tex_quad(mesh, corners[0], corners[sign > 0 ? 1 : 3], corners[2],
                     corners[sign > 0 ? 3 : 1], 0, 0, sign > 0 ? width : height,
                     sign > 0 ? height : width, tint, material == BARK ? 3 : 5);
        }
}
static void surface(Mesh *m, const Crown *volume, Crown *coarse, const Game *g, int k,
                    uint32_t seed, int stride) {
    mesh_clear(m);
    const Crown *v = volume;
    if (stride > 1) {
        memset(coarse, 0, sizeof *coarse);
        for (int y = 0; y < H; y++)
            for (int z = 0; z < W; z++)
                for (int x = 0; x < W; x++) {
                    unsigned char value = volume->v[y][z][x];
                    unsigned char *cell = &coarse->v[y / stride][z / stride][x / stride];
                    if (value > *cell)
                        *cell = value;
                }
        v = coarse;
    }
    const int dims[3] = {(W + stride - 1) / stride, (H + stride - 1) / stride,
                         (W + stride - 1) / stride};
    for (int axis = 0; axis < 3; axis++)
        for (int layer = 0; layer < dims[axis]; layer++)
            for (int sign = -1; sign <= 1; sign += 2)
                surface_plane(m, v, g, k, seed, stride, dims, axis, sign, layer);
}
static void project(unsigned char *atlas, const Crown *v, const Game *g, int form, uint32_t seed,
                    int side) {
    int tile = form * 2 + side, sx = (tile % 8) * TILE, sy = (tile / 8) * TILE;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            for (int d = 0; d < W; d++) {
                int vx = side ? d : x, vz = side ? x : d;
                if (!get(v, vx, y, vz))
                    continue;
                Color c = pigment(g, v, form / 8, vx, y, vz, seed);
                c = shade(c, (!get(v, vx, y + 1, vz) ? 1.18f : .89f) + .12f * (1 - (float)d / W));
                int dest = (sy + TILE - 1 - y) * FOREST_ATLAS + sx + (TILE - W) / 2 + x;
                unsigned char *p = atlas + dest * 4;
                p[0] = (unsigned char)(clampf(c.r, 0, 1) * 255);
                p[1] = (unsigned char)(clampf(c.g, 0, 1) * 255);
                p[2] = (unsigned char)(clampf(c.b, 0, 1) * 255);
                p[3] = 255;
                break;
            }
}
void forest_prepare(Forest *f, const Game *g) {
    if (f->atlas && f->seed == g->world.seed && f->season == g->season)
        return;
    if (!f->atlas)
        f->atlas = malloc(FOREST_ATLAS * FOREST_ATLAS * 4);
    if (!f->atlas)
        return;
    Crown *volumes = malloc(2 * sizeof *volumes);
    if (!volumes) {
        forest_free(f);
        return;
    }
    memset(f->atlas, 0, FOREST_ATLAS * FOREST_ATLAS * 4);
    f->seed = g->world.seed;
    f->season = g->season;
    for (int i = 0; i < TREE_FORMS; i++) {
        memset(volumes, 0, sizeof *volumes);
        uint32_t key = cell_hash(i, 154, f->seed);
        grow(volumes, &g->rules->tree_shapes[i / 8], key, g->season);
        surface(&f->close[i], volumes, volumes + 1, g, i / 8, key, 1);
        surface(&f->middle[i], volumes, volumes + 1, g, i / 8, key, 3);
        project(f->atlas, volumes, g, i, key, 0);
        project(f->atlas, volumes, g, i, key, 1);
    }
    free(volumes);
}
void forest_free(Forest *f) {
    for (int i = 0; i < TREE_FORMS; i++) {
        mesh_free(&f->close[i]);
        mesh_free(&f->middle[i]);
    }
    free(f->atlas);
    f->atlas = NULL;
}
void voxel_tree(Mesh *m, const Forest *forest, Tree t, float distance) {
    int form = t.tile * 8 + (t.form & 7);
    float spread = .89f + .25f * ((t.form >> 8) & 255) / 255.f;
    float width = t.height * spread, lean = .09f * (((t.form >> 16) & 255) / 255.f - .5f);
    if (distance < 55) {
        mesh_tree_instance(m, distance < 24 ? &forest->close[form] : &forest->middle[form], &t,
                           width, lean);
        return;
    }
    float height = t.height * TILE / H;
    for (int side = 0; side < 2; side++) {
        int tile = form * 2 + side;
        float a = t.angle + side * PI * .5f, dx = cosf(a) * width * TILE / H * .5f,
              dz = sinf(a) * width * TILE / H * .5f;
        Vec p = {t.x - dx, t.y, t.z - dz}, q = {t.x + dx, t.y, t.z + dz}, u = q, v = p;
        u.y += height;
        v.y += height;
        u.x += cosf(t.angle) * width * lean;
        v.x += cosf(t.angle) * width * lean;
        u.z += sinf(t.angle) * width * lean;
        v.z += sinf(t.angle) * width * lean;
        float s = (tile % 8) / 8.f, t0 = (tile / 8) / 8.f;
        tex_quad(m, p, q, u, v, s + .0002f, t0 + .0002f, s + .1248f, t0 + .1248f,
                 shade(rgb(0xffffff), t.tint), 1.45f);
    }
}
