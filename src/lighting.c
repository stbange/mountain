#include "mountain.h"
#include <math.h>

static const float sun_azimuth = .609f;

Lighting earth_lighting(void) { return (Lighting){rgb(0x58769f), rgb(0x181a16), 1.f, 1.f}; }

Vec sun_direction(float hour) {
    float angle = (hour - 6) * PI / 12;
    float horizontal = cosf(angle);
    return (Vec){cosf(sun_azimuth) * horizontal, sinf(angle), sinf(sun_azimuth) * horizontal};
}

/* Snap on the global light-space grid, then return to the local frame used by the meshes. */
static float snap(const World *w, Vec eye, Vec axis, double step) {
    double local = (double)eye.x * axis.x + (double)eye.y * axis.y + (double)eye.z * axis.z;
    double origin_phase = remainder((double)w->ox * axis.x + (double)w->oz * axis.z, step);
    return (float)(local - remainder(local + origin_phase, step));
}
Mat shadow_matrix(const World *world, Vec eye, Vec sun, float radius, int resolution) {
    /* This tangent is constant through noon, when the sun crosses the zenith. */
    Vec side = {sinf(sun_azimuth), 0, -cosf(sun_azimuth)};
    Vec up = {sun.y * side.z, sun.z * side.x - sun.x * side.z, -sun.y * side.x};
    float depth = 2600, step = 2 * radius / resolution;
    float z = eye.x * sun.x + eye.y * sun.y + eye.z * sun.z;
    return (Mat){{side.x / radius, up.x / radius, -sun.x / depth, 0, 0, up.y / radius,
                  -sun.y / depth, 0, side.z / radius, up.z / radius, -sun.z / depth, 0,
                  -snap(world, eye, side, step) / radius, -snap(world, eye, up, step) / radius,
                  z / depth, 1}};
}
