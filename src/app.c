#include "app.h"
#include <math.h>
#include <stdio.h>
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
        snprintf(path, sizeof path, "%s/sound/ridge.wav", assets);
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
        SDL_PauseAudioStreamDevice(audio);
        sound_on = false;
    } else
        sound_on = SDL_ResumeAudioStreamDevice(audio);
    return sound_on;
}
PUBLIC int mountain_action(int action) { return game_action(&game, (Action)action); }
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
PUBLIC int mountain_wet(float x, float z) {
    return height_at(&game.world, x, z) < water_at(&game.world, x, z);
}
PUBLIC void mountain_seed(uint32_t seed) {
    int season = game.season, weather = game.weather;
    float hour = game.hour;
    bool cycle = game.cycle_seasons, changing = game.changing_weather;
    game_init(&game, &scripts.rules, seed);
    game_conditions(&game, season, weather, hour);
    game.cycle_seasons = cycle;
    game.changing_weather = changing;
}
PUBLIC void mountain_pause(int state) {
    paused = state != 0;
    input_f = input_r = 0;
}
PUBLIC const char *mountain_label(int key) {
    switch (key) {
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
        return room_name(&scripts, game.cabin.room);
    }
}
PUBLIC const char *mountain_option(int kind, int index) {
    if (kind && game.season == 3 && index == 2)
        return game.rules->winter_precipitation;
    return kind ? game.rules->skies[(unsigned)index % WEATHERS]
                : game.rules->seasons[(unsigned)index % SEASONS];
}
PUBLIC float mountain_height(float x, float z) { return height_at(&game.world, x, z); }
PUBLIC float mountain_water(void) { return game.world.water; }
PUBLIC double mountain_value(int key) {
    switch (key) {
    case 0:
        return game.x;
    case 1:
        return game.z;
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
PUBLIC void mountain_walk(float dt, float f, float r) { game_tick(&game, dt, f, r); }
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
    case SDLK_N:
        mountain_seed(SDL_rand_bits());
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
        game_tick(&game, dt, f, r);
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
