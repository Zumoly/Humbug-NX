/* rt_window.c -- the one window: libnx's default NWindow, at the rendering
 * size config.ini chose.
 *
 * Every port has exactly one window. Its size starts at RT_SCREEN_W x
 * RT_SCREEN_H and config.ini [display] resolution may change it before the
 * engine starts. The vi layer scales it to the screen: 1080p is shown as is
 * docked and downscaled on the 720p panel. The EGL layer (gl_mesa.c) calls
 * dcr_window_prepare right before it makes the window surface; a game with an
 * ANativeWindow of its own gets the same window through android_ndk.c. While
 * the on-screen boot log owns the window (the null renderer, or setup before
 * EGL), its geometry is left alone. MIT.
 */
#include <switch.h>

#include "rt_settings.h"
#include "rt_window.h"
#include "util.h"

static int g_win_w = RT_SCREEN_W, g_win_h = RT_SCREEN_H;
static int g_buf_w, g_buf_h; /* the real buffers, when a port draws its own bars */

void dcr_window_set_buffers(int w, int h) {
  g_buf_w = w;
  g_buf_h = h;
}

void dcr_window_buffers(int *w, int *h) {
  *w = g_buf_w ? g_buf_w : g_win_w;
  *h = g_buf_h ? g_buf_h : g_win_h;
}

void dcr_window_size(int *w, int *h) {
  if (w)
    *w = g_win_w;
  if (h)
    *h = g_win_h;
}

void dcr_window_set_size(int w, int h) {
  if (w > 0 && h > 0) {
    g_win_w = w;
    g_win_h = h;
  }
}

void rt_window_set_geom(NWindow *w, u32 width, u32 height) {
  if (!nwindowIsValid(w) || log_console_active())
    return; /* the on-screen boot log owns the window */
  if (g_buf_w) {
    width = (u32)g_buf_w;
    height = (u32)g_buf_h;
  }
  nwindowSetDimensions(w, width, height);
  nwindowSetCrop(w, 0, 0, width, height);
  nwindowSetSwapInterval(w, 1);
}

void dcr_window_prepare(void) { rt_window_set_geom(nwindowGetDefault(), (u32)g_win_w, (u32)g_win_h); }
