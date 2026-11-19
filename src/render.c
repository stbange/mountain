#include "render.h"
#include <SDL3/SDL_opengl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Core GL 2 / GLES 2 operations, resolved from the active SDL context. */
#define FUNCTIONS(X)                                                                               \
    X(GLuint, CreateShader, (GLenum))                                                              \
    X(void, ShaderSource, (GLuint, GLsizei, const char *const *, const GLint *)) X(                \
        void, CompileShader, (GLuint)) X(void, GetShaderiv, (GLuint, GLenum, GLint *))             \
        X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, char *)) X(                         \
            GLuint, CreateProgram, (void)) X(void, AttachShader,                                   \
                                             (GLuint, GLuint)) X(void, BindAttribLocation,         \
                                                                 (GLuint, GLuint, const char *))   \
            X(void, LinkProgram, (GLuint)) X(void, GetProgramiv, (GLuint, GLenum, GLint *)) X(     \
                void, GetProgramInfoLog,                                                           \
                (GLuint, GLsizei, GLsizei *,                                                       \
                 char *)) X(void, UseProgram, (GLuint)) X(void, DeleteShader, (GLuint))            \
                X(void, DeleteProgram, (GLuint)) X(                                                \
                    GLint, GetUniformLocation,                                                     \
                    (GLuint,                                                                       \
                     const char                                                                    \
                         *)) X(void, UniformMatrix4fv,                                             \
                               (GLint, GLsizei, GLboolean, const GLfloat *))                       \
                    X(void, Uniform1i, (GLint, GLint)) X(void, Uniform1f, (GLint, GLfloat)) X(     \
                        void,                                                                      \
                        Uniform3f,                                                                 \
                        (GLint, GLfloat, GLfloat,                                                  \
                         GLfloat)) X(void, Uniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
                        X(void, GenBuffers, (GLsizei, GLuint *)) X(                                \
                            void,                                                                  \
                            DeleteBuffers,                                                         \
                            (GLsizei, const GLuint *)) X(void, BindBuffer, (GLenum, GLuint))       \
                            X(void, BufferData, (GLenum, ptrdiff_t, const void *, GLenum)) X(      \
                                void,                                                              \
                                VertexAttribPointer,                                               \
                                (GLuint, GLint, GLenum, GLboolean, GLsizei,                        \
                                 const void *)) X(void, EnableVertexAttribArray, (GLuint))         \
                                X(void, DrawArrays, (GLenum, GLint, GLsizei)) X(                   \
                                    void,                                                          \
                                    GenTextures,                                                   \
                                    (GLsizei, GLuint *)) X(void, BindTexture, (GLenum, GLuint))    \
                                    X(void, TexParameteri, (GLenum, GLenum, GLint)) X(             \
                                        void,                                                      \
                                        TexImage2D,                                                \
                                        (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,    \
                                         GLenum,                                                   \
                                         const void                                                \
                                             *)) X(void, DeleteTextures,                           \
                                                   (GLsizei, const GLuint *))                      \
                                        X(void, Viewport, (GLint, GLint, GLsizei, GLsizei)) X(     \
                                            void,                                                  \
                                            Scissor,                                               \
                                            (GLint, GLint, GLsizei,                                \
                                             GLsizei)) X(void, Enable,                             \
                                                         (GLenum)) X(void, Disable, (GLenum))      \
                                            X(void, Clear, (GLbitfield)) X(                        \
                                                void,                                              \
                                                ReadPixels,                                        \
                                                (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,   \
                                                 void *)) X(void, ActiveTexture, (GLenum))         \
                                                X(void, GetIntegerv, (GLenum, GLint *)) X(         \
                                                    void,                                          \
                                                    GenFramebuffers,                               \
                                                    (GLsizei, GLuint *))                           \
                                                    X(void, BindFramebuffer, (GLenum, GLuint)) X(  \
                                                        void,                                      \
                                                        FramebufferTexture2D,                      \
                                                        (GLenum, GLenum, GLenum, GLuint,           \
                                                         GLint)) X(void, DeleteFramebuffers,       \
                                                                   (GLsizei, const GLuint *))      \
                                                        X(void, GenRenderbuffers,                  \
                                                          (                                        \
                                                              GLsizei,                             \
                                                              GLuint *)) X(void, BindRenderbuffer, \
                                                                           (GLenum, GLuint))       \
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
#define DECL(ret, name, args) static ret(APIENTRY *q##name) args;
FUNCTIONS(DECL)
typedef struct {
    GLuint id;
    GLsizei count;
} Buffer;
struct Display {
    GLuint program, atlas, font, fb, frame, depth, lake_fb, lake_tex, lake_depth;
    GLint reflect, water, viewport;
    Mat lake_view;
    float lake_time, lake_hour;
    int lake_season, lake_weather;
    uint32_t lake_seed;
    GLint vp, inv, eye, layer, air, zenith, horizon, sun, clock;
    Buffer land, house, life, screen, hud;
    Mesh scratch;
    int cx, cz, season, fw, fh;
    uint32_t seed;
    Cabin cached;
    bool valid;
};
static bool resolve(void) {
#define LOAD(ret, name, args)                                                                      \
    q##name = (ret(APIENTRY *) args)SDL_GL_GetProcAddress("gl" #name);                             \
    if (!q##name) {                                                                                \
        SDL_Log("Missing gl%s", #name);                                                            \
        return false;                                                                              \
    }
    FUNCTIONS(LOAD)
#undef LOAD
    return true;
}
static GLuint shader(GLenum type, const char *root, const char *name) {
    char path[1024];
    snprintf(path, sizeof path, "%s/shaders/%s", root, name);
    char *body = SDL_LoadFile(path, NULL);
    if (!body)
        return 0;
#ifdef __EMSCRIPTEN__
    const char *head = "#version 100\nprecision highp float;\nprecision highp int;\n";
#else
    const char *head = "#version 120\n";
#endif
    const char *parts[] = {head, body};
    GLuint id = qCreateShader(type);
    qShaderSource(id, 2, parts, NULL);
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
    if (n >= 18 && raw[2] == 2 && raw[16] == 32) {
        w = raw[12] + 256 * raw[13];
        h = raw[14] + 256 * raw[15];
    }
    if (w == 0 || h == 0 || n - 18 != (size_t)w * h * 4) {
        SDL_free(raw);
        return 0;
    }
    for (size_t i = 18; i < n; i += 4) {
        unsigned char b = raw[i];
        raw[i] = raw[i + 2];
        raw[i + 2] = b;
    }
    GLuint id;
    qGenTextures(1, &id);
    qBindTexture(GL_TEXTURE_2D, id);
    texture_settings();
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
Display *display_open(const char *root) {
    if (!resolve())
        return NULL;
    Display *d = calloc(1, sizeof *d);
    if (!d)
        return NULL;
    GLuint v = shader(GL_VERTEX_SHADER, root, "ridge.vert"),
           f = shader(GL_FRAGMENT_SHADER, root, "ridge.frag");
    if (v && f) {
        d->program = qCreateProgram();
        qAttachShader(d->program, v);
        qAttachShader(d->program, f);
        qBindAttribLocation(d->program, 0, "a_pos");
        qBindAttribLocation(d->program, 1, "a_colour");
        qBindAttribLocation(d->program, 2, "a_uvm");
        qLinkProgram(d->program);
    }
    if (v)
        qDeleteShader(v);
    if (f)
        qDeleteShader(f);
    GLint ok = 0;
    if (d->program)
        qGetProgramiv(d->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        if (d->program) {
            char msg[600];
            qGetProgramInfoLog(d->program, sizeof msg, NULL, msg);
            SDL_Log("%s", msg);
        }
        display_close(d);
        return NULL;
    }
    qUseProgram(d->program);
#define LOC(field, name) d->field = qGetUniformLocation(d->program, "u_" name)
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
    LOC(viewport, "viewport");
    qUniform1i(qGetUniformLocation(d->program, "u_atlas"), 0);
    qUniform1i(qGetUniformLocation(d->program, "u_lake"), 1);
    d->atlas = texture(root, "field.tga");
    d->font = texture(root, "letters.tga");
    if (!d->atlas || !d->font) {
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
    GLuint tex[] = {d->atlas, d->font, d->frame, d->lake_tex};
    qDeleteBuffers(5, buffers);
    qDeleteTextures(4, tex);
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
    bool site =
        c->present != old->present || c->x != old->x || c->z != old->z || c->angle != old->angle;
    int cx = (int)floorf((g->x + 14) / 28) * 28, cz = (int)floorf((g->z + 14) / 28) * 28;
    if (!d->valid || d->seed != g->world.seed || d->season != g->season || cx != d->cx ||
        cz != d->cz || site) {
        landscape(&d->scratch, g, (float)cx, (float)cz);
        upload(&d->land, &d->scratch, GL_STATIC_DRAW);
        d->cx = cx;
        d->cz = cz;
        d->seed = g->world.seed;
        d->season = g->season;
    }
    if (!d->valid || site || old->open != c->open || old->inside != c->inside ||
        old->room != c->room) {
        cabin_mesh(&d->scratch, s, g);
        upload(&d->house, &d->scratch, GL_STATIC_DRAW);
    }
    d->cached = *c;
    d->valid = true;
    wildlife(&d->scratch, g);
    upload(&d->life, &d->scratch, GL_DYNAMIC_DRAW);
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
static void lake_reflection(Display *d, const Game *g, Viewport vp, Mat projection, Mat view) {
    GLint previous = 0;
    qGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
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
    if (fresh || fabsf(g->elapsed - d->lake_time) > .22f || fabsf(g->hour - d->lake_hour) > .02f ||
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
        d->lake_time = g->elapsed;
        d->lake_hour = g->hour;
        d->lake_seed = g->world.seed;
        d->lake_season = g->season;
        d->lake_weather = g->weather;
        d->lake_view = scene;
    }
    qBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous);
    qUniform1i(d->reflect, 0);
    qUniform4f(d->viewport, (float)vp.x, (float)vp.y, (float)vp.w, (float)vp.h);
    qActiveTexture(GL_TEXTURE1);
    qBindTexture(GL_TEXTURE_2D, d->lake_tex);
    qActiveTexture(GL_TEXTURE0);
}
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
    float day = sunlight_factor(g->hour), warm = alpenglow(g->hour),
          angle = (g->hour - 6) * PI / 12;
    qUniform3f(d->sun, cosf(angle) * .82f, sinf(angle), cosf(angle) * .572f);
    Color hor = blend(rgb(0x243047), blend(rgb(0xf2e9ce), rgb(0xfac79d), warm), day);
    Color zen = blend(rgb(0x121c32), blend(rgb(0x689cb3), rgb(0x899daf), warm * .61f), day);
    float grey = g->weather == 2 ? .67f : (g->weather == 3 ? .45f : (g->weather == 1 ? .22f : 0));
    hor = blend(hor, shade(rgb(0xacb7b6), .36f + .64f * day), grey);
    zen = blend(zen, shade(rgb(0x86979c), .35f + .65f * day), grey);
    qUniform3f(d->horizon, hor.r, hor.g, hor.b);
    qUniform3f(d->zenith, zen.r, zen.g, zen.b);
    qUniform4f(d->air, g->hour, day, warm, (float)g->weather);
    qUniform1f(d->clock, g->elapsed);
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
    draw(d->house);
}
static Vec pixel(float x, float y, int w, int h) {
    return (Vec){x * 2 / w - 1, 1 - y * 2 / h, 0};
}
static void label(Mesh *m, const char *text, int x, int y, int w, int h, Color col) {
    for (const unsigned char *p = (const unsigned char *)text; *p; p++, x += 6) {
        unsigned ch = *p < 128 ? *p : '?';
        float u = (ch % 16) / 16.f, v = (ch / 16) / 8.f;
        tex_quad(m, pixel(x, y + 8, w, h), pixel(x + 6, y + 8, w, h), pixel(x + 6, y, w, h),
                 pixel(x, y, w, h), u, v, u + 1 / 16.f, v + 1 / 8.f, col, 1);
    }
}
static void native_hud(Display *d, Script *s, const Game *g, int w, int h, bool help) {
    Mesh *m = &d->scratch;
    mesh_clear(m);
    char title[144], line[144];
    int time = (int)(g->hour * 60);
    snprintf(title, sizeof title, "%s",
             g->cabin.inside ? room_name(s, g->cabin.room) : trail_name(g));
    snprintf(line, sizeof line, "%02d:%02d   %s   %s", time / 60, time % 60,
             g->rules->seasons[g->season], g->rules->skies[g->weather]);
    const char *actions[] = {"WASD HIKE  /  P PHOTO  /  H CABIN  /  F1 GUIDE",
                             "E  OPEN DOOR",
                             "E  STEP INSIDE",
                             "E  STEP OUTSIDE",
                             "E  NEXT ROOM",
                             "E  PREVIOUS ROOM"};
    Color ink = rgb(0x233a31), cream = rgb(0xf0edda);
    quad(m, pixel(6, 7, w, h), pixel(200, 7, w, h), pixel(200, 32, w, h), pixel(6, 32, w, h), ink,
         0);
    label(m, title, 12, 10, w, h, cream);
    label(m, line, 12, 21, w, h, cream);
    const char *a = actions[game_near(g)];
    quad(m, pixel(6, h - 20, w, h), pixel(18 + strlen(a) * 6, h - 20, w, h),
         pixel(18 + strlen(a) * 6, h - 6, w, h), pixel(6, h - 6, w, h), ink, 0);
    label(m, a, 12, h - 17, w, h, cream);
    if (help) {
        const char *lines[] = {"FIELD GUIDE",
                               "WASD or arrows: hike / turn",
                               "Drag or left click: look",
                               "H: summon / dismiss cabin",
                               "E: open / use nearest door",
                               "R: return to hearth room",
                               "T: time   Y: season   U: weather",
                               "N: new trail   M: sound   P: photo",
                               "F1: close guide   Esc: release mouse"};
        quad(m, pixel(6, 36, w, h), pixel(244, 36, w, h), pixel(244, 139, w, h),
             pixel(6, 139, w, h), ink, 0);
        for (int i = 0; i < 9; i++)
            label(m, lines[i], 12, 43 + i * 10, w, h, cream);
    }
    upload(&d->hud, m, GL_DYNAMIC_DRAW);
    qDisable(GL_DEPTH_TEST);
    qBindTexture(GL_TEXTURE_2D, d->font);
    qUniform1i(d->layer, 2);
    draw(d->hud);
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
