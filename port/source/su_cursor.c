/* su_cursor.c -- the blue pointer cursor: an arrow drawn over the game's frame
 * (from the Costume Quest port, costumequest_nx's cq_cursor.c).
 *
 * The game is made for a finger. The left stick moves this cursor and A taps
 * with it (dcr_input.c turns that into a touch). It is always on screen: flicks
 * start where it is.
 *
 * The runtime calls dcr_present_hook just before each swap, on the engine's
 * thread with its OpenGL ES context current. The game's own state (program,
 * vertex array and buffer, blending and the tests, the framebuffer) is read
 * first and put back after: the arrow has its own program and vertex array. MIT.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "su_cursor.h"
#include "gl_layer.h"
#include "rt_window.h"
#include "util.h"

#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FLOAT 0x1406
#define GL_TRIANGLES 0x0004
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_FRAMEBUFFER 0x8D40
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_BLEND 0x0BE2
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44
#define GL_SCISSOR_TEST 0x0C11
#define GL_STENCIL_TEST 0x0B90
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_SAMPLES 0x80A9

/* what the game thread tells the render thread (floats and ints: tearing is harmless) */
static volatile float g_x, g_y;       /* pixels of the surface */
static volatile int g_w, g_h, g_down; /* the surface's size, 1 while a tap is held */
static volatile u64 g_seen;           /* tick of the last move or tap; 0: never */
static volatile int g_enabled;

void su_cursor_set(float x, float y, int w, int h, int down, int touched) {
  g_x = x, g_y = y, g_w = w, g_h = h, g_down = down;
  if (touched)
    g_seen = armGetSystemTick();
  g_enabled = 1;
}

/* ------------------------------------------------------------------ GL */
static struct {
  void (*GetIntegerv)(unsigned, int *);
  void (*GetBooleanv)(unsigned, unsigned char *);
  unsigned char (*IsEnabled)(unsigned);
  void (*Enable)(unsigned);
  void (*Disable)(unsigned);
  unsigned (*CreateShader)(unsigned);
  void (*ShaderSource)(unsigned, int, const char *const *, const int *);
  void (*CompileShader)(unsigned);
  void (*GetShaderiv)(unsigned, unsigned, int *);
  unsigned (*CreateProgram)(void);
  void (*AttachShader)(unsigned, unsigned);
  void (*BindAttribLocation)(unsigned, unsigned, const char *);
  void (*LinkProgram)(unsigned);
  void (*GetProgramiv)(unsigned, unsigned, int *);
  void (*DeleteShader)(unsigned);
  int (*GetUniformLocation)(unsigned, const char *);
  void (*UseProgram)(unsigned);
  void (*Uniform2f)(int, float, float);
  void (*Uniform4f)(int, float, float, float, float);
  void (*GenBuffers)(int, unsigned *);
  void (*BindBuffer)(unsigned, unsigned);
  void (*BufferData)(unsigned, long, const void *, unsigned);
  void (*GenVertexArrays)(int, unsigned *);
  void (*BindVertexArray)(unsigned);
  void (*EnableVertexAttribArray)(unsigned);
  void (*VertexAttribPointer)(unsigned, int, unsigned, unsigned char, int, const void *);
  void (*DrawArrays)(unsigned, int, int);
  void (*BlendFuncSeparate)(unsigned, unsigned, unsigned, unsigned);
  void (*BindFramebuffer)(unsigned, unsigned);
  void (*Viewport)(int, int, int, int);
  void (*Scissor)(int, int, int, int);
  void (*ColorMask)(unsigned char, unsigned char, unsigned char, unsigned char);
  void (*ClearColor)(float, float, float, float);
  void (*Clear)(unsigned);
  void (*GetFloatv)(unsigned, float *);
  unsigned (*GetError)(void);
 int ok;
} gl;

static unsigned g_prog, g_vao, g_vbo;
static int g_u_res, g_u_col;

static const char *const k_vs =
    "attribute vec2 p;\n"
    "uniform vec2 res;\n"
    "void main() { gl_Position = vec4(p.x / res.x * 2.0 - 1.0, 1.0 - p.y / res.y * 2.0, 0.0, 1.0); }\n";
static const char *const k_fs =
    "precision mediump float;\n"
    "uniform vec4 col;\n"
    "void main() { gl_FragColor = col; }\n";

static int gl_setup(void) {
#define L(n) gl.n = (void *)dcr_gl_lookup("gl" #n)
  L(GetIntegerv); L(GetBooleanv); L(IsEnabled); L(Enable); L(Disable); L(CreateShader); L(ShaderSource);
  L(CompileShader); L(GetShaderiv); L(CreateProgram); L(AttachShader); L(BindAttribLocation); L(LinkProgram);
  L(GetProgramiv); L(DeleteShader); L(GetUniformLocation); L(UseProgram); L(Uniform2f); L(Uniform4f);
  L(GenBuffers); L(BindBuffer); L(BufferData); L(GenVertexArrays); L(BindVertexArray); L(EnableVertexAttribArray);
  L(VertexAttribPointer); L(DrawArrays); L(BlendFuncSeparate); L(BindFramebuffer);
  L(Viewport); L(Scissor); L(ColorMask); L(ClearColor); L(Clear); L(GetFloatv); L(GetError);
#undef L
  /* an OpenGL ES 2 context (Unity 5.3 may ask for one): the OES names */
  if (!gl.GenVertexArrays)
    gl.GenVertexArrays = (void *)dcr_gl_lookup("glGenVertexArraysOES");
  if (!gl.BindVertexArray)
    gl.BindVertexArray = (void *)dcr_gl_lookup("glBindVertexArrayOES");
  if (!gl.GetIntegerv || !gl.CreateShader || !gl.GenVertexArrays || !gl.BindVertexArray || !gl.BlendFuncSeparate ||
      !gl.BindFramebuffer || !gl.UseProgram || !gl.DrawArrays) {
    debugPrintf("[cursor] a GL function is missing: no cursor\n");
    return -1;
  }
  unsigned sh[2];
  const char *src[2] = {k_vs, k_fs};
  const unsigned type[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  g_prog = gl.CreateProgram();
  for (int i = 0; i < 2; i++) {
    sh[i] = gl.CreateShader(type[i]);
    gl.ShaderSource(sh[i], 1, &src[i], NULL);
    gl.CompileShader(sh[i]);
    int ok = 0;
    gl.GetShaderiv(sh[i], GL_COMPILE_STATUS, &ok);
    if (!ok) {
      debugPrintf("[cursor] the shader does not compile: no cursor\n");
      return -1;
    }
    gl.AttachShader(g_prog, sh[i]);
  }
  gl.BindAttribLocation(g_prog, 0, "p");
  gl.LinkProgram(g_prog);
  int ok = 0;
  gl.GetProgramiv(g_prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    debugPrintf("[cursor] the program does not link: no cursor\n");
    return -1;
  }
  gl.DeleteShader(sh[0]);
  gl.DeleteShader(sh[1]);
  g_u_res = gl.GetUniformLocation(g_prog, "res");
  g_u_col = gl.GetUniformLocation(g_prog, "col");
  gl.GenVertexArrays(1, &g_vao);
  gl.GenBuffers(1, &g_vbo);
  int old_vao = 0, old_buf = 0;
  gl.GetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
  gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_buf);
  gl.BindVertexArray(g_vao);
  gl.BindBuffer(GL_ARRAY_BUFFER, g_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
  gl.EnableVertexAttribArray(0);
  gl.VertexAttribPointer(0, 2, GL_FLOAT, 0, 0, NULL);
  gl.BindVertexArray((unsigned)old_vao);
  gl.BindBuffer(GL_ARRAY_BUFFER, (unsigned)old_buf);
  return 0;
}

/* BARS. The Switch ignored the display's own keep-the-shape setting (hardware
 * 2026-11-05: it stretched the picture across the screen), so the buffers are
 * always screen-shaped and the game is told it has a portrait window. Copying
 * its finished picture to the middle gave a black screen (hardware, same day),
 * so instead the game draws in the middle to begin with: its viewport, scissor
 * and blits to the screen are shifted by the width of a bar. Offscreen
 * framebuffers are left alone. */
static void (*r_Viewport)(int, int, int, int);
static void (*r_Scissor)(int, int, int, int);
static void (*r_BindFramebuffer)(unsigned, unsigned);
static void (*r_BlitFramebuffer)(int, int, int, int, int, int, int, int, unsigned, unsigned);
static unsigned g_draw_fb, g_read_fb;

/* SIZE. Minus steps through these widths. The height stays, so the game fills
 * more of the screen by getting wider. Only the shift's rectangles change, so
 * it costs nothing and no restart. */
static const float k_stretch[] = {1.0f, 1.3f, 1.6f, 2.0f};
#define STRETCHES ((int)(sizeof k_stretch / sizeof k_stretch[0]))
#define STRETCH_FILE "screen_size.txt"
/* ROTATION. L and R turn the picture to stand along the screen, for a console
 * held upright. It fills the buffers from edge to edge, and the display turns
 * them: 406x720 becomes 720x1280, the game's own shape, with no bars. */
static volatile int g_level, g_rotation; /* 0 none, 1 left, 2 right */
static int g_level_used = -1, g_rotation_used;
static float g_stretch = 1.0f;
static int g_bar = -1;

const char *dcr_game_root(void);

static int bars_possible(void) {
  int w, h, bw, bh;
  dcr_window_size(&w, &h);
  dcr_window_buffers(&bw, &bh);
  return bw > w;
}

static int stretched(int x) { return (int)((float)x * g_stretch + 0.5f); }

static void update_size(void) {
  int w, h, bw, bh;
  dcr_window_size(&w, &h);
  dcr_window_buffers(&bw, &bh);
  g_level_used = g_level;
  g_rotation_used = g_rotation;
  if (bw > w && g_rotation) {
    g_stretch = (float)bw / (float)w;
    g_bar = 0;
  } else {
    g_stretch = bw > w ? k_stretch[g_level] : 1.0f;
    g_bar = bw > w ? (bw - stretched(w)) / 2 : 0;
  }
  nwindowSetTransform(nwindowGetDefault(),
                      g_rotation == 1 ? HAL_TRANSFORM_ROT_270 : g_rotation == 2 ? HAL_TRANSFORM_ROT_90 : 0);
  debugPrintf("[screen] window %dx%d in buffers %dx%d: size %d, rotation %d, %d wide, bars of %d\n", w, h, bw, bh,
              g_level + 1, g_rotation, stretched(w), g_bar);
}

static int bar_width(void) {
  if (g_bar < 0)
    update_size();
  return g_bar;
}

/* Where x of the window lands on the framebuffer that is drawn to or read. */
static int screen_x(int x, unsigned fb) { return fb ? x : bar_width() + stretched(x); }
static int screen_w(int w, unsigned fb) { return fb ? w : stretched(w); }

float su_screen_stretch(void) { return bars_possible() ? k_stretch[g_level] : 1.0f; }

int su_screen_rotation(void) { return bars_possible() ? g_rotation : 0; }

void su_screen_rotate(int direction) {
  if (bars_possible())
    g_rotation = g_rotation == direction ? 0 : direction;
}

void su_screen_next(void) {
  if (!bars_possible() || g_rotation)
    return;
  const int level = (g_level + 1) % STRETCHES;
  g_level = level;
  char path[320];
  snprintf(path, sizeof path, "%s/" STRETCH_FILE, dcr_game_root());
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f, "%d\n", level);
    fclose(f);
  }
}

static void load_size(void) {
  char path[320];
  snprintf(path, sizeof path, "%s/" STRETCH_FILE, dcr_game_root());
  FILE *f = fopen(path, "r");
  int level = 0;
  if (f) {
    if (fscanf(f, "%d", &level) == 1 && level >= 0 && level < STRETCHES)
      g_level = level;
    fclose(f);
  }
}

/* What the game last asked for. It keeps a copy of its state and sends nothing
 * when the rectangle has not changed (hardware 2026-11-05: an offscreen pass and
 * the screen share 406x720, so the screen was drawn unshifted, under the left
 * bar), so moving between the screen and a framebuffer sends them again. */
static int g_vp[4], g_sc[4], g_vp_set, g_sc_set;

static void send_viewport(void) {
  r_Viewport(screen_x(g_vp[0], g_draw_fb), g_vp[1], screen_w(g_vp[2], g_draw_fb), g_vp[3]);
}
static void send_scissor(void) {
  r_Scissor(screen_x(g_sc[0], g_draw_fb), g_sc[1], screen_w(g_sc[2], g_draw_fb), g_sc[3]);
}

static void w_Viewport(int x, int y, int w, int h) {
  g_vp[0] = x, g_vp[1] = y, g_vp[2] = w, g_vp[3] = h;
  g_vp_set = 1;
  send_viewport();
  static int logged;
  if (logged < 8 && !g_draw_fb) {
    logged++;
    debugPrintf("[screen] game viewport on the screen: %d,%d %dx%d\n", x, y, w, h);
  }
}
static void w_Scissor(int x, int y, int w, int h) {
  g_sc[0] = x, g_sc[1] = y, g_sc[2] = w, g_sc[3] = h;
  g_sc_set = 1;
  send_scissor();
}
static void w_BindFramebuffer(unsigned target, unsigned fb) {
  const int was_screen = g_draw_fb == 0;
  if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
    g_draw_fb = fb;
  if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER)
    g_read_fb = fb;
  r_BindFramebuffer(target, fb);
  if (was_screen != (g_draw_fb == 0)) {
    if (g_vp_set)
      send_viewport();
    if (g_sc_set)
      send_scissor();
  }
}
static void w_BlitFramebuffer(int sx0, int sy0, int sx1, int sy1, int dx0, int dy0, int dx1, int dy1,
                              unsigned mask, unsigned filter) {
  r_BlitFramebuffer(screen_x(sx0, g_read_fb), sy0, screen_x(sx1, g_read_fb), sy1, screen_x(dx0, g_draw_fb), dy0,
                    screen_x(dx1, g_draw_fb), dy1, mask, filter);
}

uintptr_t port_gl_wrap(const char *name, uintptr_t real) {
  if (!real || !bars_possible())
    return 0;
#define W(fn)                  \
  if (!strcmp(name, "gl" #fn)) { \
    r_##fn = (void *)real;     \
    return (uintptr_t)w_##fn;  \
  }
  W(Viewport) W(Scissor) W(BindFramebuffer) W(BlitFramebuffer)
#undef W
  return 0;
}

/* The bars go black last: a game's clear covers the whole buffer. */
static void draw_bars(int w) {
  const int bar = bar_width();
  if (!bar || !r_Scissor || !gl.Clear)
    return;
  int bw, bh, box[4];
  float clear[4];
  dcr_window_buffers(&bw, &bh);
  gl.GetIntegerv(GL_SCISSOR_BOX, box);
  gl.GetFloatv(GL_COLOR_CLEAR_VALUE, clear);
  gl.ColorMask(1, 1, 1, 1);
  gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl.Enable(GL_SCISSOR_TEST);
  r_Scissor(0, 0, bar, bh);
  gl.Clear(GL_COLOR_BUFFER_BIT);
  r_Scissor(bar + stretched(w), 0, bw - bar - stretched(w), bh);
  gl.Clear(GL_COLOR_BUFFER_BIT);
  gl.Disable(GL_SCISSOR_TEST);
  r_Scissor(box[0], box[1], box[2], box[3]);
  gl.ClearColor(clear[0], clear[1], clear[2], clear[3]);
  static int checked;
  if (!checked) {
    int samples = 0;
    checked = 1;
    gl.GetIntegerv(GL_SAMPLES, &samples);
    debugPrintf("[screen] first frame: %d samples, GL error 0x%x\n", samples, gl.GetError());
  }
}

/* an arrow, 22 x 32 pixels at 720 high: white outline, blue face */
static void cursor_draw(void) {
  if (!g_enabled)
    return;
  if (!gl.ok) {
    gl.ok = gl_setup() == 0 ? 1 : -1;
  }
  if (gl.ok != 1)
    return;
  if (g_level_used != g_level || g_rotation_used != g_rotation)
    update_size();

  /* the game's state, to be put back */
  int prog = 0, vao = 0, buf = 0, fbo = 0, vp[4], bsrc_rgb = 1, bdst_rgb = 0, bsrc_a = 1, bdst_a = 0;
  unsigned char mask[4];
  gl.GetIntegerv(GL_CURRENT_PROGRAM, &prog);
  gl.GetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
  gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &buf);
  gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
  gl.GetIntegerv(GL_BLEND_SRC_RGB, &bsrc_rgb);
  gl.GetIntegerv(GL_BLEND_DST_RGB, &bdst_rgb);
  gl.GetIntegerv(GL_BLEND_SRC_ALPHA, &bsrc_a);
  gl.GetIntegerv(GL_BLEND_DST_ALPHA, &bdst_a);
  gl.GetIntegerv(GL_VIEWPORT, vp);
  gl.GetBooleanv(GL_COLOR_WRITEMASK, mask);
  const unsigned char blend = gl.IsEnabled(GL_BLEND), depth = gl.IsEnabled(GL_DEPTH_TEST),
                      cull = gl.IsEnabled(GL_CULL_FACE), scissor = gl.IsEnabled(GL_SCISSOR_TEST),
                      stencil = gl.IsEnabled(GL_STENCIL_TEST);

  const float w = (float)(g_w ? g_w : 1280), h = (float)(g_h ? g_h : 720);
  const float s0 = h / 720.0f * (g_down ? 0.85f : 1.0f);
  /* turned, one pixel of the window covers 1.8 of the screen both ways */
  const float s = g_rotation_used ? s0 * 0.5625f : s0;
  const float x = g_x, y = g_y;
  const float sx = g_rotation_used ? s : s / g_stretch; /* the arrow keeps its shape as the picture widens */
  const float out[6] = {x, y, x, y + 32 * s, x + 22 * sx, y + 23 * s};
  const float in[6] = {x + 2.5f * sx, y + 6.5f * s, x + 2.5f * sx, y + 25 * s, x + 17 * sx, y + 21 * s};

  const int had_vp = g_vp_set;
  const int game_vp[4] = {g_vp[0], g_vp[1], g_vp[2], g_vp[3]};
  gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  gl.Viewport(0, 0, (int)w, (int)h);
  gl.Disable(GL_DEPTH_TEST);
  gl.Disable(GL_CULL_FACE);
  gl.Disable(GL_SCISSOR_TEST);
  gl.Disable(GL_STENCIL_TEST);
  gl.Enable(GL_BLEND);
  gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl.UseProgram(g_prog);
  gl.Uniform2f(g_u_res, w, h);
  gl.BindVertexArray(g_vao);
  gl.BindBuffer(GL_ARRAY_BUFFER, g_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof out, out, GL_DYNAMIC_DRAW);
  gl.Uniform4f(g_u_col, 1.0f, 1.0f, 1.0f, 1.0f);
  gl.DrawArrays(GL_TRIANGLES, 0, 3);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof in, in, GL_DYNAMIC_DRAW);
  gl.Uniform4f(g_u_col, 0.08f, 0.43f, 1.0f, 1.0f);
  gl.DrawArrays(GL_TRIANGLES, 0, 3);
  draw_bars((int)w);

  /* put back */
  gl.ColorMask(mask[0], mask[1], mask[2], mask[3]);
  gl.BindFramebuffer(GL_FRAMEBUFFER, (unsigned)fbo);
  if (r_Viewport && had_vp)
    w_Viewport(game_vp[0], game_vp[1], game_vp[2], game_vp[3]);
  else
    (r_Viewport ? r_Viewport : gl.Viewport)(vp[0], vp[1], vp[2], vp[3]);
  gl.UseProgram((unsigned)prog);
  if (r_Scissor && g_sc_set)
    send_scissor();
  gl.BindVertexArray((unsigned)vao);
  gl.BindBuffer(GL_ARRAY_BUFFER, (unsigned)buf);
  gl.BlendFuncSeparate((unsigned)bsrc_rgb, (unsigned)bdst_rgb, (unsigned)bsrc_a, (unsigned)bdst_a);
  blend ? gl.Enable(GL_BLEND) : gl.Disable(GL_BLEND);
  depth ? gl.Enable(GL_DEPTH_TEST) : gl.Disable(GL_DEPTH_TEST);
  cull ? gl.Enable(GL_CULL_FACE) : gl.Disable(GL_CULL_FACE);
  scissor ? gl.Enable(GL_SCISSOR_TEST) : gl.Disable(GL_SCISSOR_TEST);
  stencil ? gl.Enable(GL_STENCIL_TEST) : gl.Disable(GL_STENCIL_TEST);
}

void su_cursor_install(void) {
  load_size();
  dcr_present_hook = cursor_draw;
}
