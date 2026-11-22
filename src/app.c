#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define PUBLIC EMSCRIPTEN_KEEPALIVE
#else
#define PUBLIC
#endif

static Game game;
static Script scripts;
static SDL_Window *window;
static SDL_GLContext context;
static Display *display;
static char assets[1024];
static bool running = true, help = false, immersive = false, paused = false, sound_on = false;
static float input_f, input_r;
static Uint64 previous;
static SDL_AudioStream *audio;
static Uint8 *track;
static Uint32 track_size, track_pos;
static SDL_AudioSpec track_format;

static SDL_AudioStream *instrument;
static const char *ambience(Profile profile) {
    if (profile <= SURVEYED)
        return "ridge";
    if (profile == COAST || profile == BASALT || profile == KARST)
        return "surf";
    return profile >= LUNAR ? "resonance" : "wind";
}
static void play_cue(const char *name) {
    if (!sound_on)
        return;
    char path[1150];
    snprintf(path, sizeof path, "%s/sound/%s.wav", assets, name);
    SDL_AudioSpec format;
    Uint8 *data = NULL;
    Uint32 bytes = 0;
    if (!SDL_LoadWAV(path, &format, &data, &bytes))
        return;
    if (!instrument)
        instrument =
            SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format, NULL, NULL);
    if (instrument) {
        SDL_ClearAudioStream(instrument);
        SDL_PutAudioStreamData(instrument, data, bytes);
        SDL_ResumeAudioStreamDevice(instrument);
    }
    SDL_free(data);
}
#ifdef __EMSCRIPTEN__
// clang-format off
EM_JS(uint32_t, read_notes, (void), {
    try { return Number(localStorage.getItem("mountain.field-notes.v1")) >>> 0; }
    catch (e) { return 0; }
});
EM_JS(void, write_notes, (uint32_t notes), {
    try { localStorage.setItem("mountain.field-notes.v1", String(notes >>> 0)); }
    catch (e) {}
});
EM_JS(char *, load_cabin, (void), {
    try {
        const value = localStorage.getItem("mountain.cabin.v1");
        if (!value) return 0;
        const size = lengthBytesUTF8(value)+1, p = _malloc(size);
        if (p) stringToUTF8(value,p,size);
        return p;
    } catch(e) {return 0;}
});
EM_JS(bool, store_cabin, (const char *data), {
    try { localStorage.setItem("mountain.cabin.v1",UTF8ToString(data)); return true; }
    catch(e) {return false;}
});
// clang-format on
#else
static char notes_path[1152];
static uint32_t read_notes(void) {
    char *dir = SDL_GetPrefPath("Mountain", "Mountain");
    const char *override = SDL_getenv("MOUNTAIN_FIELD_NOTES");
    if (override)
        snprintf(notes_path, sizeof notes_path, "%s", override);
    else if (dir)
        snprintf(notes_path, sizeof notes_path, "%sfield-notes.v1", dir);
    SDL_free(dir);
    size_t n;
    char *saved = SDL_LoadFile(notes_path, &n);
    unsigned notes = 0;
    if (saved)
        sscanf(saved, "MTN1 %x", &notes);
    SDL_free(saved);
    return notes;
}
static void write_notes(uint32_t notes) {
    char bytes[32], tmp[1200];
    int n = snprintf(bytes, sizeof bytes, "MTN1 %08x\n", (unsigned)notes);
    snprintf(tmp, sizeof tmp, "%s.tmp", notes_path);
    if (SDL_SaveFile(tmp, bytes, n))
        SDL_RenamePath(tmp, notes_path);
}
static char *load_cabin(void) {
    char path[1200];
    snprintf(path, sizeof path, "%s.cabin", notes_path);
    char *file = SDL_LoadFile(path, NULL);
    char *text = NULL;
    if (file) {
        size_t size = strlen(file) + 1;
        text = malloc(size);
        if (text)
            memcpy(text, file, size);
        SDL_free(file);
    }
    return text;
}
static bool store_cabin(const char *data) {
    char path[1200], tmp[1210];
    snprintf(path, sizeof path, "%s.cabin", notes_path);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    return SDL_SaveFile(tmp, data, strlen(data)) && SDL_RenamePath(tmp, path);
}
#endif
static bool cabin_dirty;
static void save_cabin(void) {
    char *text = cabin_encode(&game.book);
    cabin_dirty = !text || !store_cabin(text);
    free(text);
    if (cabin_dirty) {
        snprintf(game.field_note, sizeof game.field_note,
                 "Cabin save unavailable. Your rooms are still here for this visit.");
        game.note_timer = 8;
        SDL_Log("%s", game.field_note);
    }
}
static void SDLCALL fill_audio(void *unused, SDL_AudioStream *stream, int need, int total) {
    (void)unused;
    (void)total;
    while (need > 0 && track_size) {
        Uint32 count = (Uint32)need;
        if (count > track_size - track_pos)
            count = track_size - track_pos;
        if (!SDL_PutAudioStreamData(stream, track + track_pos, (int)count))
            break;
        track_pos = (track_pos + count) % track_size;
        need -= (int)count;
    }
}
PUBLIC int mountain_sound(void) {
    if (!audio) {
        char path[1152];
        snprintf(path, sizeof path, "%s/sound/%s.wav", assets, ambience(game.world.profile));
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
            return 0;
        if (!track && !SDL_LoadWAV(path, &track_format, &track, &track_size))
            return 0;
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &track_format,
                                          fill_audio, NULL);
        if (!audio)
            return 0;
    }
    if (sound_on) {
        if (instrument)
            SDL_ClearAudioStream(instrument);
        SDL_PauseAudioStreamDevice(audio);
        sound_on = false;
    } else
        sound_on = SDL_ResumeAudioStreamDevice(audio);
    return sound_on;
}
static const char *sighting_direction(int slot) {
    static char text[64];
    if (!game.cabin.present)
        return "";
    struct LookoutSite site = game.cabin.lookout[slot];
    if (!site.valid)
        return destination_shelter(&game)->sight[slot].habitat == ON_WATER
                   ? "Summon closer to open water"
                   : "Summon near open ground";
    snprintf(text, sizeof text, "%d m %s", (int)roundf(hypotf(site.dx, site.dz)),
             compass_bearing(atan2f(site.dx, -site.dz)));
    return text;
}
PUBLIC int mountain_action(int action) {
    uint32_t before = game.notes;
    int pitch = -1;
    if (action == ACT_USE && game.world.place && game.world.place->skill == ECHO &&
        field_stage(&game) == 1 && game_near(&game) == NEAR_READ) {
        float best = 2.25f;
        for (int i = 0; i < 3; i++) {
            Place p = field_pipe(&game.world, i);
            float dist = (float)hypot(game.world.ox + (double)game.x - p.x,
                                      game.world.oz + (double)game.z - p.z);
            if (dist < best) {
                pitch = i;
                best = dist;
            }
        }
    }
    bool thunder_outside = event_thunders(&game);
    uint64_t revision = game.book.revision;
    int ok = game_action(&game, (Action)action);
    if (game.book.revision != revision || cabin_dirty)
        save_cabin();
    if (thunder_outside && !event_thunders(&game) && instrument)
        SDL_ClearAudioStream(instrument);
    if (ok && action >= ACT_LOOKOUT_WEATHER && action <= ACT_LOOKOUT_GIANTS) {
        const Shelter *shelter = destination_shelter(&game);
        if (action == ACT_LOOKOUT_WEATHER)
            snprintf(game.lookout_note, sizeof game.lookout_note, "%s / %s", shelter->name,
                     mountain_option(2, game.book.weather));
        else {
            bool large = action == ACT_LOOKOUT_GIANTS;
            bool enabled = large ? game.book.giants : game.book.predators;
            const char *direction = sighting_direction(large);
            snprintf(game.lookout_note, sizeof game.lookout_note, "Watch %s / %s",
                     shelter->sight[large].name, enabled ? (*direction ? direction : "on") : "off");
        }
        game.lookout_timer = 4;
    }
    if (pitch >= 0) {
        const char *chimes[] = {"chime-0", "chime-1", "chime-2"};
        play_cue(chimes[pitch]);
    }
    if (before != game.notes) {
        int stage = field_stage(&game);
        uint32_t saved = read_notes();
        for (int n = 0; n < scripts.rules.place_count; n++) {
            unsigned shift = n * 2, mask = 3u << shift;
            if ((saved & mask) > (game.notes & mask))
                game.notes = (game.notes & ~mask) | (saved & mask);
        }
        if (field_stage(&game) != stage)
            field_refresh(&game);
        write_notes(game.notes);
    }
    return ok;
}
PUBLIC int mountain_observe(float offset) {
    if (!game.world.place || game.world.place->skill != ALIGN || game_near(&game) != NEAR_READ)
        return mountain_action(ACT_USE);
    float yaw = game.yaw;
    game_look(&game, offset, 0);
    int ok = mountain_action(ACT_USE);
    game.yaw = yaw;
    return ok;
}
PUBLIC void mountain_input(float forward, float right) {
    input_f = clampf(forward, -1, 1);
    input_r = clampf(right, -1, 1);
}
PUBLIC void mountain_look(float yaw, float pitch) { game_look(&game, yaw, pitch); }
PUBLIC void mountain_conditions(int season, int weather, float hour) {
    game_conditions(&game, season, weather, hour);
}
PUBLIC void mountain_select(int kind, int choice) {
    if (kind)
        select_weather(&game, choice);
    else
        select_season(&game, choice);
}
PUBLIC double mountain_path(int index, int axis) {
    const Route *r = &game.world.route;
    if (index < 0 || index >= r->count)
        return 0;
    return axis ? r->point[index].z : r->point[index].x;
}
PUBLIC int mountain_wet(double x, double z) {
    return height_global(&game.world, x, z) < water_global(&game.world, x, z);
}
static void arrive(int dest, uint32_t seed) {
    uint32_t notes = game.notes;
    const char *old = ambience(game.world.profile);
    int season = game.season, weather = game.weather;
    float hour = game.hour;
    bool cycle = game.cycle_seasons, changing = game.changing_weather;
    if (instrument && event_thunders(&game))
        SDL_ClearAudioStream(instrument);
    RoomBook book = game.book;
    game_destination(&game, &scripts.rules, seed, dest);
    game.book = book;
    game.notes = notes;
    field_refresh(&game);
    if (audio && strcmp(old, ambience(game.world.profile))) {
        bool resume = sound_on;
        SDL_DestroyAudioStream(audio);
        audio = NULL;
        SDL_free(track);
        track = NULL;
        track_size = track_pos = 0;
        sound_on = false;
        if (resume)
            mountain_sound();
    }
    game_conditions(&game, season, weather, hour);
    game.cycle_seasons = cycle;
    game.changing_weather = changing;
}
PUBLIC void mountain_seed(uint32_t seed) {
    arrive(destination_for_seed(&scripts.rules, seed), seed);
}
PUBLIC void mountain_variant(uint32_t seed) {
    if (game.world.profile <= SURVEYED)
        mountain_seed(seed);
    else
        arrive(game.world.destination, seed);
}
PUBLIC int mountain_visit(int i) {
    if (i < 0 || i >= scripts.rules.place_count)
        return 0;
    arrive(i, scripts.rules.places[i].seed);
    return 1;
}
PUBLIC const char *mountain_entry(int i, int field) {
    if (i < 0 || i >= scripts.rules.place_count)
        return "";
    const Destination *d = &scripts.rules.places[i];
    return field == 1   ? d->region
           : field == 2 ? d->signs[2]
           : field == 3 ? (d->profile >= LUNAR ? "Beyond Earth" : "Earth")
                        : d->name;
}
PUBLIC double mountain_ascent(int index, int axis) {
    const Ascent *a = &game.world.ascent;
    if (index < 0 || index >= a->count)
        return NAN;
    return axis ? a->point[index].z : a->point[index].x;
}
PUBLIC void mountain_pause(int state) {
    paused = state != 0;
    input_f = input_r = 0;
}
PUBLIC const char *mountain_label(int key) {
    switch (key) {
    case 6:
        return game.world.place ? game.world.place->stops[0] : "Lakeside View";
    case 7:
        return game.world.place ? game.world.place->stops[1] : "Ridge Lookout";
    case 8:
        return field_hint(&game);
    case 9:
        return game.world.place ? game.world.place->region : "Alpine backcountry";
    case 10:
        return game.world.place
                   ? game.world.place->signs[field_stage(&game) < 2 ? field_stage(&game) : 2]
                   : "";
    case 11:
        return game.world.place ? game.world.place->notes[2] : "";
    case 12:
        return destination_shelter(&game)->name;
    case 13:
    case 14:
        return destination_shelter(&game)->sight[key - 13].name;
    case 15:
    case 16:
        return sighting_direction(key - 15);
    case 4:
        return scripts.rules.survey.name;
    case 5:
        return scripts.rules.survey.region;
    case 0:
        return trail_name(&game);
    case 1:
        return game.rules->seasons[game.season];
    case 2:
        return mountain_option(1, game.weather);
    default:
        return cabin_layout(&game)->name;
    }
}
PUBLIC const char *mountain_option(int kind, int index) {
    if (kind == 2)
        return index >= 1 && index <= 3 ? destination_shelter(&game)->event[index - 1].name
                                        : "Calm";
    return kind ? weather_label(&game, index) : game.rules->seasons[(unsigned)index % SEASONS];
}
PUBLIC float mountain_height(double x, double z) { return height_global(&game.world, x, z); }
PUBLIC int mountain_sightings(void) {
    Animal animals[LOCAL_ANIMALS];
    int mask = 0;
    int n = fauna_near(&game, animals, LOCAL_ANIMALS);
    for (int i = 0; i < n; i++)
        if (hypotf(animals[i].pos.x - game.x, animals[i].pos.z - game.z) < 72)
            mask |= 1 << animals[i].kind;
    return mask;
}
PUBLIC const char *mountain_species(int k) {
    return (unsigned)k < FAUNA_TYPES ? scripts.rules.fauna_names[k] : "";
}
PUBLIC double mountain_extension(double index, int axis) {
    if (!isfinite(index) || index > (double)(INT64_MAX / 64))
        return NAN;
    double x, z;
    trail_coordinates(&game.world, (int64_t)fmax(0, floor(index)), &x, &z);
    return axis ? z : x;
}
PUBLIC float mountain_water(void) { return game.world.water; }
PUBLIC double mountain_value(int key) {
    switch (key) {
    case 0:
        return game.world.ox + (double)game.x;
    case 1:
        return game.world.oz + (double)game.z;
    case 2:
        return game.yaw;
    case 3:
        return game.pitch;
    case 4:
        return game.hour;
    case 5:
        return game.season;
    case 6:
        return game.weather;
    case 7:
        return game.world.seed;
    case 8:
        if (game.world.profile > SURVEYED)
            return floorf(game.world.place->datum + eye(&game).y - 1.65f);
        return game.world.survey ? floorf(game.world.survey->surface +
                                          (eye(&game).y - 1.65f) * game.world.survey->scale)
                                 : floorf(1640 + eye(&game).y - 1.65f);
    case 9:
        return game.cabin.present;
    case 10:
        return game.cabin.open;
    case 11:
        return game.cabin.inside;
    case 12:
        return (double)game.cabin.room;
    case 13:
        return game_near(&game);
    case 14:
        return sound_on;
    case 15:
        return game.walked;
    case 16:
        return game.elapsed;
    case 18:
        return game.cycle_seasons;
    case 19:
        return game.changing_weather;
    case 20:
        return game.world.route.count;
    case 21:
        return game.world.route.overlook;
    case 22:
        return game.world.route.length;
    case 23:
        return scripts.rules.survey.seed;
    case 24:
        return game.world.survey ? game.world.survey->scale : 1;
    case 26:
        return trail_station(&game.world, game.x, game.z) / TRAIL_STEP;
    case 27:
        return fmax(fabs(game.world.ox + (double)game.x), fabs(game.world.oz + (double)game.z));
    case 28:
        return game.world.destination;
    case 29:
        return scripts.rules.place_count;
    case 30:
        return field_stage(&game);
    case 31:
        return game.world.ascent.count;
    case 32:
        return game.world.ascent.pass;
    case 33:
        return game.fall_time > 0 ? 2 : game.warning_timer > 0 ? 3 : game.exposed;
    case 34:
        return game.notes;
    case 35:
        return game.falls;
    case 36:
        return game.world.profile;
    case 37: {
        const Ascent *a = &game.world.ascent;
        double x = game.world.ox + (double)game.x, z = game.world.oz + (double)game.z;
        return a->count && hypot(x - a->start.x, z - a->start.z) > 36 &&
               ascent_distance(&game.world, x, z) < 24;
    }
    case 38:
        return game.book.count + 1;
    case 39:
        return game.book.weather;
    case 40:
        return game.book.predators;
    case 41:
        return game.book.giants;
    case 43:
        return cabin_dirty;
    case 42: {
        int n = cabin_window(&game);
        return n >= 0 && (cabin_room(&game.book, game.cabin.room)->blinds & (1u << n));
    }
    case 25:
        return scripts.rules.survey.data != NULL;
    default:
        return display != NULL;
    }
}
PUBLIC void mountain_resize(int w, int h) {
    if (w > 0 && h > 0)
        SDL_SetWindowSize(window, w, h);
}
PUBLIC void mountain_frame(void) {
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    display_draw(display, &scripts, &game, (Viewport){0, 0, w, h}, NULL, NULL);
}
PUBLIC void mountain_xr_active(int enabled) {
    immersive = enabled != 0;
    input_f = input_r = 0;
}
PUBLIC void mountain_walk(float dt, float f, float r) {
    bool thunder = event_thunders(&game);
    double previous_phase = fmod(game.elapsed, 19);
    game_tick(&game, dt, f, r);
    if (thunder) {
        if (game.cabin.inside && previous_phase < 5.15 && fmod(game.elapsed, 19) >= 5.15)
            play_cue("thunder");
        if (!game.cabin.inside && instrument)
            SDL_ClearAudioStream(instrument);
    }
}
PUBLIC void mountain_xr_eye(const Mat *projection, const Mat *view, int x, int y, int w, int h) {
    display_draw(display, &scripts, &game, (Viewport){x, y, w, h}, projection, view);
}
bool mountain_photo(void) {
    char *dir = SDL_GetPrefPath("Mountain", "Mountain");
    if (!dir)
        return false;
    SDL_Time now = 0;
    SDL_GetCurrentTime(&now);
    char path[1200];
    snprintf(path, sizeof path, "%sMountain-%u-%lld.bmp", dir, game.world.seed, (long long)now);
    SDL_free(dir);
    bool ok = display_save(display, &scripts, &game, path);
    if (ok) {
        SDL_Log("Photograph saved to %s", path);
        SDL_SetWindowTitle(window, path);
    }
    return ok;
}
static void key(SDL_Keycode k) {
    switch (k) {
    case SDLK_H:
        mountain_action(ACT_CABIN);
        break;
    case SDLK_E:
        mountain_action(ACT_USE);
        break;
    case SDLK_F:
        mountain_action(ACT_DOOR);
        break;
    case SDLK_J:
        mountain_action(ACT_BLINDS);
        break;
    case SDLK_K:
        mountain_action(ACT_LOOKOUT_WEATHER);
        break;
    case SDLK_L:
        mountain_action(ACT_LOOKOUT_PREDATORS);
        break;
    case SDLK_X:
        mountain_action(ACT_LOOKOUT_GIANTS);
        break;
    case SDLK_R:
        mountain_action(ACT_HOME);
        break;
    case SDLK_T:
        mountain_action(ACT_TIME);
        break;
    case SDLK_Y:
        mountain_action(ACT_SEASON);
        break;
    case SDLK_U:
        mountain_action(ACT_WEATHER);
        break;
    case SDLK_I:
        mountain_action(ACT_CYCLE_SEASONS);
        break;
    case SDLK_O:
        mountain_action(ACT_CHANGING_WEATHER);
        break;
    case SDLK_B:
        if (scripts.rules.survey.data)
            mountain_seed(scripts.rules.survey.seed);
        break;
    case SDLK_LEFTBRACKET:
    case SDLK_RIGHTBRACKET:
        if (scripts.rules.place_count)
            mountain_visit((game.world.destination + scripts.rules.place_count +
                            (k == SDLK_RIGHTBRACKET ? 1 : -1)) %
                           scripts.rules.place_count);
        break;
    case SDLK_N:
        mountain_variant(SDL_rand_bits());
        break;
    case SDLK_P:
        mountain_photo();
        break;
    case SDLK_M:
        mountain_sound();
        break;
    case SDLK_F1:
        help = !help;
        break;
    case SDLK_ESCAPE:
        SDL_SetWindowRelativeMouseMode(window, false);
        help = false;
        break;
    default:
        break;
    }
}
bool mountain_step(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT)
            running = false;
#ifndef __EMSCRIPTEN__
        if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat)
            key(ev.key.key);
        if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            SDL_SetWindowRelativeMouseMode(window, true);
        if (ev.type == SDL_EVENT_MOUSE_MOTION && SDL_GetWindowRelativeMouseMode(window))
            mountain_look(ev.motion.xrel * .0024f, -ev.motion.yrel * .0024f);
#endif
    }
    Uint64 now = SDL_GetTicksNS();
    float dt = clampf((float)((now - previous) * 1e-9), 0, .12f);
    previous = now;
    if (immersive)
        return running;
    float f = input_f, r = input_r;
#ifndef __EMSCRIPTEN__
    const bool *keys = SDL_GetKeyboardState(NULL);
    f += keys[SDL_SCANCODE_W] + keys[SDL_SCANCODE_UP] - keys[SDL_SCANCODE_S] -
         keys[SDL_SCANCODE_DOWN];
    r += keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A];
    if (!help)
        game_look(&game, (keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT]) * dt * 1.45f, 0);
#endif
    if (!paused && !help)
        mountain_walk(dt, f, r);
#ifdef __EMSCRIPTEN__
    mountain_frame();
#else
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    display_native(display, &scripts, &game, w, h, help, false);
#endif
    SDL_GL_SwapWindow(window);
    return running;
}
void mountain_close(void) {
    if (cabin_dirty)
        save_cabin();
    cabin_forget(&game.book);
    cabin_dirty = false;
    if (instrument)
        SDL_DestroyAudioStream(instrument);
    instrument = NULL;
    if (audio)
        SDL_DestroyAudioStream(audio);
    SDL_free(track);
    display_close(display);
    script_close(&scripts);
    if (context)
        SDL_GL_DestroyContext(context);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    audio = NULL;
    track = NULL;
    track_size = track_pos = 0;
    display = NULL;
    context = NULL;
    window = NULL;
    sound_on = false;
}
bool mountain_open(const char *asset_dir, int width, int height) {
    running = true;
    paused = help = immersive = false;
    input_f = input_r = 0;
#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL: %s", SDL_GetError());
        return false;
    }
#ifdef __EMSCRIPTEN__
    snprintf(assets, sizeof assets, "/assets");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    snprintf(assets, sizeof assets, "%sassets", SDL_GetBasePath());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    if (asset_dir)
        snprintf(assets, sizeof assets, "%s", asset_dir);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    if (!script_load(&scripts, assets)) {
        SDL_Log("Lua: %s", scripts.error);
        mountain_close();
        return false;
    }
    game_init(&game, &scripts.rules, scripts.rules.seed);
    game.notes = read_notes();
    char *saved_cabin = load_cabin();
    if (saved_cabin && !cabin_decode(&game.book, saved_cabin, scripts.rules.layouts))
        SDL_Log("Could not read the saved cabin floor plan.");
    free(saved_cabin);
    field_refresh(&game);
    window = SDL_CreateWindow("Mountain", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window || !(context = SDL_GL_CreateContext(window)) || !(display = display_open(assets))) {
        SDL_Log("Mountain display: %s", SDL_GetError());
        mountain_close();
        return false;
    }
    SDL_GL_SetSwapInterval(1);
    previous = SDL_GetTicksNS();
    return true;
}
