#include "mountain.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const Shelter *destination_shelter(const Game *g) {
    return g->world.place ? &g->world.place->shelter : &g->rules->default_shelter;
}
const LookoutEvent *lookout_event(const Game *g) {
    static const LookoutEvent calm = {.name = "Calm", .kind = FX_CALM};
    return g->cabin.inside && g->book.weather ? &destination_shelter(g)->event[g->book.weather - 1]
                                              : &calm;
}
bool event_thunders(const Game *g) {
    EventKind kind = lookout_event(g)->kind;
    return kind == FX_LIGHTNING || kind == FX_DRYLIGHT;
}
const Room *cabin_room(const RoomBook *b, uint32_t id) {
    return id ? &b->rooms[id - 1] : &b->hearth;
}
static Room *edit_room(RoomBook *b, uint32_t id) { return id ? &b->rooms[id - 1] : &b->hearth; }
const Layout *cabin_layout(const Game *g) {
    return &g->rules->layout[cabin_room(&g->book, g->cabin.room)->kind];
}
Cabin cabin_frame(const Game *g) {
    Cabin frame = g->cabin;
    frame.angle += cabin_room(&g->book, frame.room)->turn * (PI / 2);
    return frame;
}
/* The room on the parent side owns the shared door, including when operated from its child. */
unsigned cabin_doors(const Game *g, uint32_t id) {
    const Room *r = cabin_room(&g->book, id);
    bool front =
        id ? (cabin_room(&g->book, r->parent)->doors & (1u << r->incoming)) != 0 : g->cabin.open;
    return r->doors | (unsigned)front;
}
uint32_t cabin_link(const RoomBook *b, uint32_t id, Wall wall) {
    const Room *r = cabin_room(b, id);
    if (wall == FRONT)
        return id ? r->parent : CABIN_NO_ROOM;
    if (wall == REAR || wall == RIGHT_WALL) {
        uint32_t child = r->child[wall - REAR];
        if (child)
            return child;
    }
    return CABIN_NO_ROOM;
}
const char *cabin_door_name(const Game *g, uint32_t id, Wall wall) {
    uint32_t link = cabin_link(&g->book, id, wall);
    if (link != CABIN_NO_ROOM)
        return g->rules->layout[cabin_room(&g->book, link)->kind].name;
    if (!id && wall <= RIGHT_WALL)
        return wall == FRONT ? "Trail" : g->rules->layout[wall].name;
    if (wall == LEFT_WALL && g->rules->layout[cabin_room(&g->book, id)->kind].balcony)
        return "Balcony";
    return "";
}
static bool append_room(RoomBook *b, uint32_t parent, Wall side, int layouts) {
    if (b->count == b->capacity) {
        size_t cap = b->capacity ? (size_t)b->capacity * 2 : 16;
        if (cap > UINT32_MAX || cap > SIZE_MAX / sizeof(Room) || cap <= b->count)
            return false;
        Room *space = realloc(b->rooms, cap * sizeof *space);
        if (!space)
            return false;
        b->rooms = space;
        b->capacity = (uint32_t)cap;
    }
    Room *from = edit_room(b, parent);
    uint32_t key = hash32(from->key ^ (side == REAR ? 0x9e3779b9u : 0x85ebca6bu));
    /* Number the shallow rooms by doorway path, independent of exploration order.
       Beyond the named plan, the same path continues into repeatable new rooms. */
    unsigned branch = parent ? from->address : 1;
    unsigned address = 2 * branch + (side == RIGHT_WALL);
    if (address > CABIN_LAYOUTS || !branch)
        address = 0;
    unsigned kind;
    if (address && address <= (unsigned)layouts)
        kind = address - 1;
    else {
        kind = 1 + key % (layouts - 2);
        if (kind >= from->kind)
            kind++;
    }
    Room next = {.parent = parent,
                 .incoming = side,
                 .key = key,
                 .depth = from->depth + 1,
                 .address = address,
                 .kind = kind,
                 .turn = (from->turn + (side == RIGHT_WALL)) % 4};
    b->rooms[b->count++] = next;
    from->child[side - REAR] = b->count;
    return true;
}
static Wall closest_wall(Vec p, const Layout *l) {
    if (l->width - fabsf(p.x) < l->depth - fabsf(p.z))
        return p.x > 0 ? RIGHT_WALL : LEFT_WALL;
    return p.z > 0 ? FRONT : REAR;
}
typedef struct {
    Wall wall;
    int window;
    bool at_door;
} Handle;
static Handle handle(const Game *g) {
    Cabin frame = cabin_frame(g);
    Vec p = cabin_local(&frame, g->x, g->z);
    const Layout *l = cabin_layout(g);
    Wall side = closest_wall(p, l);
    bool end = side <= REAR;
    float distance = fabsf((end ? l->depth : l->width) - fabsf(end ? p.z : p.x));
    float t = end ? p.x : p.z, half = end ? l->width : l->depth;
    Handle h = {.wall = side, .window = -1};
    if (distance <= 2.15f) {
        h.at_door = fabsf(t) <= 1.05f && (side != LEFT_WALL || l->balcony);
        if (!h.at_door && fabsf(fabsf(t) - .60f * half) <= fminf(3.6f, half * .26f) + .18f)
            h.window = 2 * side + (t > 0);
    }
    return h;
}
int cabin_window(const Game *g) { return g->cabin.inside ? handle(g).window : -1; }
int cabin_near_door(const Game *g) {
    if (!g->cabin.inside || g->seated)
        return -1;
    Handle h = handle(g);
    return h.at_door ? (int)h.wall : -1;
}
bool cabin_on_balcony(const Game *g) {
    if (!g->cabin.inside || !cabin_layout(g)->balcony)
        return false;
    Cabin frame = cabin_frame(g);
    return cabin_local(&frame, g->x, g->z).x < -cabin_layout(g)->width;
}
int cabin_balcony_step(const Game *g) {
    if (!g->cabin.inside || cabin_on_balcony(g))
        return -1;
    if (cabin_layout(g)->balcony)
        return LEFT_WALL;
    return g->cabin.room ? FRONT : RIGHT_WALL;
}
float cabin_door_bearing(const Game *g, Wall wall) {
    Cabin frame = cabin_frame(g);
    const Layout *l = cabin_layout(g);
    Vec p = cabin_local(&frame, g->x, g->z);
    float x = wall == RIGHT_WALL ? l->width : wall == LEFT_WALL ? -l->width : 0;
    float z = wall == FRONT ? l->depth : wall == REAR ? -l->depth : 0;
    return atan2f(x - p.x, p.z - z);
}
bool cabin_map_action(Game *g, Action action) {
    if (!g->cabin.inside)
        return false;
    switch (action) {
    case ACT_CABIN_MAP:
        g->cabin_map_open = !g->cabin_map_open;
        g->map_room = g->cabin.room;
        g->cookbook_open = g->experiment.open = false;
        return true;
    case ACT_BALCONY_GUIDE:
        g->balcony_guide = !g->balcony_guide;
        return true;
    case ACT_MAP_HEARTH:
        if (!g->cabin_map_open)
            return false;
        g->map_room = 0;
        return true;
    case ACT_MAP_FRONT:
    case ACT_MAP_REAR:
    case ACT_MAP_RIGHT: {
        if (!g->cabin_map_open)
            return false;
        uint32_t link = cabin_link(&g->book, g->map_room, (Wall)(action - ACT_MAP_FRONT));
        if (link == CABIN_NO_ROOM)
            return false;
        g->map_room = link;
        return true;
    }
    default:
        return false;
    }
}
Interaction cabin_near(const Game *g) {
    Handle h = handle(g);
    const Room *r = cabin_room(&g->book, g->cabin.room);
    if (h.window >= 0)
        return r->panes & (1u << h.window) ? NEAR_PANE_OPEN : NEAR_PANE_CLOSED;
    if (!h.at_door)
        return NEAR_NONE;
    bool open = (cabin_doors(g, g->cabin.room) & (1u << h.wall)) != 0;
    if (h.wall == FRONT && !g->cabin.room)
        return open ? NEAR_EXIT : NEAR_OPEN;
    if (h.wall == LEFT_WALL)
        return open ? NEAR_BALCONY : NEAR_BALCONY_DOOR;
    return !open ? NEAR_ROOM_DOOR : h.wall == FRONT ? NEAR_BACK : NEAR_NEXT;
}
static void put(Game *g, Cabin frame, float x, float z) {
    Vec v = cabin_world(&frame, x, 0, z);
    g->x = v.x;
    g->z = v.z;
}
void cabin_home(Game *g) {
    g->cabin.room = 0;
    g->cabin.inside = true;
    put(g, g->cabin, 0, g->rules->layout[0].depth - 1.35f);
    g->yaw = g->cabin.angle + PI;
    g->pitch = 0;
}
static void cross(Game *g, Wall side) {
    Cabin *c = &g->cabin;
    RoomBook *b = &g->book;
    Cabin frame = cabin_frame(g);
    const Room *from = cabin_room(b, c->room);
    Vec p = cabin_local(&frame, g->x, g->z);
    if (side == FRONT && !c->room) {
        c->inside = false;
        g->cabin_map_open = false;
        put(g, frame, 0, g->rules->layout[0].depth + 1.5f);
        return;
    }
    if (side == LEFT_WALL) {
        float edge = cabin_layout(g)->width;
        put(g, frame, p.x < -edge ? -edge + .8f : -edge - 1.15f, 0);
        return;
    }
    Wall arrival = side == FRONT ? from->incoming : FRONT;
    c->room = side == FRONT ? from->parent : from->child[side - REAR];
    frame = cabin_frame(g);
    const Layout *l = cabin_layout(g);
    /* Enter at the matching jamb while retaining the player's world-facing direction. */
    float offset = clampf(side <= REAR ? p.x : p.z, -.65f, .65f);
    if (arrival == RIGHT_WALL)
        put(g, frame, l->width - .85f, offset);
    else
        put(g, frame, offset, (arrival == FRONT ? 1 : -1) * (l->depth - .85f));
}
bool cabin_use(Game *g, Action action) {
    Handle h = handle(g);
    uint32_t id = g->cabin.room;
    RoomBook *b = &g->book;
    Room *r = edit_room(b, id);
    if (h.window >= 0 && action != ACT_DOOR) {
        unsigned char *mask = action == ACT_BLINDS ? &r->blinds : &r->panes;
        *mask ^= 1u << h.window;
    } else if (h.at_door && action != ACT_BLINDS) {
        if (action == ACT_USE && (cabin_doors(g, id) & (1u << h.wall))) {
            cross(g, h.wall);
            return true;
        }
        if (h.wall == FRONT && !id) {
            g->cabin.open = !g->cabin.open;
            return true;
        }
        Wall side = h.wall;
        if (side == FRONT) {
            side = r->incoming;
            r = edit_room(b, r->parent);
        } else if (side != LEFT_WALL && !r->child[side - REAR]) {
            if (!append_room(b, id, side, g->rules->layouts))
                return false;
            r = edit_room(b, id);
        }
        /* From the deck the player can always reopen the same balcony door. */
        r->doors ^= 1u << side;
    } else
        return false;
    b->revision++;
    return true;
}
/* Carry the tray through the same garden and balcony doors used on foot. */
bool cabin_to_balcony(Game *g) {
    if (!g->cabin.inside || g->cabin.room || !g->rules->layout[2].balcony)
        return false;
    Cabin f = cabin_frame(g);
    put(g, f, cabin_layout(g)->width - .8f, 0);
    if (!(cabin_doors(g, 0) & (1u << RIGHT_WALL)) && !cabin_use(g, ACT_USE))
        return false;
    if (!cabin_use(g, ACT_USE))
        return false;
    f = cabin_frame(g);
    put(g, f, -cabin_layout(g)->width + .8f, 0);
    if (!(cabin_doors(g, g->cabin.room) & (1u << LEFT_WALL)) && !cabin_use(g, ACT_USE))
        return false;
    if (!cabin_use(g, ACT_USE))
        return false;
    put(g, f, -cabin_layout(g)->width - 1.15f, 1.78f);
    g->yaw = f.angle - PI / 2;
    g->pitch = -.12f;
    return true;
}
void cabin_walk(Game *g, Vec target) {
    Cabin frame = cabin_frame(g);
    Vec p = cabin_local(&frame, target.x, target.z), old = cabin_local(&frame, g->x, g->z);
    const Layout *l = cabin_layout(g);
    unsigned doors = cabin_doors(g, g->cabin.room);
    Wall side = closest_wall(p, l);
    bool end = side <= REAR;
    float along = end ? p.x : p.z, normal = end ? fabsf(p.z) : fabsf(p.x);
    bool threshold = side != LEFT_WALL && fabsf(along) < .78f && (doors & (1u << side));
    if (threshold && normal >= (end ? l->depth : l->width) - .35f) {
        cross(g, side);
        return;
    }
    bool deck = l->balcony && old.x < -l->width;
    bool passage = l->balcony && (doors & (1u << LEFT_WALL)) && fabsf(p.z) < .78f;
    if (deck) {
        p.x = clampf(p.x, -l->width - 2.95f, passage ? l->width - .35f : -l->width - .25f);
        p.z = clampf(p.z, -l->depth * .60f + .2f, l->depth * .60f - .2f);
    } else {
        p.x = clampf(p.x, -l->width - (passage ? 2.95f : -.35f), l->width - .35f);
        p.z = clampf(p.z, -l->depth + .35f, l->depth - .35f);
    }
    put(g, frame, p.x, p.z);
}
/* CAB2 expands the layout ID to a full byte. Depth and links are derived from parent edges. */
char *cabin_encode(const RoomBook *b) {
    size_t records = (size_t)b->count + 1;
    if (!records || records > (SIZE_MAX - 6) / 16)
        return NULL;
    char *out = malloc(6 + records * 16);
    if (!out)
        return NULL;
    unsigned watch = b->weather | (b->predators << 2) | (b->giants << 3);
    snprintf(out, 6, "CAB2%x", watch);
    for (size_t i = 0; i < records; i++) {
        const Room *r = cabin_room(b, (uint32_t)i);
        snprintf(out + 5 + 16 * i, 17, "%08x%x%02x%x%02x%02x", (unsigned)r->parent, r->incoming,
                 r->kind, r->doors, r->blinds, r->panes);
    }
    return out;
}
static unsigned hex(const char *p, unsigned n) {
    unsigned value = 0;
    while (n--) {
        unsigned char c = (unsigned char)*p++;
        value = value * 16 + (c <= '9' ? c - '0' : c - 'a' + 10);
    }
    return value;
}
bool cabin_decode(RoomBook *b, const char *text, int layouts) {
    if (!text || strncmp(text, "CAB", 3) || (text[3] != '1' && text[3] != '2'))
        return false;
    unsigned kind_digits = text[3] - '0';
    size_t stride = 14 + kind_digits;
    size_t n = strlen(text);
    if (n < 5 + stride || (n - 5) % stride || (n - 5) / stride > UINT32_MAX ||
        strspn(text + 4, "0123456789abcdef") != n - 4)
        return false;
    unsigned watch = hex(text + 4, 1);
    RoomBook next = {
        .weather = watch & 3, .predators = (watch & 4) != 0, .giants = (watch & 8) != 0};
    for (size_t i = 0; i < (n - 5) / stride; i++) {
        const char *p = text + 5 + i * stride;
        const char *flags = p + 9 + kind_digits;
        unsigned parent = hex(p, 8), incoming = hex(p + 8, 1), kind = hex(p + 9, kind_digits),
                 doors = hex(flags, 1);
        if (doors & 1 || (!i && (parent || incoming || kind)) ||
            (i && (parent >= i || incoming < REAR || incoming > RIGHT_WALL || !kind ||
                   kind >= (unsigned)layouts || cabin_room(&next, parent)->child[incoming - REAR] ||
                   !append_room(&next, parent, (Wall)incoming, layouts))))
            goto invalid;
        Room *r = edit_room(&next, (uint32_t)i);
        r->kind = kind;
        r->doors = doors;
        r->blinds = hex(flags + 1, 2);
        r->panes = hex(flags + 3, 2);
    }
    for (uint32_t i = 0; i <= next.count; i++) {
        const Room *r = cabin_room(&next, i);
        if (((r->doors & (1u << REAR)) && !r->child[0]) ||
            ((r->doors & (1u << RIGHT_WALL)) && !r->child[1]))
            goto invalid;
    }
    next.revision = b->revision + 1;
    cabin_forget(b);
    *b = next;
    return true;
invalid:
    cabin_forget(&next);
    return false;
}
void cabin_forget(RoomBook *b) {
    free(b->rooms);
    *b = (RoomBook){0};
}
