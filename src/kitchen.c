#include "mountain.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATCH_GRID 24
static const Recipe *recipe_at(const Game *g, int id) {
    const Cookbook *b = g->rules->cookbook;
    return b && id > 0 && id <= b->recipe_max && b->recipes[id].name[0] ? &b->recipes[id] : NULL;
}
static bool hearth(const Game *g) { return g->cabin.inside && !g->cabin.room; }
bool kitchen_deck(const Game *g) {
    if (!g->cabin.inside || !cabin_layout(g)->balcony)
        return false;
    Cabin frame = cabin_frame(g);
    return cabin_local(&frame, g->x, g->z).x < -cabin_layout(g)->width;
}
static bool claimed(const Game *g, int64_t x, int64_t z) {
    for (unsigned i = 0; i < g->pantry.picks; i++) {
        const Harvest *h = &g->pantry.picked[i];
        if (h->x == x && h->z == z && h->region == (int)g->world.profile &&
            h->seed == g->world.seed && g->pantry.distance - h->at < 1000)
            return true;
    }
    return false;
}
static bool patch(const Game *g, int64_t cx, int64_t cz, ForagePatch *p, bool near_only) {
    const World *w = &g->world;
    const Cookbook *book = g->rules->cookbook;
    int region = w->profile, count = book->regional_count[region];
    if (!count)
        return false;
    bool start = cx == 0 && cz == 0;
    uint32_t h = cell_hash(cx, cz, w->seed ^ 0x4b9e126u);
    float x = (float)((double)cx * PATCH_GRID - w->ox) + (random2(cx, cz, w->seed + 81) - .5f) * 13;
    float z = (float)((double)cz * PATCH_GRID - w->oz) + (random2(cx, cz, w->seed + 23) - .5f) * 13;
    if (start) {
        float yaw = trail_bearing(w);
        x = (float)(7 * sin(yaw) - w->ox);
        z = (float)(-7 * cos(yaw) - w->oz);
    }
    if (near_only && hypotf(x - g->x, z - g->z) >= 2.65f)
        return false;
    if (g->cabin.present) {
        Vec local = cabin_local(&g->cabin, x, z);
        if (fabsf(local.x) < g->rules->layout[0].width + 1 &&
            fabsf(local.z) < g->rules->layout[0].depth + 1)
            return false;
    }
    float y = height_at(w, x, z), water = water_at(w, x, z);
    if (!start && (y < water + .18f || fabsf(height_at(w, x + 1, z) - y) > 1.1f ||
                   fabsf(height_at(w, x, z + 1) - y) > 1.1f))
        return false;
    if (!start && region <= SURVEYED && y > 152)
        return false;
    double wx = w->ox + (double)x, wz = w->oz + (double)z;
    float cover = noise2(wx / 68, wz / 68, w->seed + 9201);
    ForageHabitat niche = y - water < 8  ? FORAGE_WATER
                          : cover > .53f ? FORAGE_SHELTER
                                         : FORAGE_OPEN;
    if (region == DUNES && oasis_distance(w, wx, wz) < 64)
        niche = FORAGE_WATER;
    /* Water in a lunar relay is in its tanks; cultivated beds follow the relay layout. */
    if (region == LUNAR)
        niche = cover < .4f ? FORAGE_WATER : cover < .6f ? FORAGE_OPEN : FORAGE_SHELTER;
    int choice = start ? 0 : (int)((h >> 8) % (unsigned)count);
    if (!start && h % 8 != 0) {
        /* Walk the local catalogue from a seeded offset. Near banks and in sheltered
           clusters the preferred habitat wins; scattered plants can occur between them. */
        for (int n = 0; n < count; n++) {
            int k = (choice + n) % count;
            if (book->ingredients[book->regional[region][k]].niche == niche) {
                choice = k;
                break;
            }
        }
    }
    int id = book->regional[region][choice];
    if (!start && region == DUNES &&
        oasis_distance(w, w->ox + (double)x, w->oz + (double)z) > 105) {
        if (h % 4 != 0)
            return false;
        for (int i = 0; i < count; i++) {
            int candidate = book->regional[region][i];
            if (book->ingredients[candidate].form == PATCH_CAP) {
                id = candidate;
                break;
            }
        }
    }
    if (!start && book->ingredients[id].form == PATCH_WEED && y - water > 8)
        return false;
    *p = (ForagePatch){cx, cz, id, x, z, !claimed(g, cx, cz)};
    return true;
}
int forage_patches(const Game *g, ForagePatch *out, int radius) {
    if (!g->rules->cookbook || g->cabin.inside || radius < 0 || radius > 2)
        return 0;
    int64_t cx = (int64_t)floor((g->world.ox + (double)g->x) / PATCH_GRID + .5);
    int64_t cz = (int64_t)floor((g->world.oz + (double)g->z) / PATCH_GRID + .5);
    int n = 0;
    for (int j = -radius; j <= radius; j++)
        for (int i = -radius; i <= radius; i++)
            if (patch(g, cx + i, cz + j, &out[n], false))
                n++;
    return n;
}
int forage_near(const Game *g, ForagePatch *out) {
    if (!g->rules->cookbook || g->cabin.inside)
        return 0;
    int64_t cx = (int64_t)floor((g->world.ox + (double)g->x) / PATCH_GRID + .5);
    int64_t cz = (int64_t)floor((g->world.oz + (double)g->z) / PATCH_GRID + .5);
    return patch(g, cx, cz, out, true) && out->ripe ? out->item : 0;
}
static bool add_stock(Game *g, int id, int amount) {
    if (g->pantry.stock[id] > UINT16_MAX - amount)
        return false;
    g->pantry.stock[id] += (uint16_t)amount;
    g->pantry.found[id] = 1;
    g->pantry.revision++;
    snprintf(g->kitchen_note, sizeof g->kitchen_note, "Gathered %s (+%d)",
             g->rules->cookbook->ingredients[id].name, amount);
    return true;
}
bool kitchen_gather(Game *g) {
    ForagePatch p;
    if (!forage_near(g, &p) || g->fall_time > 0)
        return false;
    Pantry *b = &g->pantry;
    unsigned j = 0;
    for (unsigned i = 0; i < b->picks; i++)
        if (b->distance - b->picked[i].at < 1000)
            b->picked[j++] = b->picked[i];
    b->picks = j;
    if (j == PICKED_PATCHES ||
        !add_stock(g, p.item, 2 + (g->rules->cookbook->ingredients[p.item].season == g->season)))
        return false;
    b->picked[b->picks++] = (Harvest){g->world.seed, g->world.profile, p.x, p.z, b->distance};
    return true;
}
bool kitchen_ready(const Game *g, int id) {
    const Recipe *r = recipe_at(g, id);
    if (!r)
        return false;
    for (int i = 0; i < 3 && r->need[i]; i++)
        if (!g->pantry.stock[r->need[i]])
            return false;
    return true;
}
bool kitchen_matches(const Game *g, int id, RecipeShelf shelf) {
    const Recipe *r = recipe_at(g, id);
    if (!r)
        return false;
    switch (shelf) {
    case SHELF_LOCAL:
        return r->region == (int)g->world.profile;
    case SHELF_READY:
        return kitchen_ready(g, id);
    case SHELF_DRINKS:
        return kitchen_beverage(r->serving);
    case SHELF_TRAVEL:
        return r->region < 0;
    default:
        return true;
    }
}
bool kitchen_select(Game *g, int id) {
    if (!recipe_at(g, id))
        return false;
    g->selection = id;
    return true;
}
static void turn_page(Game *g, int step) {
    int max = g->rules->cookbook->recipe_max, id = g->selection;
    for (int n = 0; n < max; n++) {
        id = 1 + (id - 1 + max + step) % max;
        if (kitchen_matches(g, id, g->shelf)) {
            g->selection = id;
            return;
        }
    }
    g->selection = 0;
}
void kitchen_details(const Game *g, int id, char *dst, size_t size) {
    if (!size)
        return;
    *dst = 0;
    const Recipe *r = recipe_at(g, id);
    if (!r)
        return;
    for (int j = 0; j < 3 && r->need[j]; j++) {
        size_t used = strlen(dst);
        int key = r->need[j];
        snprintf(dst + used, size - used, "%s%s (%u)", j ? ", " : "",
                 g->rules->cookbook->ingredients[key].name, (unsigned)g->pantry.stock[key]);
    }
    if (*r->staples) {
        size_t used = strlen(dst);
        snprintf(dst + used, size - used, "%s%s", used ? " + " : "", r->staples);
    }
}
bool kitchen_pack(Game *g, int id) {
    const Recipe *r = recipe_at(g, id);
    if (!r || !g->pantry.portions[id])
        return false;
    if (kitchen_beverage(r->serving))
        g->pantry.drink = id;
    else
        g->pantry.food = id;
    g->pantry.revision++;
    snprintf(g->kitchen_note, sizeof g->kitchen_note, "%s packed for the table", r->name);
    return true;
}
static bool cook(Game *g, int id) {
    const Recipe *r = recipe_at(g, id);
    Pantry *p = &g->pantry;
    if (!r || !hearth(g)) {
        snprintf(g->kitchen_note, sizeof g->kitchen_note, "Cook in the Hearth Room kitchen");
        return false;
    }
    if (!kitchen_ready(g, id)) {
        snprintf(g->kitchen_note, sizeof g->kitchen_note,
                 "Gather the missing ingredients or pin this recipe");
        return false;
    }
    unsigned yield = r->serving == DISH_COOKIES ? 3 : kitchen_beverage(r->serving) ? 2 : 1;
    if (p->portions[id] > UINT16_MAX - yield)
        return false;
    for (int i = 0; i < 3 && r->need[i]; i++)
        p->stock[r->need[i]]--;
    p->portions[id] += (uint16_t)yield;
    p->learned[id] = 1;
    kitchen_pack(g, id);
    snprintf(g->kitchen_note, sizeof g->kitchen_note, "%s ready / %u serving%s on your tray",
             r->name, yield, yield == 1 ? "" : "s");
    return true;
}
bool kitchen_mix(Game *g, const char *method, const int *items, int count) {
    if (!hearth(g) || !method || !items || count < 1 || count > 3)
        return false;
    const Cookbook *b = g->rules->cookbook;
    if (!b)
        return false;
    bool known = false;
    for (int j = 0; j < count; j++) {
        if (items[j] < 1 || items[j] > b->item_max || !g->pantry.stock[items[j]])
            return false;
        for (int k = 0; k < j; k++)
            if (items[k] == items[j])
                return false;
    }
    for (int i = 1; i <= b->recipe_max; i++) {
        const Recipe *r = &b->recipes[i];
        if (strcmp(method, r->method))
            continue;
        known = true;
        int n = 0, hits = 0;
        while (n < 3 && r->need[n]) {
            for (int k = 0; k < count; k++)
                hits += r->need[n] == items[k];
            n++;
        }
        if (n == count && hits == count) {
            g->selection = i;
            return cook(g, i);
        }
    }
    if (!known)
        return false;
    bool drink = !strcmp(method, "Steep") || !strcmp(method, "Brew");
    int *portions = drink ? &g->pantry.custom_drink : &g->pantry.custom_food;
    if (*portions == UINT16_MAX)
        return false;
    for (int j = 0; j < count; j++)
        g->pantry.stock[items[j]]--;
    (*portions)++;
    if (drink)
        g->pantry.drink = -1;
    else
        g->pantry.food = -1;
    g->pantry.revision++;
    snprintf(g->kitchen_note, sizeof g->kitchen_note, "Packed a field %s with %s",
             drink ? "infusion" : "bowl", b->ingredients[items[0]].name);
    return true;
}
static bool taste(Game *g, bool drink) {
    Pantry *p = &g->pantry;
    int *slot = drink ? &p->drink : &p->food;
    if (!g->seated || !*slot)
        return false;
    const Recipe *r = recipe_at(g, *slot);
    if (r) {
        if (!p->portions[*slot])
            return false;
        if (!--p->portions[*slot])
            *slot = 0;
    } else {
        if (*slot != -1)
            return false;
        int *n = drink ? &p->custom_drink : &p->custom_food;
        if (!*n)
            return false;
        if (!--*n)
            *slot = 0;
    }
    snprintf(g->kitchen_note, sizeof g->kitchen_note, "%s %s", drink ? "Sipped" : "Tasted",
             r       ? r->name
             : drink ? "field infusion"
                     : "field bowl");
    p->revision++;
    return true;
}
bool kitchen_photo(Game *g) {
    if (!g->seated)
        return false;
    unsigned bit = 1u << g->world.profile;
    int house = g->rules->cookbook->house[g->world.profile];
    if (!house || (g->pantry.food != house && g->pantry.drink != house) ||
        (g->pantry.tableware & bit))
        return false;
    g->pantry.tableware |= bit;
    g->pantry.setting = bit;
    g->pantry.revision++;
    return true;
}
static void change_combination(Game *g, int step) {
    const Cookbook *b = g->rules->cookbook;
    if (!g->experiment.field) {
        g->experiment.method = (g->experiment.method + step + b->method_count) % b->method_count;
        return;
    }
    int *id = &g->experiment.items[g->experiment.field - 1];
    do {
        *id = (*id + step + b->item_max + 1) % (b->item_max + 1);
    } while (*id && (!b->ingredients[*id].name[0] || !g->pantry.stock[*id]));
}
static bool try_combination(Game *g) {
    const Cookbook *b = g->rules->cookbook;
    int items[3], count = 0;
    for (int i = 0; i < 3; i++)
        if (g->experiment.items[i])
            items[count++] = g->experiment.items[i];
    if (kitchen_mix(g, b->recipes[b->methods[g->experiment.method]].method, items, count))
        return true;
    snprintf(g->kitchen_note, sizeof g->kitchen_note,
             "In the kitchen, choose different ingredients from your basket");
    return false;
}
bool kitchen_action(Game *g, Action a) {
    Cookbook *b = g->rules->cookbook;
    if (!b)
        return false;
    Pantry *p = &g->pantry;
    switch (a) {
    case ACT_COOKBOOK:
        g->cookbook_open = !g->cookbook_open;
        g->experiment.open = false;
        if (!g->selection)
            g->selection = b->coffee;
        return true;
    case ACT_EXPERIMENT:
        if (!g->cookbook_open)
            return false;
        g->experiment.open = !g->experiment.open;
        return true;
    case ACT_COOK:
        return g->experiment.open ? try_combination(g) : cook(g, g->selection);
    case ACT_RECIPE_NEXT:
    case ACT_RECIPE_PREV:
        if (g->experiment.open)
            change_combination(g, a == ACT_RECIPE_NEXT ? 1 : -1);
        else
            turn_page(g, a == ACT_RECIPE_NEXT ? 1 : -1);
        return true;
    case ACT_SHELF:
        if (g->experiment.open) {
            g->experiment.field = (g->experiment.field + 1) % 4;
            return true;
        }
        g->shelf = (RecipeShelf)((g->shelf + 1) % SHELF_COUNT);
        g->selection = 0;
        turn_page(g, 1);
        return true;
    case ACT_PIN:
        if (!recipe_at(g, g->selection))
            return false;
        p->pin = p->pin == g->selection ? 0 : g->selection;
        p->revision++;
        return true;
    case ACT_GROW: {
        if (!g->cabin.inside || cabin_room(&g->book, g->cabin.room)->kind != 2)
            return false;
        const Recipe *r = recipe_at(g, p->pin);
        int best = 0;
        if (r)
            for (int i = 0; i < 3 && r->need[i]; i++) {
                int id = r->need[i];
                if (b->ingredients[id].region == (int)g->world.profile &&
                    (!best || p->stock[id] < p->stock[best]))
                    best = id;
            }
        if (!best) {
            snprintf(g->kitchen_note, sizeof g->kitchen_note,
                     "Pin a recipe with an ingredient from this destination");
            return false;
        }
        return add_stock(g, best, 1);
    }
    case ACT_TRAY:
        if (!hearth(g) || (!p->food && !p->drink))
            return false;
        if (!cabin_to_balcony(g))
            return false;
        g->cookbook_open = false;
        g->experiment.open = false;
        return true;
    case ACT_SIT: {
        if (!kitchen_deck(g))
            return false;
        if (!g->seated) {
            Cabin f = cabin_frame(g);
            Vec seat = cabin_world(&f, -cabin_layout(g)->width - .92f, 0, 1.78f);
            g->x = seat.x;
            g->z = seat.z;
            g->yaw = f.angle - PI / 2;
            g->pitch = -.095f;
        }
        g->seated = !g->seated;
        g->cookbook_open = false;
        g->experiment.open = false;
        return true;
    }
    case ACT_SIP:
    case ACT_BITE:
        return taste(g, a == ACT_SIP);
    case ACT_REST:
        if (!g->cabin.inside)
            return false;
        p->picks = 0;
        p->revision++;
        return true;
    case ACT_SETTING: {
        unsigned bit = p->setting ? p->setting << 1 : 1;
        while (bit && !(p->tableware & bit))
            bit <<= 1;
        p->setting = bit;
        p->revision++;
        return true;
    }
    default:
        return false;
    }
}
static unsigned serving_shape(const Game *g, int id, bool cup) {
    const Recipe *r = recipe_at(g, id);
    return r ? (unsigned)r->serving : cup ? DISH_CUP : DISH_BOWL;
}
void kitchen_scenery(Mesh *m, const Game *g) {
    const Cookbook *b = g->rules->cookbook;
    if (!b || !b->models)
        return;
    if (!g->cabin.inside) {
        ForagePatch patches[25];
        int n = forage_patches(g, patches, 2);
        for (int i = 0; i < n; i++) {
            ForagePatch v = patches[i];
            float y = fmaxf(height_at(&g->world, v.px, v.pz), water_at(&g->world, v.px, v.pz));
            Pose pose = {{v.px, y, v.pz}, random2(v.x, v.z, g->world.seed) * PI};
            float scale = v.ripe ? 1.f : .29f;
            instance(m, &b->models[b->ingredients[v.item].form], pose, scale);
        }
    } else if (cabin_layout(g)->balcony) {
        Cabin f = cabin_frame(g);
        float x = -cabin_layout(g)->width - 2.13f;
        if (g->pantry.drink)
            instance(m, &b->models[serving_shape(g, g->pantry.drink, true)],
                     (Pose){cabin_world(&f, x, .752f, 1.49f), f.angle - PI / 2}, 1);
        if (g->pantry.food)
            instance(m, &b->models[serving_shape(g, g->pantry.food, false)],
                     (Pose){cabin_world(&f, x, .752f, 2.03f), f.angle}, 1);
        if (g->pantry.setting) {
            Color dye = rgb(0x665847u + (g->pantry.setting % 233u) * 0x2639u);
            box(m, (Pose){cabin_world(&f, x, .749f, 1.78f), f.angle}, (Vec){0},
                (Vec){.60f, .004f, .99f}, dye);
        }
    }
}

/* Sparse, versioned records keep a new pantry small and survive additions to the cookbook.
   Read a whole snapshot before replacing the active pantry. */
char *pantry_encode(const Pantry *p) {
    const size_t cap = 256 + PANTRY_ITEMS * 32 + COOKBOOK_RECIPES * 32 + PICKED_PATCHES * 108;
    char *s = malloc(cap);
    if (!s)
        return NULL;
    size_t at =
        (size_t)snprintf(s, cap, "KIT1 %d %d %d %u %u %d %d %.5f\n", p->drink, p->food, p->pin,
                         p->tableware, p->setting, p->custom_food, p->custom_drink, p->distance);
    for (int i = 1; i <= PANTRY_ITEMS; i++)
        if (p->found[i] || p->stock[i])
            at += (size_t)snprintf(s + at, cap - at, "I %d %u\n", i, (unsigned)p->stock[i]);
    for (int i = 1; i <= COOKBOOK_RECIPES; i++)
        if (p->learned[i] || p->portions[i])
            at += (size_t)snprintf(s + at, cap - at, "R %d %u\n", i, (unsigned)p->portions[i]);
    for (unsigned i = 0; i < p->picks; i++) {
        const Harvest *h = &p->picked[i];
        at += (size_t)snprintf(s + at, cap - at, "H %u %d %lld %lld %.5f\n", (unsigned)h->seed,
                               h->region, (long long)h->x, (long long)h->z, h->at);
    }
    memcpy(s + at, "END", 4);
    return s;
}
bool pantry_decode(Pantry *out, const char *s) {
    Pantry p = {0};
    int n = 0;
    if (!s ||
        sscanf(s, "KIT1 %d %d %d %u %u %d %d %lf%n", &p.drink, &p.food, &p.pin, &p.tableware,
               &p.setting, &p.custom_food, &p.custom_drink, &p.distance, &n) != 8 ||
        s[n] != '\n' || p.drink < -1 || p.drink > COOKBOOK_RECIPES || p.food < -1 ||
        p.food > COOKBOOK_RECIPES || p.pin < 0 || p.pin > COOKBOOK_RECIPES || p.custom_food < 0 ||
        p.custom_food > UINT16_MAX || p.custom_drink < 0 || p.custom_drink > UINT16_MAX ||
        (p.setting & ~p.tableware) || !isfinite(p.distance) || p.distance < 0 || p.distance > 1.e16)
        return false;
    s += n + 1;
    while (strcmp(s, "END")) {
        int id;
        unsigned qty;
        n = 0;
        if (*s == 'I' || *s == 'R') {
            if (sscanf(s + 1, " %d %u%n", &id, &qty, &n) != 2 || id < 1 || qty > UINT16_MAX)
                return false;
            if (*s == 'I') {
                if (id > PANTRY_ITEMS || p.found[id])
                    return false;
                p.found[id] = 1;
                p.stock[id] = (uint16_t)qty;
            } else {
                if (id > COOKBOOK_RECIPES || p.learned[id])
                    return false;
                p.learned[id] = 1;
                p.portions[id] = (uint16_t)qty;
            }
            n++;
        } else if (*s == 'H') {
            unsigned seed;
            int region;
            long long x, z;
            double age;
            if (p.picks >= PICKED_PATCHES ||
                sscanf(s, "H %u %d %lld %lld %lf%n", &seed, &region, &x, &z, &age, &n) != 5 ||
                region < 0 || region >= DEST_LIMIT || !isfinite(age) || age < 0 || age > p.distance)
                return false;
            p.picked[p.picks++] = (Harvest){seed, region, (int64_t)x, (int64_t)z, age};
        } else
            return false;
        if (s[n] != '\n')
            return false;
        s += n + 1;
    }
    if ((p.food > 0 && !p.portions[p.food]) || (p.drink > 0 && !p.portions[p.drink]) ||
        (p.food < 0 && !p.custom_food) || (p.drink < 0 && !p.custom_drink))
        return false;
    *out = p;
    return true;
}
