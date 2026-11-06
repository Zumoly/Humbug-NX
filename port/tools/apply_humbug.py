#!/usr/bin/env python3
"""One-time edit of the template's files into the Humbug port. Each replacement
must match, so a wrong assumption stops here instead of building quietly."""
import re

S = "source/"


def edit(path, pairs):
    t = open(path, encoding="utf8", newline="").read()
    for a, b in pairs:
        if a not in t:
            raise SystemExit("%s: not found: %r" % (path, a[:60]))
        t = t.replace(a, b)
    open(path, "w", encoding="utf8", newline="\n").write(t)


def cut(path, start, end, new):
    t = open(path, encoding="utf8", newline="").read()
    i = t.index(start)
    j = t.index(end, i)
    t = t[:i] + new + t[j:]
    open(path, "w", encoding="utf8", newline="\n").write(t)


edit(S + "port_config.h", [
    ("Teeny Titans' settings for the", "Humbug's settings for the"),
    ('#define PORT_TITLE    "Teeny Titans"', '#define PORT_TITLE    "Humbug"'),
    ('#define PORT_NAME     "teenytitans_nx"', '#define PORT_NAME     "humbug_nx"'),
    ('#define PORT_PACKAGE  "com.turner.ttgfigures"', '#define PORT_PACKAGE  "com.dunderbit.humbug"'),
    ('"teenytitans_nx: Teeny Titans (Unity 2017.4.36f1 / IL2CPP, armeabi-v7a)"',
     '"humbug_nx: Humbug (Unity 2019.2.15f1 / IL2CPP, armeabi-v7a)"'),
    (" * game folder (dcr_path.c maps Android/obb/<package>/ there). */",
     " * game folder (dcr_path.c maps Android/obb/<package>/ there). Humbug has none:\n"
     " * its data is inside the APK. */"),
    ('#define PORT_APK_DESC "Teeny Titans (armeabi-v7a)"', '#define PORT_APK_DESC "Humbug (armeabi-v7a)"'),
    ('{.what = "Teeny Titans", .name = "game.apk",', '{.what = "Humbug", .name = "game.apk",'),
    ('.package = "com.turner.ttgfigures", .flags', '.package = "com.dunderbit.humbug", .flags'),
    ('"by Iqbalriz (the Switch port); the game by Grumpyface Studios / Cartoon Network"',
     '"the game by Dunderbit; Switch wrapper on android32"'),
    ('#define RT_PROC_COMM        "turner.ttgfigu"', '#define RT_PROC_COMM        "dunderbit.humbu"'),
    ("#define RT_ZLIB_HOLDBACK    0\n",
     "#define RT_ZLIB_HOLDBACK    0\n\n"
     "/* ------------------------------------------------------------------ window */\n"
     "/* The game is portrait: bars at the sides, the console held normally. */\n"
     "#define RT_VI_SCALING_MODE ViScalingMode_PreserveAspectRatio\n"),
])

edit(S + "config.h", [
    ("Teeny Titans's own build-wide constants", "Humbug's own build-wide constants"),
    (" * Unity 2017.4.36f1, IL2CPP backend, armeabi-v7a; the data is in the expansion\n"
     " * file (main.<version>.com.turner.ttgfigures.obb). MIT.",
     " * Unity 2019.2.15f1, IL2CPP backend, armeabi-v7a; the data is in the APK. MIT."),
    ("#define DCR_TIME_ICALL_HOOKS    1   /* hook the 11 Time icalls                */\n", ""),
])

edit(S + "dcr_setup_plan.c", [
    ("Teeny Titans's part of the first", "Humbug's part of the first"),
    ('build of Steven Universe:\\n"\n                       "(com.turner.ttgfigures): use an APK with lib/armeabi-v7a."',
     'build of Humbug\\n"\n                       "(com.dunderbit.humbug): use an APK with lib/armeabi-v7a."'),
    (" * classes.txt, then \"Starting the game\". The game's data is in the\n"
     " * expansion file (obb/), which Unity reads as it is.\n",
     " * classes.txt, then \"Starting the game\".\n"),
])

edit(S + "dcr_watch.c", [("Teeny Titans' part", "Humbug's part")])

edit(S + "dcr_main.c", [
    ("Teeny Titans's part of the boot", "Humbug's part of the boot"),
    ("void dcr_il2cpp_video_install(void);\n", ""),
    ("void dcr_fmod_patch_voices(void);\nvoid dcr_fmod_patch_rate(void);\n", ""),
    ('#include "error.h"\n', '#include "error.h"\n#include "hb_game.h"\n'),
    ('"Copy the APK of your own Teeny Titans (armeabi-v7a) into that folder,\\n"\n'
     '         "under any name, and its expansion file\\n"\n'
     '         "(main.<version>.com.turner.ttgfigures.obb) into obb/ there: the libraries\\n"\n'
     '         "are unpacked from the APK on the first launch, the data is read from the OBB."',
     '"Copy the APK of your own Humbug (armeabi-v7a) into that folder,\\n"\n'
     '         "under any name: the libraries are unpacked from it on the first launch."'),
    ("  if (dcr_config()->sound_priority)\n"
     "    dcr_fmod_patch_voices(); /* FMOD: 64 real voices, not 32 (dcr_fmod.c) */\n"
     "  if (dcr_config()->mix_48k)\n"
     "    dcr_fmod_patch_rate(); /* FMOD mixes at 48 kHz, as on a phone (dcr_fmod.c) */\n\n"
     "  dcr_time_install(); /* 11 Time icalls + 4 TimeManager entries */\n", "\n"),
    ("  dcr_il2cpp_video_install(); /* the intro, through su_video.c */\n",
     "  hb_game_install();    /* Y, L and Plus press the game's own buttons */\n"),
    ("int dcr_time_install(void);\n", ""),
])

edit(S + "dcr_boot.c", [
    ("void dcr_il2cpp_report(void);\n", "void dcr_il2cpp_report(void);\n"),
    (" * Set at the first swap, since the boot log shares the window until then. */\n", " */\n"),
    ("/* The window is portrait, the panel landscape: the display turns each frame.\n",
     "/* What is current on the engine's thread, for a movie.\n"),
    ("  static int done;\n  /* Unity draws on its own thread", "  /* Unity draws on its own thread"),
    ("  g_egl_ctx = b_eglGetCurrentContext();\n  if (done)\n    return;\n  done = 1;\n"
     "  nwindowSetTransform(nwindowGetDefault(), dcr_config()->tate_left ? HAL_TRANSFORM_ROT_270 : HAL_TRANSFORM_ROT_90);\n",
     "  g_egl_ctx = b_eglGetCurrentContext();\n"),
    ('"copy of Teeny Titans as game.apk."', '"copy of Humbug as game.apk."'),
    ("  fn_v resume, done, lowMemory;\n", "  fn_v resume, done, lowMemory, surfaceChanged;\n"),
    ('  U.lowMemory = (fn_v)native("nativeLowMemory", 0);\n',
     '  U.lowMemory = (fn_v)native("nativeLowMemory", 0);\n'
     '  U.surfaceChanged = (fn_v)native("nativeSendSurfaceChangedEvent", 0);\n'),
    ("  U.recreateGfxState(g_jni_env, g_thiz, 0, g_surface);\n",
     "  U.recreateGfxState(g_jni_env, g_thiz, 0, g_surface);\n"
     "  if (U.surfaceChanged)\n    U.surfaceChanged(g_jni_env, g_thiz);\n"),
    ("        if (dcr_config()->log_sounds) {\n"
     "          void dcr_fmod_census(char *out, size_t cap);\n"
     "          static char census[300];\n"
     "          dcr_fmod_census(census, sizeof census);\n"
     "          if (census[0])\n"
     '            debugPrintf("[fmod] %s\\n", census);\n'
     "        }\n", ""),
])

edit(S + "dcr_vsync.c", [
    ("/* dcr_vsync.c -- the frame pulse Unity 2017.4's Android player waits for.",
     "/* dcr_vsync.c -- the frame pulse Unity 2019.2's Android player waits for."),
    ("  if (s_wait)\n    s_hooked = dcr_hook(&unity_mod, DCR_RVA_WaitVSync, DCR_GUARD_WaitVSync, (void *)w_wait_vsync,\n"
     '                        "WaitVSync");\n',
     "  /* WaitVSync is Thumb code here and is left alone: it keeps its own wait. */\n"),
])
t = open(S + "dcr_vsync.c", encoding="utf8").read()
t = t.replace("static void w_wait_vsync(int target) {", "__attribute__((unused)) static void w_wait_vsync(int target) {")
open(S + "dcr_vsync.c", "w", encoding="utf8", newline="\n").write(t)

# ---- the collector
GC = '''#define RVA_SUSPEND_HANDLER HB_RVA_GC_HANDLER
#define k_guard_handler HB_GUARD_GC_HANDLER

/* GC_Thread, read off GC_suspend_handler */
typedef struct GcThread {
  struct GcThread *next;
  uintptr_t id; /* pthread_t */
  volatile uint32_t last_stop_count; /* bit 0: restarted, when signals are retried */
  void *volatile stack_ptr;
} GcThread;

typedef int (*fn_sem_post)(void *sem);

static fn_sem_post g_sem_post;
static volatile uint32_t *g_stop_count;
static volatile int *g_retry_signals;
static volatile int *g_sig_suspend, *g_sig_restart;
static GcThread **g_threads;
static void *g_ack_sem;
static int g_gc_ready;
'''
cut(S + "il2cpp_rt.c", "#define RVA_SUSPEND_HANDLER 0x1049d94u", "static volatile uint32_t g_n_suspend", GC + "\n")
edit(S + "il2cpp_rt.c", [
    (" * The addresses are for Teeny Titans 1.2.7's libil2cpp (Unity 2017.4.36f1). The\n"
     " * handler's first words are checked before any of them is used. MIT.\n",
     " * The addresses are for Humbug 2.2.4's libil2cpp (Unity 2019.2.15f1): see\n"
     " * dcr_offsets.h. The handler's first words are checked before any of them is\n"
     " * used. Unlike Unity 2017.4's collector this one hashes thread ids, keeps\n"
     " * no tid in GC_Thread, and with GC_retry_signals also waits for restarts:\n"
     " * bit 0 of last_stop_count, and one more post of the semaphore. MIT.\n"),
    (' *   restart:  resume the thread. This collector waits for no acknowledgement.\n',
     ' *   restart:  resume the thread, and acknowledge if signals are retried.\n'),
    (' *   suspend:  find the thread\'s GC_Thread in GC_threads[id & 0xff]; if its\n',
     ' *   suspend:  find the thread\'s GC_Thread in the GC_threads table; if its\n'),
    ('#include "bionic_pthread.h"\n', '#include "bionic_pthread.h"\n#include "dcr_offsets.h"\n'),
    ("    g_stop_count = (volatile uint32_t *)(b + RVA_STOP_COUNT);\n"
     "    g_sig_suspend = (volatile int *)(b + RVA_SIG_SUSPEND);\n"
     "    g_sig_restart = (volatile int *)(b + RVA_SIG_RESTART);\n"
     "    g_threads = (GcThread **)(b + RVA_GC_THREADS);\n"
     "    g_ack_sem = (void *)(b + RVA_ACK_SEM);\n",
     "    g_stop_count = (volatile uint32_t *)(b + HB_RVA_GC_STOP_COUNT);\n"
     "    g_retry_signals = (volatile int *)(b + HB_RVA_GC_RETRY_SIGNALS);\n"
     "    g_sig_suspend = (volatile int *)(b + HB_RVA_GC_SIG_SUSPEND);\n"
     "    g_sig_restart = (volatile int *)(b + HB_RVA_GC_SIG_RESTART);\n"
     "    g_threads = (GcThread **)(b + HB_RVA_GC_THREADS);\n"
     "    g_ack_sem = (void *)(b + HB_RVA_GC_ACK_SEM);\n"),
    ("  GcThread *me = g_threads[(uintptr_t)t & 0xff];\n",
     "  const uintptr_t id = (uintptr_t)t;\n"
     "  GcThread *me = g_threads[(id ^ (id >> 8) ^ (id >> 16)) & 0xff];\n"),
    ("  if (me->last_stop_count == stop) {\n", "  if ((me->last_stop_count & ~1u) == stop) {\n"),
    ("    svcSetThreadActivity(t->handle, ThreadActivity_Runnable);\n  }\n  b_pause_unlock();\n  g_n_restart++;\n",
     "    svcSetThreadActivity(t->handle, ThreadActivity_Runnable);\n  }\n  b_pause_unlock();\n  g_n_restart++;\n"
     "  if (*g_retry_signals) {\n"
     "    GcThread *me = gc_lookup(t);\n"
     "    if (me)\n"
     "      me->last_stop_count = *g_stop_count | 1;\n"
     "    __atomic_thread_fence(__ATOMIC_SEQ_CST);\n"
     "    g_sem_post(g_ack_sem);\n"
     "  }\n"),
])
cut(S + "il2cpp_rt.c", "/* ---------------------------------------------------------- VideoPlayer",
    "/* dcr_prof.c's names for managed frames",
    "/* Humbug plays no intro movie through su_video.c. */\nvoid dcr_il2cpp_video_done(void) {}\n\n")

edit(S + "dcr_config.c", [
    ("Teeny Titans' settings", "Humbug's settings"),
    (".res_w = 720, .res_h = 1280, .boost = 1, .mix_48k = 1,", ".res_w = 406, .res_h = 720, .boost = 1, .mix_48k = 1,"),
    ('''    {"display", "tate_rotation", "right",
     "The game is portrait: hold the console upright. right: its right side up.\\n"
     "# left: its left side up.",
     CFG_CHOICE, "right,left", &g_cfg.tate_left},
''', ""),
    ('''    {"audio", "mix_at_48khz", "true",
     "Mix the game's audio at 48 kHz, as phones do. Off: 24 kHz, the Android\\n"
     "# fallback's rate.",
     CFG_BOOL, NULL, &g_cfg.mix_48k},
''', ""),
    ('''    {"debug", "log_sounds", "false",
     "Write every sound the game plays to debug.log (for a bug report about audio).",
     CFG_BOOL, NULL, &g_cfg.log_sounds},
''', ""),
    ("  /* a portrait window; the picture is turned on the way out (dcr_boot.c) */\n"
     "  g_cfg.res_w = rt->res_h;\n  g_cfg.res_h = rt->res_w;\n",
     "  /* a portrait window, 9:16, with bars at the sides of the screen */\n"
     "  g_cfg.res_h = rt->res_h;\n  g_cfg.res_w = (rt->res_h * 9 / 16 + 1) & ~1;\n"),
])

edit(S + "jni_android.c", [
    ("H(act_getRequestedOrientation) { return jv_i(6); /* SCREEN_ORIENTATION_SENSOR_LANDSCAPE */ }",
     "H(act_getRequestedOrientation) { return jv_i(1); /* SCREEN_ORIENTATION_PORTRAIT */ }"),
    ("F(ai_screenOrientation) { return jv_i(6); }", "F(ai_screenOrientation) { return jv_i(1); }"),
])

edit(S + "su_cursor.c", [
    ("/* su_cursor.c -- the pointer cursor: an arrow drawn over the game's frame",
     "/* su_cursor.c -- the blue pointer cursor: an arrow drawn over the game's frame"),
    (" * The game is made for a finger. The right stick moves this cursor and A taps with it (dcr_input.c turns that\n"
     " * into a touch). It shows when the stick moves and fades a few seconds after\n"
     " * the last move or tap, so it is never in the way.\n",
     " * The game is made for a finger. The left stick moves this cursor and A taps\n"
     " * with it (dcr_input.c turns that into a touch). It is always on screen: flicks\n"
     " * start where it is.\n"),
    ("  if (!g_enabled || !g_seen)\n    return;\n  const u64 now = armGetSystemTick();\n"
     "  const u64 age = armTicksToNs(now - g_seen);\n  if (age >= HIDE_NS)\n    return;\n",
     "  if (!g_enabled)\n    return;\n"),
    ("  const float fade = age > 3000000000ull ? 1.0f - (float)(age - 3000000000ull) / 1e9f : 1.0f;\n", ""),
    ("  gl.Uniform4f(g_u_col, 0.0f, 0.0f, 0.0f, 0.9f * fade);\n", "  gl.Uniform4f(g_u_col, 1.0f, 1.0f, 1.0f, 1.0f);\n"),
    ("  gl.Uniform4f(g_u_col, 1.0f, 1.0f, 1.0f, 0.95f * fade);\n", "  gl.Uniform4f(g_u_col, 0.08f, 0.43f, 1.0f, 1.0f);\n"),
    ("/* an arrow, 22 x 32 pixels at 1280x720: black outline, white face */",
     "/* an arrow, 22 x 32 pixels at 720 high: white outline, blue face */"),
    ("#define HIDE_NS 4000000000ull\n\n", ""),
])

# ---- input
I = S + "dcr_input.c"
edit(I, [
    (" * CONTROLLERS: the game is touch-only on Android, so the\n"
     " * controller plays it as a finger -- a cursor on the right stick that A taps\n"
     " * with, swipes on the left stick and D-pad, Back on B and -. pad_frame below\n"
     " * has the details. (dcr_sea_nx presented an NVIDIA Shield pad to Crossy\n"
     " * Road's InControl instead.)\n",
     " * CONTROLLERS: the game is touch-only on Android, so the controller plays it\n"
     " * as a finger: hb_controls.c. pad_frame below has the buttons.\n"),
    ('#include "gl_layer.h"\n', '#include "gl_layer.h"\n#include "hb_controls.h"\n#include "hb_game.h"\n'),
    ("static float g_view_sx = 1.0f, g_view_sy = 1.0f; /* panel -> view */\n",
     "static float g_view_scale = 1.0f, g_view_bar = 0.0f; /* panel -> view: scale, and the side bar */\n"
     "static HbControls g_ctl;\n"),
    ("static int g_cursor_pid = -1, g_swipe_pid = -1; /* the pad's fingers (pad_frame); -1 none */\n",
     "static int g_cursor_pid = -1; /* the pad's finger (pad_frame); -1 none */\n"),
    ("  /* the panel is turned: its long side is the view's height */\n"
     "  g_view_sx = (float)w / 720.0f;\n  g_view_sy = (float)h / 1280.0f;\n",
     "  /* the portrait view fills the panel's height, with a bar at each side */\n"
     "  g_view_scale = (float)h / 720.0f;\n"
     "  g_view_bar = (1280.0f - (float)w / g_view_scale) * 0.5f;\n"
     "  hb_controls_init(&g_ctl, w, h);\n"),
    ("    /* panel space (1280x720) -> view space (the rendering size), turned as\n"
     "     * the display turns the picture */\n"
     "    const float px = (float)ts.touches[k].x, py = (float)ts.touches[k].y;\n"
     "    const int left = dcr_config()->tate_left;\n"
     "    float x = (left ? 720.0f - py : py) * g_view_sx, y = (left ? px : 1280.0f - px) * g_view_sy;\n",
     "    /* panel space (1280x720) -> view space (the rendering size) */\n"
     "    float x = ((float)ts.touches[k].x - g_view_bar) * g_view_scale;\n"
     "    const float y = (float)ts.touches[k].y * g_view_scale;\n"
     "    x = x < 0 ? 0 : x > (float)(g_ctl.w - 1) ? (float)(g_ctl.w - 1) : x;\n"
     "    if (k == 0)\n      hb_controls_set(&g_ctl, x, y); /* the cursor follows the finger */\n"),
    ("    if (!seen[i] && g_cur.id[i] != g_cursor_pid && g_cur.id[i] != g_swipe_pid &&\n"
     "        g_cur.id[i] != 99) /* the pad's fingers (pad_frame), the test script's */\n",
     "    if (!seen[i] && g_cur.id[i] != g_cursor_pid &&\n"
     "        g_cur.id[i] != 99) /* the pad's finger (pad_frame), the test script's */\n"),
])
PAD = r'''/* The controller, for a game made for a finger (the moves: hb_controls.c):
 *
 *   left stick, D-pad  move the blue cursor (the D-pad slowly)
 *   right stick        flick up, down, left or right, from the cursor
 *   A, ZR, ZL          tap at the cursor; held, it drags
 *   B                  Android's Back key: the game's back, closes menus
 *   Y                  restart the level
 *   L                  hint
 *   +                  pause
 */
/* The pad's finger takes the lowest pointer id free, as Android gives a new
 * finger: Unity makes its mouse (GetMouseButtonDown(0)) from pointer id 0 only. */
static int free_pid(void) {
  for (int id = 0; id < DCR_MAX_POINTERS; id++)
    if (find_ptr(id) < 0)
      return id;
  return -1;
}
#define KEY_BACK 4
#define SRC_KEYBOARD 0x101

static void send_back(int action) {
  DcrKey *k = calloc(1, sizeof *k);
  if (!k)
    return;
  k->action = action;
  k->key_code = KEY_BACK;
  k->source = SRC_KEYBOARD;
  k->device_id = -1;
  k->down_time_ms = k->event_time_ms = now_ms();
  inject("android/view/KeyEvent", k);
}

void su_cursor_set(float x, float y, int w, int h, int down, int touched);

static void pad_finger(const HbEvent *e) {
  if (e->action == HB_DOWN) {
    g_cursor_pid = free_pid();
    if (g_cursor_pid >= 0)
      ptr_down(g_cursor_pid, e->x, e->y);
    return;
  }
  if (g_cursor_pid < 0)
    return;
  const int i = find_ptr(g_cursor_pid);
  if (i >= 0) {
    g_cur.x[i] = e->x;
    g_cur.y[i] = e->y;
  }
  if (e->action == HB_MOVE) {
    if (i >= 0)
      send_motion(AM_MOVE);
  } else {
    ptr_up(g_cursor_pid);
    g_cursor_pid = -1;
  }
}

static void pad_frame(void) {
  for (int i = 0; i < NSLOTS; i++)
    padUpdate(&g_pads[i]);
  int active = pick_active();
  log_controllers(active);
  float st4[4] = {0};
  u64 held = (active >= 0 ? slot_read(active, st4) : 0) | g_ts_held; /* + the test script's */
  u64 down = held & ~g_held_sent;
  u64 up = g_held_sent & ~held;
  g_held_sent = held;

  static u64 last_tick;
  const u64 tick = armGetSystemTick();
  float dt = last_tick ? (float)armTicksToNs(tick - last_tick) * 1e-9f : 1.0f / 60.0f;
  last_tick = tick;
  if (dt > 0.1f)
    dt = 0.1f;

  HbFrame f;
  f.lx = st4[0], f.ly = st4[1], f.rx = st4[2], f.ry = st4[3];
  f.dpad_left = !!(held & HidNpadButton_Left);
  f.dpad_right = !!(held & HidNpadButton_Right);
  f.dpad_up = !!(held & HidNpadButton_Up);
  f.dpad_down = !!(held & HidNpadButton_Down);
  f.tap_held = !!(held & (HidNpadButton_A | HidNpadButton_ZR | HidNpadButton_ZL));
  f.dt = dt;

  HbEvent ev[HB_MAX_EVENTS];
  const int n = hb_controls_update(&g_ctl, &f, ev);
  for (int i = 0; i < n; i++)
    pad_finger(&ev[i]);
  su_cursor_set(g_ctl.x, g_ctl.y, g_ctl.w, g_ctl.h, g_cursor_pid >= 0, 1);

  if (down & HidNpadButton_B)
    send_back(0);
  if (up & HidNpadButton_B)
    send_back(1);
  if (down & HidNpadButton_Y)
    hb_game_restart();
  if (down & HidNpadButton_L)
    hb_game_hint();
  if (down & HidNpadButton_Plus)
    hb_game_pause();
}

'''
cut(I, "/* The controller, for a game made for a finger. The console is held upright:",
    "/* ------------------------------------------------------------ test script", PAD)

# ---- runtime: the window's scaling, a setting with the old default
edit("runtime/source/nx_init.c", [
    ("rc = viSetLayerScalingMode(&g_vi_layer, ViScalingMode_FitToLayer);",
     "rc = viSetLayerScalingMode(&g_vi_layer, RT_VI_SCALING_MODE);"),
])
t = open("runtime/source/rt_settings.h", encoding="utf8").read()
if "RT_VI_SCALING_MODE" not in t:
    t = t.replace("#ifndef RT_OPENSLES\n", "#ifndef RT_VI_SCALING_MODE\n#define RT_VI_SCALING_MODE ViScalingMode_FitToLayer\n#endif\n#ifndef RT_OPENSLES\n", 1)
    open("runtime/source/rt_settings.h", "w", encoding="utf8", newline="\n").write(t)

edit("Makefile", [
    ("# Teeny Titans -- Nintendo Switch wrapper (32-bit / AArch32, Unity 2017.4 IL2CPP)",
     "# Humbug -- Nintendo Switch wrapper (32-bit / AArch32, Unity 2019.2 IL2CPP)"),
    ("TARGET               := teenytitans_nx", "TARGET               := humbug_nx"),
    ("PORT_NPDM_PROGRAM_ID := 0x0100A7E35B2C5E20", "PORT_NPDM_PROGRAM_ID := 0x0100D4B81C7A5E30"),
])
edit("tools/imports.cfg", [])
print("ok")
