#include "mountain.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { SWITCHBACK = 38 };
static float crest_profile(float f) {
    return .53f * smooth(0, .48f, f) - .08f * smooth(.48f, .58f, f) + .55f * smooth(.58f, 1, f);
}
static float spine(float s) { return .33f * s + 13 * sinf(s * .008f) + .42f * fminf(s, 135); }
void ascent_generate(World *w) {
    Ascent *a = &w->ascent;
    a->start = w->route.point[w->route.count - 1];
    float sign = random2(13, 7, w->seed) > .5f ? 1 : -1;
    a->along = (Place){-w->trail.dz * sign, w->trail.dx * sign};
    /* Turn away from the onward trail so its graded shelf never cuts a hairpin. */
    a->across = (Place){-w->trail.dx, -w->trail.dz};
    a->count = SUMMIT_POINTS;
    a->pass = 13;
    a->base = w->trail.start_y;
    a->rise = 448 + 104 * random2(9, 5, w->seed);
    for (int i = 0; i < a->count; i++) {
        float s = i * SWITCHBACK, f = (float)i / (a->count - 1);
        float turns = (i % 2 ? 1 : -1) * (67 + 30 * random2(i, 19, w->seed)) * smooth(0, .15f, f) *
                      (1 - smooth(.84f, 1, f));
        float t = spine(s) + turns;
        a->point[i] = (Place){a->start.x + s * a->along.x + t * a->across.x,
                              a->start.z + s * a->along.z + t * a->across.z};
        a->level[i] = a->base + a->rise * crest_profile(f);
    }
}
static float projection(const Ascent *a, double x, double z, Place axis) {
    return (float)((x - a->start.x) * axis.x + (z - a->start.z) * axis.z);
}
/* The longitudinal position selects nearby hairpins even where separate traverses overlap in plan.
 */
static float nearest_leg(const Ascent *a, float x, float z, float s, float *level) {
    int center = (int)floorf(s / SWITCHBACK);
    int first = (int)clampf(center - 2, 0, a->count - 2),
        last = (int)clampf(center + 2, 0, a->count - 2);
    float best = 1e9f;
    for (int i = first; i <= last; i++) {
        Place p = a->point[i], q = a->point[i + 1];
        float dx = q.x - p.x, dz = q.z - p.z,
              t = clampf(((x - p.x) * dx + (z - p.z) * dz) / (dx * dx + dz * dz), 0, 1);
        float d = hypotf(x - p.x - t * dx, z - p.z - t * dz);
        if (d < best) {
            best = d;
            *level = mixf(a->level[i], a->level[i + 1], t);
        }
    }
    return best;
}
float ascent_distance(const World *w, double x, double z) {
    const Ascent *a = &w->ascent;
    if (!a->count)
        return 10000;
    float s = projection(a, x, z, a->along);
    if (s < -40 || s > (a->count - 1) * SWITCHBACK + 40 ||
        fabsf(projection(a, x, z, a->across) - spine(s)) > 165)
        return 10000;
    float level = 0;
    return nearest_leg(a, (float)x, (float)z, s, &level);
}
float ascent_ground(const World *w, double x, double z, float natural) {
    const Ascent *a = &w->ascent;
    float s = projection(a, x, z, a->along), end = (a->count - 1) * SWITCHBACK;
    if (s <= -10 || s > end + 480)
        return natural;
    float t = projection(a, x, z, a->across) - spine(s), at = fabsf(t);
    if (at > 560)
        return natural;
    float f = s / end, rough = noise2(x / 40, z / 40, w->seed + 353) - .5f;
    float crest = a->base + a->rise * crest_profile(f) - .065f * at - .0007f * t * t +
                  rough * 19 * smooth(0, 100, s);
    /* The summit has a walkable crown and an oversteepened lee face. */
    float lee = fmaxf(0, s - end);
    crest -= .61f * lee + 108 * smooth(5, 14, lee);
    float wall = 1.36f * fabsf(at - (188 + 36 * sinf(s * .021f))) + 1.02f * fabsf(s - end * .57f);
    float horns = a->base + a->rise * 1.15f - wall;
    float massif = fmaxf(crest, mixf(crest, horns + rough * 25, smooth(96, 177, at)));
    float blend_in =
        smooth(0, 148, s) * (1 - smooth(365, 560, at)) * (1 - smooth(end + 220, end + 480, s));
    float height = mixf(natural, fmaxf(natural, massif), blend_in);
    /* The pass is an open col between two horns. Shape the whole shoulder
       through the older relief so the tread never runs in a narrow trench. */
    float shoulder =
        smooth(22, 145, s) * (1 - smooth(106, 184, at)) * (1 - smooth(end + 40, end + 135, s));
    height = mixf(height, crest, shoulder);
    if (s <= end + 4 && at < 150) {
        float bench = 0, dist = nearest_leg(a, (float)x, (float)z, s, &bench);
        height = mixf(height, bench, (1 - smooth(2.5f, 22, dist)) * smooth(-10, 0, s));
    }
    return height;
}
Place field_site(const World *w, int stage) {
    const Route *r = &w->route;
    if (w->ascent.count) {
        if (stage == 0)
            return w->ascent.point[0];
        return w->ascent.point[stage == 1 ? w->ascent.pass : w->ascent.count - 1];
    }
    int i = stage == 2 ? r->count - 1
                       : (stage == 1 ? r->overlook : (r->overlook > 3 ? r->overlook / 2 : 1));
    return r->point[i < r->count ? i : 0];
}
static float clue_yaw(const World *w, int stage) {
    Place a = field_site(w, stage), b = field_site(w, stage + 1);
    return atan2f(b.x - a.x, a.z - b.z);
}
Place field_pipe(const World *w, int which) {
    Place p = field_site(w, 1);
    float yaw = clue_yaw(w, 1), offset = (which - 1) * 3.2f;
    return (Place){p.x + cosf(yaw) * 3.5f + sinf(yaw) * offset,
                   p.z + sinf(yaw) * 3.5f - cosf(yaw) * offset};
}
int field_stage(const Game *g) { return (g->notes >> (2 * g->world.destination)) & 3; }
static float distance_to(const Game *g, Place p) {
    return (float)hypot(g->world.ox + (double)g->x - p.x, g->world.oz + (double)g->z - p.z);
}
Interaction field_near(const Game *g) {
    int stage = field_stage(g);
    if (!g->world.place || g->cabin.inside || g->fall_time > 0 || stage >= 3)
        return NEAR_NONE;
    return distance_to(g, field_site(&g->world, stage)) < (stage == 1 ? 7.2f : 5.5f)
               ? (stage == 2 ? NEAR_TREASURE : NEAR_READ)
               : NEAR_NONE;
}
const char *compass_bearing(float yaw) {
    static const char *names[] = {"north", "northeast", "east", "southeast",
                                  "south", "southwest", "west", "northwest"};
    return names[((int)floorf(yaw * 4 / PI + .5f) + 8) % 8];
}
const char *field_hint(const Game *g) {
    if (!g->world.place)
        return "";
    if (g->field_note[0])
        return g->field_note;
    int stage = field_stage(g);
    return stage == 3 ? g->world.place->signs[2] : g->world.place->notes[stage];
}
void field_refresh(Game *g) {
    const World *w = &g->world;
    const Destination *d = w->place;
    g->field_note[0] = 0;
    g->note_timer = field_stage(g) ? 12 : 0;
    if (!d)
        return;
    int stage = field_stage(g);
    if (stage == 1)
        snprintf(g->field_note, sizeof g->field_note, "%s Head %s. %s", d->notes[1],
                 compass_bearing(clue_yaw(w, 0)),
                 d->skill == ECHO   ? "Sound the pipes from shortest to tallest."
                 : d->skill == DUSK ? "Return in evening light."
                                    : "Sight along the two markers.");
    else if (stage == 2)
        snprintf(g->field_note, sizeof g->field_note,
                 "The last marks point %s. Look for %s beyond them.",
                 compass_bearing(clue_yaw(w, 1)), d->signs[2]);
    else if (stage == 3)
        snprintf(g->field_note, sizeof g->field_note,
                 "Discovered %s. Recorded in your field guide.", d->signs[2]);
}
bool field_read(Game *g) {
    if (field_near(g) == NEAR_NONE)
        return false;
    g->note_timer = 12;
    const World *w = &g->world;
    const Destination *d = w->place;
    int stage = field_stage(g);
    if (stage == 1) {
        float yaw = clue_yaw(w, 1);
        if (d->skill == ALIGN && cosf(g->yaw - yaw) < .935f) {
            snprintf(g->field_note, sizeof g->field_note,
                     "Sight %s along the two markers, then read them.", compass_bearing(yaw));
            return false;
        }
        if (d->skill == DUSK && (g->hour < 16.5f || g->hour > 19.5f)) {
            snprintf(g->field_note, sizeof g->field_note,
                     "The dial is dark. Read it in evening light, from 16:30 to 19:30.");
            return false;
        }
        if (d->skill == ECHO) {
            int hit = 0;
            float best = 1e9f;
            for (int i = 0; i < 3; i++) {
                float dist = distance_to(g, field_pipe(w, i));
                if (dist < best) {
                    best = dist;
                    hit = i;
                }
            }
            if (best > 2.25f) {
                snprintf(g->field_note, sizeof g->field_note,
                         "Stand beside a pipe to sound it. Start with the shortest.");
                return false;
            }
            if (hit != g->chimes) {
                g->chimes = hit == 0 ? 1 : 0;
                snprintf(g->field_note, sizeof g->field_note,
                         "The notes fade. Sound the pipes from shortest to tallest.");
                return false;
            }
            if (++g->chimes < 3) {
                snprintf(g->field_note, sizeof g->field_note,
                         "%s note rings. Try the next taller pipe.", hit == 0 ? "Low" : "Middle");
                return false;
            }
        }
    }
    g->notes += UINT32_C(1) << (2 * w->destination);
    field_refresh(g);
    return true;
}
bool footing_recover(Game *g, float dt) {
    g->warning_timer = fmaxf(0, g->warning_timer - dt);
    if (g->fall_time <= 0)
        return false;
    float elapsed = 1.15f - g->fall_time;
    g->fall_y -= dt * (3 + elapsed * (g->world.profile == LUNAR ? 5 : 18));
    g->fall_time = fmaxf(0, g->fall_time - dt);
    if (g->fall_time == 0) {
        world_position(g, g->safe_x, g->safe_z);
        g->exposed = false;
        g->warning_timer = 4;
        g->falls++;
    }
    return true;
}
void footing_move(Game *g, float old_x, float old_z, float dt) {
    if (g->cabin.inside ||
        (g->cabin.present && hypotf(g->x - g->cabin.x, g->z - g->cabin.z) < 10)) {
        g->exposed = false;
        return;
    }
    float dx = g->x - old_x, dz = g->z - old_z, n = hypotf(dx, dz);
    if (n < .0001f)
        return;
    float h = height_at(&g->world, g->x, g->z);
    float tread = trail_distance(&g->world, g->x, g->z);
    if (tread < 2.2f) {
        g->exposed = false;
        if (hypot(g->world.ox + (double)g->x - g->safe_x, g->world.oz + (double)g->z - g->safe_z) >
            7) {
            g->safe_x = g->world.ox + (double)g->x;
            g->safe_z = g->world.oz + (double)g->z;
        }
        return;
    }
    float ahead = height_at(&g->world, g->x + dx / n * 4, g->z + dz / n * 4);
    g->footing_timer -= dt;
    if (g->footing_timer <= 0) {
        g->footing_timer = .15f;
        float low =
            fminf(height_at(&g->world, g->x + 4, g->z), height_at(&g->world, g->x - 4, g->z));
        low = fminf(
            low, fminf(height_at(&g->world, g->x, g->z + 4), height_at(&g->world, g->x, g->z - 4)));
        g->exposed = h - fminf(low, ahead) > 4.8f;
    }
    if (h - ahead > 6 && height_at(&g->world, old_x, old_z) - h > 1.15f * n) {
        g->exposed = true;
        g->fall_time = 1.15f;
        g->fall_y = h;
    }
}
