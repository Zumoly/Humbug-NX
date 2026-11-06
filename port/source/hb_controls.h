/* hb_controls.h -- the controller as a finger. No Switch code, so test/selftest.c
 * runs the same logic on the host. MIT. */
#ifndef HB_CONTROLS_H
#define HB_CONTROLS_H

#define HB_MAX_EVENTS 8

/* One frame of controller state. Sticks run -1..1 with up positive. */
typedef struct {
  float lx, ly, rx, ry; /* the left stick moves the cursor, the right one slowly */
  int tap_held; /* A, ZR or ZL */
  int flick_press; /* an HbFlick: the flick button pressed this frame, or 0 */
  float dt;
} HbFrame;

typedef enum { HB_DOWN, HB_MOVE, HB_UP } HbAction;
typedef struct {
  HbAction action;
  float x, y;
} HbEvent;

typedef enum { HB_FLICK_NONE, HB_FLICK_UP, HB_FLICK_DOWN, HB_FLICK_LEFT, HB_FLICK_RIGHT } HbFlick;

typedef struct {
  float x, y;       /* the blue cursor, in pixels of the view */
  int w, h;         /* the view */
  int finger_down;  /* a tap or drag is held */
  HbFlick flick;    /* in the air now */
  HbFlick last_flick;
  float flick_t, flick_x, flick_y, last_x, last_y;
  HbFlick queued;   /* pressed while another flick was in the air */
} HbControls;

void hb_controls_init(HbControls *c, int w, int h);
/* Puts the cursor where a real finger touched. */
void hb_controls_set(HbControls *c, float x, float y);
/* Writes up to HB_MAX_EVENTS events to out and returns how many. */
int hb_controls_update(HbControls *c, const HbFrame *f, HbEvent *out);

#endif
