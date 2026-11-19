#include "mountain.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void mesh_clear(Mesh *m) {
    m->count = 0;
    m->failed = false;
}
void mesh_free(Mesh *m) {
    free(m->v);
    *m = (Mesh){0};
}
static Vertex *reserve(Mesh *m, size_t n) {
    if (m->failed)
        return NULL;
    if (m->count + n > m->capacity) {
        size_t next = m->capacity ? m->capacity * 2 : 1024;
        if (next < m->count + n)
            next = m->count + n;
        Vertex *v = realloc(m->v, next * sizeof *v);
        if (!v) {
            m->failed = true;
            return NULL;
        }
        m->v = v;
        m->capacity = next;
    }
    Vertex *start = m->v + m->count;
    m->count += n;
    return start;
}
void mesh_tree_instance(Mesh *out, const Mesh *model, const Tree *tree, float width, float lean) {
    Vertex *v = reserve(out, model->count);
    if (!v)
        return;
    float c = cosf(tree->angle) * width, s = sinf(tree->angle) * width;
    for (size_t i = 0; i < model->count; i++) {
        Vertex p = model->v[i];
        float x = p.x + lean * p.y * p.y;
        v[i] = (Vertex){tree->x + c * x - s * p.z,
                        tree->y + p.y * tree->height,
                        tree->z + s * x + c * p.z,
                        p.r * tree->tint,
                        p.g * tree->tint,
                        p.b * tree->tint,
                        p.u,
                        p.v,
                        p.material};
    }
}
static Vertex vertex(Vec p, Color c, float u, float v, float mode) {
    return (Vertex){p.x, p.y, p.z, c.r, c.g, c.b, u, v, mode};
}
void tri(Mesh *m, Vec a, Vec b, Vec c, Color ca, Color cb, Color cc, float material) {
    Vertex *v = reserve(m, 3);
    if (!v)
        return;
    v[0] = vertex(a, ca, 0, 0, material);
    v[1] = vertex(b, cb, 0, 0, material);
    v[2] = vertex(c, cc, 0, 0, material);
}
void tex_quad(Mesh *m, Vec a, Vec b, Vec c, Vec d, float u0, float v0, float u1, float v1,
              Color tint, float material) {
    Vertex *v = reserve(m, 6);
    if (!v)
        return;
    v[0] = vertex(a, tint, u0, v1, material);
    v[1] = vertex(b, tint, u1, v1, material);
    v[2] = vertex(c, tint, u1, v0, material);
    v[3] = v[0];
    v[4] = v[2];
    v[5] = vertex(d, tint, u0, v0, material);
}
void quad(Mesh *m, Vec a, Vec b, Vec c, Vec d, Color tint, float material) {
    tex_quad(m, a, b, c, d, 0, 0, 1, 1, tint, material);
}
Vec transform(Pose p, Vec v) {
    return (Vec){p.origin.x + cosf(p.angle) * v.x - sinf(p.angle) * v.z, p.origin.y + v.y,
                 p.origin.z + sinf(p.angle) * v.x + cosf(p.angle) * v.z};
}
void tilted_box(Mesh *m, Pose p, Vec center, Vec size, Color color, float material, float roll) {
    Vec v[8];
    for (int i = 0; i < 8; i++) {
        Vec q = {center.x + ((i & 1) ? .5f : -.5f) * size.x,
                 center.y + ((i & 2) ? .5f : -.5f) * size.y,
                 center.z + ((i & 4) ? .5f : -.5f) * size.z};
        float x = q.x - center.x, y = q.y - center.y;
        q.x = center.x + x * cosf(roll) - y * sinf(roll);
        q.y = center.y + x * sinf(roll) + y * cosf(roll);
        v[i] = transform(p, q);
    }
    const int faces[][4] = {{0, 1, 3, 2}, {5, 4, 6, 7}, {4, 0, 2, 6},
                            {1, 5, 7, 3}, {2, 3, 7, 6}, {4, 5, 1, 0}};
    const float lights[] = {.86f, .96f, .78f, .88f, 1.08f, .61f};
    for (int f = 0; f < 6; f++)
        quad(m, v[faces[f][0]], v[faces[f][1]], v[faces[f][2]], v[faces[f][3]],
             shade(color, lights[f]), material);
}
void box_material(Mesh *m, Pose p, Vec center, Vec size, Color color, float material) {
    tilted_box(m, p, center, size, color, material, 0);
}
void box(Mesh *m, Pose p, Vec center, Vec size, Color color) {
    box_material(m, p, center, size, color, 0);
}
void sprite(Mesh *m, float x, float y, float z, float width, float height, float angle, int tile,
            Color tint) {
    float dx = cosf(angle) * width * .5f, dz = sinf(angle) * width * .5f;
    Vec a = {x - dx, y, z - dz}, b = {x + dx, y, z + dz}, c = b, d = a;
    c.y += height;
    d.y += height;
    float u = (tile % 4) * .25f, v = (tile / 4) * .25f;
    tex_quad(m, a, b, c, d, u + .00035f, v + .00035f, u + .24965f, v + .24965f, tint,
             tile == 7 ? 1.f : 1.25f);
}
Mat identity(void) { return (Mat){{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}}; }
Mat multiply(Mat a, Mat b) {
    Mat r = {0};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
    return r;
}
Mat view_matrix(Vec pos, float yaw, float pitch) {
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    Vec right = {cy, 0, sy}, up = {-sy * sp, cp, cy * sp}, back = {-sy * cp, -sp, cy * cp};
    Mat r = {{right.x, up.x, back.x, 0, right.y, up.y, back.y, 0, right.z, up.z, back.z, 0,
              -right.x * pos.x - right.z * pos.z, -up.x * pos.x - up.y * pos.y - up.z * pos.z,
              -back.x * pos.x - back.y * pos.y - back.z * pos.z, 1}};
    return r;
}
Mat perspective(float fovy, float aspect, float near_z, float far_z) {
    float s = 1 / tanf(fovy * .5f), d = near_z - far_z;
    return (Mat){{s / aspect, 0, 0, 0, 0, s, 0, 0, 0, 0, (far_z + near_z) / d, -1, 0, 0,
                  2 * far_z * near_z / d, 0}};
}
