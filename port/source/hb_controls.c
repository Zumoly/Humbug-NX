/* hb_controls.c -- see hb_controls.h.
 *
 * Humbug moves a creature when a finger goes down on it and drags. So a flick
 * is that drag: down at the cursor, a quick move in the stick's direction, up.
 * The cursor stays where it was, ready for the next flick.
 *
 * One press is one flick. Holding repeats nothing, and a press made while a
 * flick is in the air waits for it to end; the newest press wins. MIT.
 */
#include "hb_controls.h"

#include <math.h>

/* Speeds in screen heights per second, so they feel the same at 720 and 1080. */
#define CURSOR_SPEED 1.38f
#define SLOW_SPEED 0.32f /* the right stick */
#define DEADZONE 0.12f

#define FLICK_REACH 0.16f  /* of the view's height */
#define FLICK_TIME 0.14f   /* seconds */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float curve(float v) {
  float a = fabsf(v);
  if (a < DEADZONE)
    return 0;
  a = (a - DEADZONE) / (1 - DEADZONE);
  return copysignf(a * a, v); /* squared for fine control near the centre */
}

void hb_controls_init(HbControls *c, int w, int h) {
  HbControls z = {0};
  *c = z;
  c->w = w;
  c->h = h;
  c->x = (float)w * 0.5f;
  c->y = (float)h * 0.5f;
}

void hb_controls_set(HbControls *c, float x, float y) {
  c->x = clampf(x, 0, (float)(c->w - 1));
  c->y = clampf(y, 0, (float)(c->h - 1));
}

static void push(HbEvent *out, int *n, HbAction a, float x, float y) {
  if (*n < HB_MAX_EVENTS) {
    out[*n].action = a;
    out[*n].x = x;
    out[*n].y = y;
    (*n)++;
  }
}

static void flick_point(const HbControls *c, float t, float *x, float *y) {
  const float dx = c->flick == HB_FLICK_LEFT ? -1.0f : c->flick == HB_FLICK_RIGHT ? 1.0f : 0.0f;
  const float dy = c->flick == HB_FLICK_UP ? -1.0f : c->flick == HB_FLICK_DOWN ? 1.0f : 0.0f;
  const float reach = (float)c->h * FLICK_REACH * t;
  *x = clampf(c->flick_x + dx * reach, 0, (float)(c->w - 1));
  *y = clampf(c->flick_y + dy * reach, 0, (float)(c->h - 1));
}

static void end_flick(HbControls *c, HbEvent *out, int *n) {
  push(out, n, HB_UP, c->last_x, c->last_y);
  c->flick = HB_FLICK_NONE;
}

int hb_controls_update(HbControls *c, const HbFrame *f, HbEvent *out) {
  int n = 0;
  const float h = (float)c->h, old_x = c->x, old_y = c->y;

  /* A flick pins the pointer: it starts and ends on the same spot. */
  if (c->flick == HB_FLICK_NONE) {
    const float x = c->x + (curve(f->lx) * CURSOR_SPEED + curve(f->rx) * SLOW_SPEED) * h * f->dt;
    const float y = c->y - (curve(f->ly) * CURSOR_SPEED + curve(f->ry) * SLOW_SPEED) * h * f->dt;
    hb_controls_set(c, x, y);
  }
  const int moved = c->x != old_x || c->y != old_y;

  /* A tap always beats a flick in the air: both need the one finger. */
  if (f->tap_held && !c->finger_down) {
    c->queued = HB_FLICK_NONE;
    if (c->flick != HB_FLICK_NONE)
      end_flick(c, out, &n);
  }

  if (f->tap_held && !c->finger_down)
    push(out, &n, HB_DOWN, c->x, c->y);
  else if (!f->tap_held && c->finger_down)
    push(out, &n, HB_UP, c->x, c->y);
  else if (f->tap_held && moved)
    push(out, &n, HB_MOVE, c->x, c->y);
  c->finger_down = f->tap_held;

  int ended = 0; /* the game must see a frame with no finger between two flicks */
  if (c->flick != HB_FLICK_NONE) {
    c->flick_t += f->dt / FLICK_TIME;
    flick_point(c, c->flick_t > 1 ? 1 : c->flick_t, &c->last_x, &c->last_y);
    push(out, &n, HB_MOVE, c->last_x, c->last_y);
    if (c->flick_t >= 1) {
      end_flick(c, out, &n);
      ended = 1;
    }
  }

  if (f->flick_press)
    c->queued = (HbFlick)f->flick_press;

  /* A held tap is a drag already: the flick waits for it to end. */
  if (c->flick == HB_FLICK_NONE && c->queued != HB_FLICK_NONE && !c->finger_down && !ended) {
    c->flick = c->last_flick = c->queued;
    c->queued = HB_FLICK_NONE;
    c->flick_t = 0;
    c->flick_x = c->last_x = c->x;
    c->flick_y = c->last_y = c->y;
    push(out, &n, HB_DOWN, c->x, c->y);
  }
  return n;
}
