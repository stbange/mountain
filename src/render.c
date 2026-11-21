#include "render.h"
#include <SDL3/SDL_opengl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
/* A compositor-owned XR framebuffer has no Emscripten GLuint name.
   Preserve its actual WebGL object during offscreen rendering. */
EM_JS(void, save_browser_target, (void),
      { Module['mountainSavedTarget'] = GLctx.getParameter(GLctx.FRAMEBUFFER_BINDING); });
EM_JS(void, restore_browser_target, (void), {
    GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, Module['mountainSavedTarget']);
    Module['mountainSavedTarget'] = null;
});
#endif

/* Core GL 2 / GLES 2 operations, resolved from the active SDL context. */
#define FUNCTIONS(X)                                                                               \
    X(GLuint, CreateShader, (GLenum))                                                              \
    X(void, ShaderSource, (GLuint, GLsizei, const char *const *, const GLint *))                   \
    X(void, CompileShader, (GLuint))                                                               \
    X(void, GetShaderiv, (GLuint, GLenum, GLint *))                                                \
    X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, char *))                                \
    X(GLuint, CreateProgram, (void))                                                               \
    X(void, AttachShader, (GLuint, GLuint)) X(void, BindAttribLocation,                            \
                                              (GLuint, GLuint, const char *))                      \
        X(void, LinkProgram, (GLuint)) X(void, GetProgramiv, (GLuint, GLenum, GLint *)) X(         \
            void, GetProgramInfoLog,                                                               \
            (GLuint, GLsizei, GLsizei *,                                                           \
             char *)) X(void, UseProgram, (GLuint)) X(void, DeleteShader,                          \
                                                      (GLuint)) X(void, DeleteProgram, (GLuint))   \
            X(GLint, GetUniformLocation, (GLuint, const char *)) X(                                \
                void,                                                                              \
                UniformMatrix4fv,                                                                  \
                (GLint, GLsizei, GLboolean, const GLfloat *)) X(void, Uniform1i, (GLint, GLint))   \
                X(void, Uniform1f, (GLint, GLfloat)) X(                                            \
                    void,                                                                          \
                    Uniform3f,                                                                     \
                    (GLint, GLfloat, GLfloat,                                                      \
                     GLfloat)) X(void, Uniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat))     \
                    X(void, GenBuffers, (GLsizei, GLuint *)) X(                                    \
                        void,                                                                      \
                        DeleteBuffers,                                                             \
                        (GLsizei, const GLuint *)) X(void, BindBuffer, (GLenum, GLuint))           \
                        X(void, BufferData, (GLenum, ptrdiff_t, const void *, GLenum)) X(          \
                            void,                                                                  \
                            VertexAttribPointer,                                                   \
                            (GLuint, GLint, GLenum, GLboolean, GLsizei,                            \
                             const void *)) X(void, EnableVertexAttribArray, (GLuint))             \
                            X(void, DrawArrays, (GLenum, GLint, GLsizei)) X(                       \
                                void,                                                              \
                                GenTextures,                                                       \
                                (GLsizei, GLuint *)) X(void, BindTexture, (GLenum, GLuint))        \
                                X(void, TexParameteri, (GLenum, GLenum, GLint)) X(                 \
                                    void,                                                          \
                                    TexImage2D,                                                    \
                                    (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,        \
                                     GLenum,                                                       \
                                     const void                                                    \
                                         *)) X(void, DeleteTextures, (GLsizei, const GLuint *))    \
                                    X(void, Viewport, (GLint, GLint, GLsizei, GLsizei)) X(         \
                                        void,                                                      \
                                        Scissor,                                                   \
                                        (GLint, GLint, GLsizei,                                    \
                                         GLsizei)) X(void, Enable, (GLenum))                       \
                                        X(void, Disable, (GLenum)) X(void, Clear, (GLbitfield)) X( \
                                            void,                                                  \
                                            ReadPixels,                                            \
                                            (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,       \
                                             void *)) X(void, ActiveTexture, (GLenum))             \
                                            X(void, GetIntegerv,                                   \
                                              (GLenum,                                             \
                                               GLint *)) X(void, GenFramebuffers,                  \
                                                           (GLsizei, GLuint *))                    \
                                                X(void, BindFramebuffer, (GLenum, GLuint)) X(      \
                                                    void,                                          \
                                                    FramebufferTexture2D,                          \
                                                    (GLenum, GLenum, GLenum, GLuint,               \
                                                     GLint)) X(void, DeleteFramebuffers,           \
                                                               (GLsizei, const GLuint *))          \
                                                    X(void, GenRenderbuffers, (GLsizei, GLuint *)) \
                                                        X(void, BindRenderbuffer,                  \
                                                          (GLenum, GLuint))                        \
                                                            X(void, RenderbufferStorage,           \
                                                              (GLenum, GLenum, GLsizei, GLsizei))  \
                                                                X(void, FramebufferRenderbuffer,   \
                                                                  (GLenum, GLenum, GLenum,         \
                                                                   GLuint))                        \
                                                                    X(void, DeleteRenderbuffers,   \
                                                                      (GLsizei, const GLuint *))   \
                                                                        X(GLenum,                  \
                                                                          CheckFramebufferStatus,  \
                                                                          (GLenum))
#define SHADOW_FUNCTIONS(X)                                                                        \
    X(void, DrawBuffers, (GLsizei, const GLenum *))                                                \
    X(void, PixelStorei, (GLenum, GLint))                                                          \
    X(void, ReadBuffer, (GLenum))                                                                  \
    X(void, PolygonOffset, (GLfloat, GLfloat))                                                     \
    X(void, BlitFramebuffer,                                                                       \
      (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum))
#define DECL(ret, name, args) static ret(APIENTRY *q##name) args;
FUNCTIONS(DECL)
SHADOW_FUNCTIONS(DECL)
typedef struct {
    GLuint id;
    GLsizei count;
} Buffer;
typedef struct {
    GLuint texture;
    Mat matrix;
    int size;
    bool valid;
    Vec centre;
    float hour;
    double time;
} ShadowMap;
struct Display {
    GLuint shadow_program, shadow_fb, static_fb, static_depth;
    ShadowMap shadows[2];
    GLint shadow_vp, shadow_clock, shadow_pattern, near_light, far_light, light_params;
    GLint sky_fill, bounce, cloud_origin, house_pass, lamps[4], lamp_colour[4];
    int shadow_weather;
    uint64_t cabin_revision;
    GLint room_origin, room_axes, lookout;
    uint32_t shadow_notes;
    double life_time;
    float life_hour, life_x, life_z;
    int life_weather;
    uint32_t life_notes;
    bool life_valid;
    GLuint program, atlas, font, trail, canopy, clouds, fb, frame, depth, lake_fb, lake_tex,
        lake_depth;
    GLint reflect, water, viewport, season_tint, trail_center, pattern, profile, liquid, soil;
    int destination;
    int trail_x, trail_z;
    int64_t ox, oz;
    unsigned char tread[TRAIL_TEXTURE_SIZE * TRAIL_TEXTURE_SIZE];
    Mat lake_view;
    double lake_time;
    float lake_hour;
    int lake_season, lake_weather;
    uint32_t lake_seed;
    GLint vp, inv, eye, layer, air, zenith, horizon, sun, clock;
    Buffer land, house, life, screen, hud;
    Mesh scratch;
    int cx, cz, season, fw, fh;
    uint32_t seed;
    Cabin cached;
    bool valid, house_in_view;
};
static bool resolve(void) {
#define LOAD(ret, name, args)                                                                      \
    q##name = (ret(APIENTRY *) args)SDL_GL_GetProcAddress("gl" #name);                             \
    if (!q##name) {                                                                                \
        SDL_Log("Missing gl%s", #name);                                                            \
        return false;                                                                              \
    }
    FUNCTIONS(LOAD)
    SHADOW_FUNCTIONS(LOAD)
#undef LOAD
    return true;
}
static GLuint shader(GLenum type, const char *root, const char *name) {
    char path[1024];
    snprintf(path, sizeof path, "%s/shaders/%s", root, name);
    char *body = SDL_LoadFile(path, NULL);
    if (!body)
        return 0;
    bool vertex = type == GL_VERTEX_SHADER;
#ifdef __EMSCRIPTEN__
    const char *head = "#version 300 es\nprecision highp float;\nprecision highp int;\n";
    const char *syntax = vertex ? "#define IN in\n#define OUT out\n"
                                : "#define IN in\n#define SAMPLE texture\nout vec4 "
                                  "fragmentColour;\n#define FRAGMENT fragmentColour\n";
#else
    const char *head = "#version 120\n";
    const char *syntax =
        vertex ? "#define IN attribute\n#define OUT varying\n"
               : "#define IN varying\n#define SAMPLE texture2D\n#define FRAGMENT gl_FragColor\n";
#endif
    char *shared = NULL;
    if (!vertex) {
        snprintf(path, sizeof path, "%s/shaders/surface.glsl", root);
        shared = SDL_LoadFile(path, NULL);
        if (!shared) {
            SDL_free(body);
            return 0;
        }
    }
    const char *parts[] = {head, syntax, shared ? shared : "", body};
    GLuint id = qCreateShader(type);
    qShaderSource(id, 4, parts, NULL);
    SDL_free(shared);
    qCompileShader(id);
    SDL_free(body);
    GLint ok = 0;
    qGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[768];
        qGetShaderInfoLog(id, sizeof log, NULL, log);
        SDL_Log("%s: %s", name, log);
        qDeleteShader(id);
        return 0;
    }
    return id;
}
static void texture_settings(void) {
    qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
static GLuint texture(const char *root, const char *name) {
    char path[1024];
    snprintf(path, sizeof path, "%s/art/%s", root, name);
    size_t n = 0;
    unsigned char *raw = SDL_LoadFile(path, &n);
    if (!raw)
        return 0;
    int w = 0, h = 0;
    bool grey = n >= 18 && raw[2] == 3 && raw[16] == 8;
    if (n >= 18 && (grey || (raw[2] == 2 && raw[16] == 32))) {
        w = raw[12] + 256 * raw[13];
        h = raw[14] + 256 * raw[15];
    }
    if (w == 0 || h == 0 || n - 18 != (size_t)w * h * (grey ? 1 : 4)) {
        SDL_free(raw);
        return 0;
    }
    for (size_t i = 18; !grey && i < n; i += 4) {
        unsigned char b = raw[i];
        raw[i] = raw[i + 2];
        raw[i + 2] = b;
    }
    GLuint id;
    qGenTextures(1, &id);
    qBindTexture(GL_TEXTURE_2D, id);
    texture_settings();
    if (grey) {
        qPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifdef __EMSCRIPTEN__
        qTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, raw + 18);
#else
        qTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    raw + 18);
#endif
        qPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    } else
        qTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw + 18);
    SDL_free(raw);
    return id;
}
static void upload(Buffer *b, const Mesh *m, GLenum usage) {
    if (!b->id)
        qGenBuffers(1, &b->id);
    qBindBuffer(GL_ARRAY_BUFFER, b->id);
    qBufferData(GL_ARRAY_BUFFER, (ptrdiff_t)(m->count * sizeof(Vertex)), m->v, usage);
    b->count = (GLsizei)m->count;
}
static void draw(Buffer b) {
    if (!b.count)
        return;
    qBindBuffer(GL_ARRAY_BUFFER, b.id);
    qVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
    qVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, r));
    qVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, u));
    qDrawArrays(GL_TRIANGLES, 0, b.count);
}
static GLuint scene_program(const char *root, const char *fragment) {
    GLuint v = shader(GL_VERTEX_SHADER, root, "ridge.vert"),
           f = shader(GL_FRAGMENT_SHADER, root, fragment);
    GLuint program = 0;
    if (v && f) {
        program = qCreateProgram();
        qAttachShader(program, v);
        qAttachShader(program, f);
        qBindAttribLocation(program, 0, "a_pos");
        qBindAttribLocation(program, 1, "a_colour");
        qBindAttribLocation(program, 2, "a_uvm");
        qLinkProgram(program);
        GLint ok;
        qGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char msg[600];
            qGetProgramInfoLog(program, sizeof msg, NULL, msg);
            SDL_Log("%s: %s", fragment, msg);
            qDeleteProgram(program);
            program = 0;
        }
    }
    if (v)
        qDeleteShader(v);
    if (f)
        qDeleteShader(f);
    return program;
}
static bool depth_texture(GLuint *texture, int size) {
    qGenTextures(1, texture);
    qBindTexture(GL_TEXTURE_2D, *texture);
    texture_settings();
    qTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT,
                GL_UNSIGNED_INT, NULL);
    qFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *texture, 0);
    return qCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}
static void depth_framebuffer(GLuint *framebuffer) {
    GLenum none = GL_NONE;
    qGenFramebuffers(1, framebuffer);
    qBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
    qDrawBuffers(1, &none);
    qReadBuffer(GL_NONE);
}
static bool shadow_targets(Display *d) {
    GLint limit;
    qGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    bool compact = false;
#ifdef __EMSCRIPTEN__
    compact = EM_ASM_INT({ return window.matchMedia("(pointer: coarse)").matches; });
#endif
    depth_framebuffer(&d->shadow_fb);
    bool complete = true;
    for (int i = 0; i < 2; i++) {
        ShadowMap *map = &d->shadows[i];
        int size = i ? (compact ? 768 : 1024) : (compact ? 1024 : 1536);
        map->size = size < limit ? size : limit;
        qActiveTexture(GL_TEXTURE4 + i);
        complete = depth_texture(&map->texture, map->size) && complete;
    }
    /* Keep the near landscape depth between wildlife poses. */
    qActiveTexture(GL_TEXTURE0);
    depth_framebuffer(&d->static_fb);
    complete = depth_texture(&d->static_depth, d->shadows[0].size) && complete;
    qBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete)
        SDL_Log("Could not create sunlight depth target");
    return complete;
}
Display *display_open(const char *root) {
    if (!resolve())
        return NULL;
    Display *d = calloc(1, sizeof *d);
    if (!d)
        return NULL;
    d->program = scene_program(root, "ridge.frag");
    d->shadow_program = scene_program(root, "shadow.frag");
    if (!d->program || !d->shadow_program || !shadow_targets(d)) {
        display_close(d);
        return NULL;
    }
    qUseProgram(d->shadow_program);
    d->shadow_vp = qGetUniformLocation(d->shadow_program, "u_vp");
    d->shadow_clock = qGetUniformLocation(d->shadow_program, "u_clock");
    d->shadow_pattern = qGetUniformLocation(d->shadow_program, "u_pattern");
    qUniform1i(qGetUniformLocation(d->shadow_program, "u_canopy"), 3);
    qUseProgram(d->program);
#define LOC(field, name) d->field = qGetUniformLocation(d->program, "u_" name)
    LOC(near_light, "near_light");
    LOC(far_light, "far_light");
    LOC(sky_fill, "sky_fill");
    LOC(bounce, "bounce");
    LOC(light_params, "light_params");
    LOC(cloud_origin, "cloud_origin");
    LOC(house_pass, "house");
    LOC(room_origin, "room_origin");
    LOC(room_axes, "room_axes");
    LOC(lookout, "lookout");
    for (int i = 0; i < 4; i++) {
        char name[32];
        snprintf(name, sizeof name, "u_lamps[%d]", i);
        d->lamps[i] = qGetUniformLocation(d->program, name);
        snprintf(name, sizeof name, "u_lamp_colour[%d]", i);
        d->lamp_colour[i] = qGetUniformLocation(d->program, name);
    }
    qUniform1i(qGetUniformLocation(d->program, "u_near_depth"), 4);
    qUniform1i(qGetUniformLocation(d->program, "u_far_depth"), 5);
    LOC(vp, "vp");
    LOC(inv, "inverse");
    LOC(eye, "eye");
    LOC(layer, "layer");
    LOC(air, "air");
    LOC(zenith, "zenith");
    LOC(horizon, "horizon");
    LOC(sun, "sun");
    LOC(clock, "clock");
    LOC(reflect, "reflection");
    LOC(water, "water");
    LOC(trail_center, "trail_center");
    LOC(pattern, "pattern");
    LOC(profile, "profile");
    LOC(liquid, "liquid");
    LOC(soil, "soil");
    LOC(season_tint, "season");
    LOC(viewport, "viewport");
    qUniform1i(qGetUniformLocation(d->program, "u_atlas"), 0);
    qUniform1i(qGetUniformLocation(d->program, "u_lake"), 1);
    qUniform1i(qGetUniformLocation(d->program, "u_trail"), 2);
    qUniform1i(qGetUniformLocation(d->program, "u_canopy"), 3);
    qGenTextures(1, &d->canopy);
    qGenTextures(1, &d->trail);
    qActiveTexture(GL_TEXTURE6);
    d->clouds = texture(root, "clouds.tga");
    qUniform1i(qGetUniformLocation(d->program, "u_clouds"), 6);
    qActiveTexture(GL_TEXTURE0);
    d->atlas = texture(root, "field.tga");
    d->font = texture(root, "letters.tga");
    if (!d->atlas || !d->font || !d->clouds) {
        display_close(d);
        return NULL;
    }
    Color white = {1, 1, 1};
    tex_quad(&d->scratch, (Vec){-1, -1, 0}, (Vec){1, -1, 0}, (Vec){1, 1, 0}, (Vec){-1, 1, 0}, 0, 1,
             1, 0, white, 0);
    upload(&d->screen, &d->scratch, GL_STATIC_DRAW);
    for (int i = 0; i < 3; i++)
        qEnableVertexAttribArray((GLuint)i);
    return d;
}
void display_close(Display *d) {
    if (!d)
        return;
    GLuint buffers[] = {d->land.id, d->house.id, d->life.id, d->screen.id, d->hud.id};
    GLuint tex[] = {d->atlas, d->font, d->frame, d->lake_tex, d->trail, d->canopy, d->clouds};
    qDeleteBuffers(5, buffers);
    qDeleteTextures(7, tex);
    for (int i = 0; i < 2; i++)
        qDeleteTextures(1, &d->shadows[i].texture);
    qDeleteFramebuffers(1, &d->shadow_fb);
    qDeleteFramebuffers(1, &d->static_fb);
    qDeleteTextures(1, &d->static_depth);
    qDeleteProgram(d->shadow_program);
    qDeleteFramebuffers(1, &d->lake_fb);
    qDeleteRenderbuffers(1, &d->lake_depth);
    qDeleteFramebuffers(1, &d->fb);
    qDeleteRenderbuffers(1, &d->depth);
    qDeleteProgram(d->program);
    mesh_free(&d->scratch);
    free(d);
}
static void scenery(Display *d, Script *s, const Game *g) {
    const Cabin *c = &g->cabin, *old = &d->cached;
    if (d->destination != g->world.destination) {
        d->valid = false;
        d->destination = g->world.destination;
        d->lake_time = -100;
    }
    if (g->world.profile <= SURVEYED &&
        (!d->valid || d->seed != g->world.seed || d->season != g->season)) {
        forest_prepare(&s->forest, g);
        qActiveTexture(GL_TEXTURE3);
        qBindTexture(GL_TEXTURE_2D, d->canopy);
        texture_settings();
        qTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FOREST_ATLAS, FOREST_ATLAS, 0, GL_RGBA,
                    GL_UNSIGNED_BYTE, s->forest.atlas);
        qActiveTexture(GL_TEXTURE0);
    }
    bool shifted = d->ox != g->world.ox || d->oz != g->world.oz;
    int tx = (int)floorf((g->x + 32) / 64) * 64, tz = (int)floorf((g->z + 32) / 64) * 64;
    if (!d->valid || d->seed != g->world.seed || shifted || d->trail_x != tx || d->trail_z != tz) {
        d->trail_x = tx;
        d->trail_z = tz;
        qUniform3f(d->trail_center, (float)tx, 0, (float)tz);
        qUniform3f(d->pattern, (float)(g->world.ox % 4096), 0, (float)(g->world.oz % 4096));
        route_texture(&g->world, (float)tx, (float)tz, d->tread);
        qActiveTexture(GL_TEXTURE2);
        qBindTexture(GL_TEXTURE_2D, d->trail);
        texture_settings();
        qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        qTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef __EMSCRIPTEN__
        qTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TRAIL_TEXTURE_SIZE, TRAIL_TEXTURE_SIZE, 0, GL_RED,
                    GL_UNSIGNED_BYTE, d->tread);
#else
        qTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, TRAIL_TEXTURE_SIZE, TRAIL_TEXTURE_SIZE, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, d->tread);
#endif
        qActiveTexture(GL_TEXTURE0);
    }
    bool site = shifted || d->season != g->season || c->present != old->present || c->x != old->x ||
                c->z != old->z || c->angle != old->angle;
    int cx = (int)floorf((g->x + 14) / 28) * 28, cz = (int)floorf((g->z + 14) / 28) * 28;
    if (!d->valid || d->seed != g->world.seed || d->season != g->season || cx != d->cx ||
        cz != d->cz || site) {
        landscape(&d->scratch, &s->forest, g, (float)cx, (float)cz);
        upload(&d->land, &d->scratch, GL_STATIC_DRAW);
        d->life_valid = d->shadows[0].valid = d->shadows[1].valid = false;
        d->cx = cx;
        d->cz = cz;
        d->seed = g->world.seed;
        d->season = g->season;
    }
    bool house_in_view = cabin_in_view(g);
    if (!d->valid || site || d->house_in_view != house_in_view || old->open != c->open ||
        old->inside != c->inside || old->room != c->room || d->cabin_revision != g->book.revision) {
        d->cabin_revision = g->book.revision;
        Cabin frame = cabin_frame(g);
        const Layout *layout = cabin_layout(g);
        qUniform3f(d->room_origin, c->x, c->y, c->z);
        qUniform4f(d->room_axes, cosf(frame.angle), sinf(frame.angle), layout->width,
                   layout->depth);
        cabin_mesh(&d->scratch, s, g);
        upload(&d->house, &d->scratch, GL_STATIC_DRAW);
        d->life_valid = d->shadows[0].valid = d->shadows[1].valid = false;
        d->lake_time = -100;
        for (int i = 0; i < 4; i++) {
            const struct RoomLight *lamp = &d->scratch.lights[i];
            float radius = i < d->scratch.light_count ? lamp->radius : 0;
            qUniform4f(d->lamps[i], lamp->position.x, lamp->position.y, lamp->position.z, radius);
            qUniform3f(d->lamp_colour[i], lamp->colour.r, lamp->colour.g * .70f,
                       lamp->colour.b * .40f);
        }
    }
    d->house_in_view = house_in_view;
    d->cached = *c;
    d->ox = g->world.ox;
    d->oz = g->world.oz;
    d->valid = true;
    if (!d->life_valid || d->life_time != g->elapsed || d->life_hour != g->hour ||
        d->life_x != g->x || d->life_z != g->z || d->life_weather != g->weather ||
        d->life_notes != g->notes) {
        wildlife(&d->scratch, s, g);
        upload(&d->life, &d->scratch, GL_DYNAMIC_DRAW);
        d->life_time = g->elapsed;
        d->life_hour = g->hour;
        d->life_x = g->x;
        d->life_z = g->z;
        d->life_weather = g->weather;
        d->life_notes = g->notes;
        d->life_valid = true;
    }
}
static Mat inverse(Mat a) {
    float q[4][8];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            q[i][j] = a.m[j * 4 + i];
            q[i][j + 4] = (float)(i == j);
        }
    for (int k = 0; k < 4; k++) {
        int pivot = k;
        for (int i = k + 1; i < 4; i++)
            if (fabsf(q[i][k]) > fabsf(q[pivot][k]))
                pivot = i;
        for (int j = 0; j < 8; j++) {
            float t = q[k][j];
            q[k][j] = q[pivot][j];
            q[pivot][j] = t;
        }
        float scale = q[k][k];
        if (fabsf(scale) < 1e-12f)
            return identity();
        for (int j = 0; j < 8; j++)
            q[k][j] /= scale;
        for (int i = 0; i < 4; i++)
            if (i != k) {
                float f = q[i][k];
                for (int j = 0; j < 8; j++)
                    q[i][j] -= f * q[k][j];
            }
    }
    Mat result;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            result.m[j * 4 + i] = q[i][j + 4];
    return result;
}
static void camera(Display *d, Mat projection, Mat view) {
    Mat vp = multiply(projection, view), inv = inverse(vp), viewer = inverse(view);
    qUniformMatrix4fv(d->vp, 1, GL_FALSE, vp.m);
    qUniformMatrix4fv(d->inv, 1, GL_FALSE, inv.m);
    qUniform3f(d->eye, viewer.m[12], viewer.m[13], viewer.m[14]);
}
static void outdoor(Display *d, Viewport vp) {
    qBindTexture(GL_TEXTURE_2D, d->atlas);
    qViewport(vp.x, vp.y, vp.w, vp.h);
    qEnable(GL_SCISSOR_TEST);
    qScissor(vp.x, vp.y, vp.w, vp.h);
    qClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    qDisable(GL_SCISSOR_TEST);
    qDisable(GL_DEPTH_TEST);
    qUniform1i(d->layer, 1);
    draw(d->screen);
    qEnable(GL_DEPTH_TEST);
    qUniform1i(d->layer, 0);
    qBindTexture(GL_TEXTURE_2D, d->atlas);
    draw(d->land);
    draw(d->life);
}
static void draw_house(Display *d) {
    qUniform1i(d->house_pass, 1);
    draw(d->house);
    qUniform1i(d->house_pass, 0);
}
static void sunlight_maps(Display *d, const Game *g, Vec centre, Vec sun) {
    if (sun.y <= 0)
        return;
    bool changed = d->shadow_notes != g->notes || d->shadow_weather != g->weather;
    d->shadow_notes = g->notes;
    d->shadow_weather = g->weather;
    bool offscreen = false;
#ifndef __EMSCRIPTEN__
    GLint previous = 0;
#endif
    for (int i = 0; i < 2; i++) {
        ShadowMap *map = &d->shadows[i];
        float radius = i ? 1700.f : (g->cabin.inside ? 20.f : 66.f);
        bool static_changed =
            !map->valid || fabsf(g->hour - map->hour) >= (i ? .035f : .010f) ||
            hypotf(centre.x - map->centre.x, centre.z - map->centre.z) >= radius * .09f ||
            fabsf(centre.y - map->centre.y) >= radius * .09f;
        if (!static_changed && (i || (!changed && fabs(g->elapsed - map->time) < .14)))
            continue;
        if (!offscreen) {
#ifdef __EMSCRIPTEN__
            save_browser_target();
#else
            qGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
#endif
            offscreen = true;
            qBindFramebuffer(GL_FRAMEBUFFER, d->shadow_fb);
            qUseProgram(d->shadow_program);
            qUniform3f(d->shadow_pattern, (float)(g->world.ox % 4096), 0,
                       (float)(g->world.oz % 4096));
            qUniform1f(d->shadow_clock, (float)g->elapsed);
            qBindTexture(GL_TEXTURE_2D, d->atlas);
            qEnable(GL_DEPTH_TEST);
            qDisable(GL_SCISSOR_TEST);
            qEnable(GL_POLYGON_OFFSET_FILL);
            qPolygonOffset(2, 3);
        }
        int n = map->size;
        qBindFramebuffer(GL_FRAMEBUFFER, d->shadow_fb);
        qFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, map->texture, 0);
        qViewport(0, 0, n, n);
        if (static_changed) {
            map->matrix = shadow_matrix(&g->world, centre, sun, radius, n);
            qUniformMatrix4fv(d->shadow_vp, 1, GL_FALSE, map->matrix.m);
            if (!i)
                qBindFramebuffer(GL_FRAMEBUFFER, d->static_fb);
            qClear(GL_DEPTH_BUFFER_BIT);
            draw(d->land);
            draw(d->house);
            map->centre = centre;
            map->hour = g->hour;
            d->lake_time = -100;
        }
        if (!i) {
            qBindFramebuffer(GL_READ_FRAMEBUFFER, d->static_fb);
            qBindFramebuffer(GL_DRAW_FRAMEBUFFER, d->shadow_fb);
            qBlitFramebuffer(0, 0, n, n, 0, 0, n, n, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            qUniformMatrix4fv(d->shadow_vp, 1, GL_FALSE, map->matrix.m);
            draw(d->life);
        }
        map->time = g->elapsed;
        map->valid = true;
    }
    if (offscreen) {
        qDisable(GL_POLYGON_OFFSET_FILL);
#ifdef __EMSCRIPTEN__
        restore_browser_target();
#else
        qBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous);
#endif
        qUseProgram(d->program);
        qUniformMatrix4fv(d->near_light, 1, GL_FALSE, d->shadows[0].matrix.m);
        qUniformMatrix4fv(d->far_light, 1, GL_FALSE, d->shadows[1].matrix.m);
    }
}
static void lake_reflection(Display *d, const Game *g, Viewport vp, Mat projection, Mat view) {
#ifdef __EMSCRIPTEN__
    save_browser_target();
#else
    GLint previous = 0;
    qGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
#endif
    bool fresh = !d->lake_fb;
    if (fresh) {
        qGenFramebuffers(1, &d->lake_fb);
        qBindFramebuffer(GL_FRAMEBUFFER, d->lake_fb);
        qGenTextures(1, &d->lake_tex);
        qBindTexture(GL_TEXTURE_2D, d->lake_tex);
        texture_settings();
        qTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 240, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        qFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d->lake_tex, 0);
        qGenRenderbuffers(1, &d->lake_depth);
        qBindRenderbuffer(GL_RENDERBUFFER, d->lake_depth);
        qRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 240, 128);
        qFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                 d->lake_depth);
    }
    Mat scene = multiply(projection, view);
    if (fresh || fabs(g->elapsed - d->lake_time) > .22f || fabsf(g->hour - d->lake_hour) > .02f ||
        d->lake_seed != g->world.seed || d->lake_season != g->season ||
        d->lake_weather != g->weather || memcmp(&scene, &d->lake_view, sizeof scene)) {
        qBindFramebuffer(GL_FRAMEBUFFER, d->lake_fb);
        /* Keep the reflection attachment off both active sampler units. */
        qActiveTexture(GL_TEXTURE1);
        qBindTexture(GL_TEXTURE_2D, d->atlas);
        qActiveTexture(GL_TEXTURE0);
        qBindTexture(GL_TEXTURE_2D, d->atlas);
        Mat mirror = identity();
        mirror.m[5] = -1;
        mirror.m[13] = 2 * g->world.water;
        camera(d, projection, multiply(view, mirror));
        qUniform1i(d->reflect, 1);
        qUniform1f(d->water, g->world.water);
        outdoor(d, (Viewport){0, 0, 240, 128});
        if (!g->cabin.inside)
            draw_house(d);
        d->lake_time = g->elapsed;
        d->lake_hour = g->hour;
        d->lake_seed = g->world.seed;
        d->lake_season = g->season;
        d->lake_weather = g->weather;
        d->lake_view = scene;
    }
#ifdef __EMSCRIPTEN__
    restore_browser_target();
#else
    qBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous);
#endif
    qUniform1i(d->reflect, 0);
    qUniform4f(d->viewport, (float)vp.x, (float)vp.y, (float)vp.w, (float)vp.h);
    qActiveTexture(GL_TEXTURE1);
    qBindTexture(GL_TEXTURE_2D, d->lake_tex);
    qActiveTexture(GL_TEXTURE0);
}
static void headset_note(Display *d, const Game *g);
void display_draw(Display *d, Script *s, const Game *g, Viewport vp, const Mat *proj,
                  const Mat *head) {
    if (vp.w < 1 || vp.h < 1)
        return;
    scenery(d, s, g);
    Vec e = eye(g);
    Mat p = proj ? *proj : perspective(1.04f, (float)vp.w / vp.h, .08f, 2400);
    Vec room_eye = e;
    if (g->cabin.inside) {
        float lift = g->cabin.room ? 9.f + 2.f * (g->cabin.room % 13) : 1.0f;
        e.y = fmaxf(e.y, ground(g, e.x, e.z) + 1.65f) + lift;
    }
    Mat view = view_matrix(e, g->yaw, head ? 0 : g->pitch);
    if (head)
        view = multiply(*head, view);
    qUseProgram(d->program);
    float day = sunlight_factor(g->hour), warm = alpenglow(g->hour);
    Vec sun = sun_direction(g->hour);
    Lighting light = g->world.place ? g->world.place->lighting : earth_lighting();
    sunlight_maps(d, g, room_eye, sun);
    qUniform3f(d->sun, sun.x, sun.y, sun.z);
    qUniform3f(d->sky_fill, light.sky.r, light.sky.g, light.sky.b);
    qUniform3f(d->bounce, light.bounce.r, light.bounce.g, light.bounce.b);
    qUniform4f(d->light_params, light.clouds, light.softness, 1.f / d->shadows[0].size,
               1.f / d->shadows[1].size);
    qUniform3f(d->cloud_origin, (float)fmod(g->world.ox * (1.22 / 950.0), 256.0), 0,
               (float)fmod(g->world.oz * (2.32 / 950.0), 256.0));
    Color hor = blend(rgb(0x243047), blend(rgb(0xf2e9ce), rgb(0xfac79d), warm), day);
    Color zen = blend(rgb(0x121c32), blend(rgb(0x689cb3), rgb(0x899daf), warm * .61f), day);
    unsigned watch = g->cabin.inside ? g->book.weather : LOOKOUT_CALM;
    int weather = watch == LOOKOUT_FOG ? 3 : watch ? 2 : g->weather;
    qUniform1i(d->lookout, (int)watch);
    float grey = weather == 2 ? .67f : (weather == 3 ? .45f : (weather == 1 ? .22f : 0));
    hor = blend(hor, shade(rgb(0xacb7b6), .36f + .64f * day), grey);
    zen = blend(zen, shade(rgb(0x86979c), .35f + .65f * day), grey);
    const Destination *destination = g->world.place;
    Profile profile = g->world.profile;
    if (profile > SURVEYED) {
        Color sky = destination->colours[4];
        zen = blend(shade(sky, .19f), sky, .80f * day);
        hor = blend(shade(sky, .31f),
                    blend(sky, profile >= PRISM ? destination->colours[3] : rgb(0xffe5c1), .53f),
                    day);
        if (profile == LUNAR) {
            zen = rgb(0x03050d);
            hor = rgb(0x101322);
            grey = 0;
        } else {
            hor = blend(hor, shade(sky, .80f), grey * .4f);
            zen = blend(zen, hor, grey * .45f);
        }
    }
    Color liquid = destination ? destination->colours[5] : rgb(0x36757b);
    Color soil = destination ? destination->colours[0] : rgb(0xa79876);
    qUniform1i(d->profile, profile);
    qUniform3f(d->liquid, liquid.r, liquid.g, liquid.b);
    qUniform3f(d->soil, soil.r, soil.g, soil.b);
    qUniform3f(d->horizon, hor.r, hor.g, hor.b);
    qUniform3f(d->zenith, zen.r, zen.g, zen.b);
    qUniform4f(d->air, g->hour, day, warm, profile == LUNAR && !watch ? 0 : (float)weather);
    qUniform1f(d->clock, g->elapsed);
    qUniform1f(d->season_tint, (float)g->season);
    if (!d->lake_tex || (profile != LUNAR && profile != PRISM))
        lake_reflection(d, g, vp, p, view);
    camera(d, p, view);
    outdoor(d, vp);
    if (g->cabin.inside) {
        /* The windows borrow the summit view. Keep outdoor depth out of the room. */
        qEnable(GL_SCISSOR_TEST);
        qScissor(vp.x, vp.y, vp.w, vp.h);
        qClear(GL_DEPTH_BUFFER_BIT);
        qDisable(GL_SCISSOR_TEST);
        view = view_matrix(room_eye, g->yaw, head ? 0 : g->pitch);
        if (head)
            view = multiply(*head, view);
        camera(d, p, view);
    }
    draw_house(d);
    if (head)
        headset_note(d, g);
}
static Vec pixel(float x, float y, int w, int h) { return (Vec){x * 2 / w - 1, 1 - y * 2 / h, 0}; }
static void label(Mesh *m, const char *text, int x, int y, int w, int h, Color col) {
    for (const unsigned char *p = (const unsigned char *)text; *p; p++, x += 6) {
        unsigned ch = *p < 128 ? *p : '?';
        float u = (ch % 16) / 16.f, v = (ch / 16) / 8.f;
        tex_quad(m, pixel(x, y + 8, w, h), pixel(x + 6, y + 8, w, h), pixel(x + 6, y, w, h),
                 pixel(x, y, w, h), u, v, u + 1 / 16.f, v + 1 / 8.f, col, 1);
    }
}
static void note_lines(Mesh *m, const char *note, int left, int start, int width, int w, int h,
                       Color ink, Color cream) {
    for (int row = 0; *note && row < 5 && width > 4; row++) {
        int n = (int)strlen(note);
        if (n > width) {
            n = width;
            while (n > 1 && note[n] != ' ')
                n--;
            if (n == 1)
                n = width;
        }
        char line[108];
        memcpy(line, note, n);
        line[n] = 0;
        quad(m, pixel(left, start + row * 10, w, h),
             pixel(left + 12 + n * 6, start + row * 10, w, h),
             pixel(left + 12 + n * 6, start + row * 10 + 10, w, h),
             pixel(left, start + row * 10 + 10, w, h), ink, 0);
        label(m, line, left + 6, start + row * 10 + 1, w, h, cream);
        note += n;
        while (*note == ' ')
            note++;
    }
}
static void finish_hud(Display *d, Mesh *m) {
    upload(&d->hud, m, GL_DYNAMIC_DRAW);
    qDisable(GL_DEPTH_TEST);
    qBindTexture(GL_TEXTURE_2D, d->font);
    qUniform1i(d->layer, 2);
    draw(d->hud);
}
static void headset_note(Display *d, const Game *g) {
    Interaction nearby = game_near(g);
    const char *note = NULL;
    if (g->fall_time > 0 || g->exposed)
        note = "Exposed edge - watch your footing";
    else if (g->warning_timer > 0)
        note = "Back on safe footing";
    else if (g->note_timer > 0)
        note = field_hint(g);
    else if (nearby == NEAR_ENTER || nearby == NEAR_EXIT)
        note = "Select to cross; left grip closes door";
    else if (nearby == NEAR_READ)
        note = "Select to read the field sign";
    else if (nearby == NEAR_TREASURE)
        note = "Select to discover the treasure";
    if (!note || !*note)
        return;
    Mesh *m = &d->scratch;
    mesh_clear(m);
    note_lines(m, note, 126, 279, 38, 500, 400, rgb(0x233a31), rgb(0xf0edda));
    finish_hud(d, m);
}
static void native_hud(Display *d, Script *s, const Game *g, int w, int h, bool help) {
    Mesh *m = &d->scratch;
    mesh_clear(m);
    char title[144], line[144];
    int time = (int)(g->hour * 60);
    snprintf(title, sizeof title, "%s", g->cabin.inside ? cabin_layout(g)->name : trail_name(g));
    snprintf(line, sizeof line, "%02d:%02d   %s   %s", time / 60, time % 60,
             g->rules->seasons[g->season], weather_label(g, g->weather));
    if (g->cabin.inside) {
        const char *weather[] = {"Calm", "Thunderstorm", "Snowstorm", "Heavy fog"};
        snprintf(line, sizeof line, "%02d:%02d   %s%s%s", time / 60, time % 60,
                 weather[g->book.weather], g->book.predators ? "  Wolves" : "",
                 g->book.giants ? "  Giant" : "");
    }
    const char *actions[] = {"WASD HIKE  /  P PHOTO  /  H CABIN  /  F1 GUIDE",
                             "E  OPEN DOOR",
                             "E  STEP INSIDE  /  F  CLOSE DOOR",
                             "E  STEP OUTSIDE  /  F  CLOSE DOOR",
                             "E  NEXT ROOM  /  F  CLOSE DOOR",
                             "E  PREVIOUS ROOM  /  F  CLOSE DOOR",
                             "E  READ FIELD SIGN",
                             "E  DISCOVER TREASURE",
                             "E  OPEN ROOM DOOR",
                             "E  OPEN WINDOW  /  J  ADJUST BLIND",
                             "E  CLOSE WINDOW  /  J  ADJUST BLIND",
                             "E  OPEN BALCONY DOOR",
                             "E  CROSS TO OTHER SIDE  /  F  CLOSE DOOR"};
    Color ink = rgb(0x233a31), cream = rgb(0xf0edda);
    quad(m, pixel(6, 7, w, h), pixel(200, 7, w, h), pixel(200, 32, w, h), pixel(6, 32, w, h), ink,
         0);
    label(m, title, 12, 10, w, h, cream);
    label(m, line, 12, 21, w, h, cream);
    const char *a = g->fall_time > 0       ? "SLIPPING"
                    : g->warning_timer > 0 ? "BACK ON SAFE FOOTING"
                    : g->exposed           ? "EXPOSED EDGE - WATCH YOUR FOOTING"
                                           : actions[game_near(g)];
    quad(m, pixel(6, h - 20, w, h), pixel(18 + strlen(a) * 6, h - 20, w, h),
         pixel(18 + strlen(a) * 6, h - 6, w, h), pixel(6, h - 6, w, h), ink, 0);
    label(m, a, 12, h - 17, w, h, cream);
    if (help) {
        const char *lines[] = {"FIELD GUIDE",
                               "WASD or arrows: hike / turn",
                               "Drag or left click: look",
                               "H: summon / dismiss cabin",
                               "E use   F door   J window blind",
                               "R return to Hearth Room",
                               "K shelter weather  L wolves  X giant",
                               "T: time   Y: season   U: weather",
                               "I: cycle seasons   O: weather changes",
                               "N: new trail   B: Taggart Lake",
                               "[ / ]: previous / next destination",
                               "M: sound   P: photo",
                               "F1: close guide   Esc: release mouse"};
        quad(m, pixel(6, 36, w, h), pixel(244, 36, w, h), pixel(250, 184, w, h),
             pixel(6, 184, w, h), ink, 0);
        for (int i = 0; i < (int)(sizeof lines / sizeof *lines); i++)
            label(m, lines[i], 12, 43 + i * 10, w, h, cream);
        int found = 0;
        for (int i = 0; i < g->rules->place_count; i++)
            if (((g->notes >> (2 * i)) & 3u) == 3) {
                if (w >= 455)
                    note_lines(m, g->rules->places[i].signs[2], 254, 48 + found * 10, 45, w, h, ink,
                               cream);
                found++;
            }
        if (g->rules->place_count) {
            char total[64];
            snprintf(total, sizeof total, "Treasures found  %d / %d", found, g->rules->place_count);
            note_lines(m, total, w >= 455 ? 254 : 6, w >= 455 ? 36 : 161, 40, w, h, ink, cream);
        }
    }
    if ((help || g->note_timer > 0) && g->world.place) {
        const char *note = field_hint(g);
        int start = help ? 177 : 36, width = (w - 24) / 6;
        if (width > 105)
            width = 105;
        note_lines(m, note, 6, start, width, w, h, ink, cream);
    }
    finish_hud(d, m);
}
static bool target(Display *d, int w, int h) {
    if (!d->fb) {
        qGenFramebuffers(1, &d->fb);
        qGenRenderbuffers(1, &d->depth);
        qGenTextures(1, &d->frame);
    }
    qBindFramebuffer(GL_FRAMEBUFFER, d->fb);
    if (w == d->fw && h == d->fh)
        return true;
    qBindTexture(GL_TEXTURE_2D, d->frame);
    texture_settings();
    qTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    qFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d->frame, 0);
    qBindRenderbuffer(GL_RENDERBUFFER, d->depth);
    qRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    qFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, d->depth);
    d->fw = w;
    d->fh = h;
    return qCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}
void display_native(Display *d, Script *s, const Game *g, int w, int h, bool help, bool clean) {
    if (w < 1 || h < 1)
        return;
    int low_h = h < 360 ? h : 360, low_w = (int)fmaxf(1, (float)w * low_h / h);
    if (target(d, low_w, low_h)) {
        display_draw(d, s, g, (Viewport){0, 0, low_w, low_h}, NULL, NULL);
        if (!clean)
            native_hud(d, s, g, low_w, low_h, help);
    }
    qBindFramebuffer(GL_FRAMEBUFFER, 0);
    qViewport(0, 0, w, h);
    qDisable(GL_DEPTH_TEST);
    qUniform1i(d->layer, 3);
    qBindTexture(GL_TEXTURE_2D, d->frame);
    draw(d->screen);
}
bool display_save(Display *d, Script *s, const Game *g, const char *path) {
    int w = d->fw ? d->fw : 640, h = d->fh ? d->fh : 360;
    if (!target(d, w, h))
        return false;
    display_draw(d, s, g, (Viewport){0, 0, w, h}, NULL, NULL);
    SDL_Surface *surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        qBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    qReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_FlipSurface(surface, SDL_FLIP_VERTICAL);
    bool ok = SDL_SaveBMP(surface, path);
    SDL_DestroySurface(surface);
    qBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}
uint32_t display_sample(int w, int h) {
    unsigned char px[4];
    uint32_t v = 2166136261u;
    for (int y = 1; y < 10; y++)
        for (int x = 1; x < 12; x++) {
            qReadPixels(w * x / 12, h * y / 10, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
            for (int i = 0; i < 3; i++)
                v = (v ^ px[i]) * 16777619u;
        }
    return v;
}
