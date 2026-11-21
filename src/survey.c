#include "mountain.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned little16(const unsigned char *p) { return p[0] | ((unsigned)p[1] << 8); }

static unsigned sample(const Survey *s, int i, int j) {
    return little16(s->data + 2 * ((size_t)j * s->side + i));
}

bool survey_load(Survey *s, const char *root) {
    char path[1152];
    snprintf(path, sizeof path, "%s/%s", root, s->file);
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    unsigned char header[8];
    bool ok = fread(header, 1, sizeof header, f) == sizeof header && !memcmp(header, "MHT1", 4);
    if (ok) {
        s->side = little16(header + 4);
        s->cell = little16(header + 6) / s->scale;
        ok = s->side >= 3 && s->side <= 2049 && (s->side & 1) && s->cell > 0;
    }
    if (ok) {
        size_t n = 2 * (size_t)s->side * s->side;
        s->data = malloc(n);
        ok = s->data && fread(s->data, 1, n, f) == n;
    }
    fclose(f);
    if (!ok) {
        free(s->data);
        s->data = NULL;
        return false;
    }
    s->shore[0] = s->shore[2] = (int)s->side - 1;
    s->shore[1] = s->shore[3] = 0;
    for (int j = 0; j < (int)s->side; j++)
        for (int i = 0; i < (int)s->side; i++)
            if (sample(s, i, j) & 0x8000) {
                if (i < s->shore[0])
                    s->shore[0] = i;
                if (i > s->shore[1])
                    s->shore[1] = i;
                if (j < s->shore[2])
                    s->shore[2] = j;
                if (j > s->shore[3])
                    s->shore[3] = j;
            }
    return true;
}

static float coordinate(const Survey *s, float p) {
    return clampf(p / s->cell + (int)(s->side / 2), 0, (float)s->side - 1.001f);
}

static float elevation(const Survey *s, int i, int j) {
    unsigned v = sample(s, i, j);
    /* USGS encodes a flat water surface. Lower its bed for shoreline intersections. */
    return (v & 0x8000) ? -.52f : ((v & 0x7fff) * .25f - s->surface) / s->scale;
}

float survey_height(const Survey *s, float x, float z) {
    float u = coordinate(s, x), v = coordinate(s, z);
    int i = (int)u, j = (int)v;
    return mixf(mixf(elevation(s, i, j), elevation(s, i + 1, j), u - i),
                mixf(elevation(s, i, j + 1), elevation(s, i + 1, j + 1), u - i), v - j);
}

bool survey_lake(const Survey *s, float x, float z) {
    int i = (int)coordinate(s, x), j = (int)coordinate(s, z);
    if (i < s->shore[0] - 1 || i > s->shore[1] || j < s->shore[2] - 1 || j > s->shore[3])
        return false;
    return (sample(s, i, j) | sample(s, i + 1, j) | sample(s, i, j + 1) | sample(s, i + 1, j + 1)) &
           0x8000;
}

float water_global(const World *w, double ax, double az) {
    if (w->profile == LUNAR || w->profile == PRISM)
        return -10000;
    if (!w->survey)
        return w->water;
    float edge = w->survey->cell * (w->survey->side / 2);
    return fabs(ax) >= edge || fabs(az) >= edge || survey_lake(w->survey, (float)ax, (float)az)
               ? w->water
               : -10000.f;
}
float water_at(const World *w, float x, float z) {
    return water_global(w, w->ox + (double)x, w->oz + (double)z);
}
