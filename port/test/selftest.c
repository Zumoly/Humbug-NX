/* Host test of the controller logic. build.ps1 runs it before the Switch build. */
#include <stdio.h>
#include <string.h>

#include "hb_controls.h"

#define W 406
#define H 720

static int fails;
#define CHECK(c)                                     \
  do {                                               \
    if (!(c)) {                                      \
      printf("FAIL line %d: %s\n", __LINE__, #c);    \
      fails++;                                       \
    }                                                \
  } while (0)

static HbEvent g_ev[2048];
static int g_n;

static void run(HbControls *c, const HbFrame *f, int frames) {
  g_n = 0;
  for (int i = 0; i < frames; i++) {
    HbEvent e[HB_MAX_EVENTS];
    const int n = hb_controls_update(c, f, e);
    for (int k = 0; k < n && g_n < 2048; k++)
      g_ev[g_n++] = e[k];
  }
}

static int count(HbAction a) {
  int n = 0;
  for (int i = 0; i < g_n; i++)
    n += g_ev[i].action == a;
  return n;
}

static HbFrame frame(void) {
  HbFrame f;
  memset(&f, 0, sizeof f);
  f.dt = 1.0f / 60;
  return f;
}

/* A press: the event for one frame, then the button is only held. */
static void press(HbControls *c, HbFlick dir, int frames) {
  HbFrame f = frame();
  f.flick_press = dir;
  run(c, &f, 1);
  HbEvent first[8];
  const int n = g_n;
  memcpy(first, g_ev, sizeof first);
  f.flick_press = 0;
  run(c, &f, frames);
  memmove(g_ev + n, g_ev, sizeof(HbEvent) * (size_t)g_n);
  memcpy(g_ev, first, sizeof(HbEvent) * (size_t)n);
  g_n += n;
}

static void flick_case(HbFlick dir, int dx, int dy) {
  HbControls c;
  hb_controls_init(&c, W, H);
  hb_controls_set(&c, 150, 300);
  press(&c, dir, 12);
  CHECK(c.last_flick == dir);
  CHECK(count(HB_DOWN) == 1 && count(HB_UP) == 1);
  CHECK(g_ev[0].x == 150 && g_ev[0].y == 300); /* it starts at the pointer */
  const float mx = g_ev[g_n - 1].x - 150, my = g_ev[g_n - 1].y - 300;
  CHECK(mx * (float)dx >= 0 && my * (float)dy >= 0);
  CHECK((dx ? mx * (float)dx : my * (float)dy) >= 110);
  CHECK((dx ? my : mx) == 0);
  CHECK(c.x == 150 && c.y == 300); /* and the pointer stays there */
}

int main(void) {
  flick_case(HB_FLICK_UP, 0, -1);
  flick_case(HB_FLICK_DOWN, 0, 1);
  flick_case(HB_FLICK_LEFT, -1, 0);
  flick_case(HB_FLICK_RIGHT, 1, 0);

  HbControls c;
  HbFrame f;

  /* a press is one flick, however long the button stays down */
  hb_controls_init(&c, W, H);
  press(&c, HB_FLICK_RIGHT, 120);
  CHECK(count(HB_DOWN) == 1 && count(HB_UP) == 1);

  /* right, then up while right is in the air: up follows, right does not repeat */
  hb_controls_init(&c, W, H);
  press(&c, HB_FLICK_RIGHT, 3);
  press(&c, HB_FLICK_UP, 60);
  CHECK(count(HB_DOWN) == 1 && c.last_flick == HB_FLICK_UP && c.flick == HB_FLICK_NONE);

  /* two flicks are a frame apart, so the game sees the finger lift */
  hb_controls_init(&c, W, H);
  f = frame();
  f.flick_press = HB_FLICK_RIGHT;
  HbEvent e[HB_MAX_EVENTS];
  int up_and_down_together = 0;
  for (int i = 0; i < 40; i++) {
    const int n = hb_controls_update(&c, &f, e);
    f.flick_press = i == 2 ? HB_FLICK_LEFT : 0;
    for (int k = 0; k + 1 < n; k++)
      up_and_down_together |= e[k].action == HB_UP && e[k + 1].action == HB_DOWN;
  }
  CHECK(!up_and_down_together && c.last_flick == HB_FLICK_LEFT);

  /* a held tap keeps the flick waiting */
  hb_controls_init(&c, W, H);
  f = frame();
  f.tap_held = 1;
  run(&c, &f, 5);
  f.flick_press = HB_FLICK_RIGHT;
  run(&c, &f, 1);
  f.flick_press = 0;
  run(&c, &f, 15);
  CHECK(g_n == 0);
  CHECK(c.last_flick == HB_FLICK_NONE);
  f.tap_held = 0;
  run(&c, &f, 2);
  CHECK(c.last_flick == HB_FLICK_RIGHT);

  /* a tap ends a flick first, and drops one that was waiting */
  hb_controls_init(&c, W, H);
  press(&c, HB_FLICK_UP, 2);
  CHECK(c.flick != HB_FLICK_NONE);
  f = frame();
  f.flick_press = HB_FLICK_LEFT;
  run(&c, &f, 1);
  f.flick_press = 0;
  f.tap_held = 1;
  run(&c, &f, 1);
  CHECK(g_n >= 2 && g_ev[0].action == HB_UP && g_ev[1].action == HB_DOWN);
  CHECK(c.flick == HB_FLICK_NONE);
  f.tap_held = 0;
  run(&c, &f, 30);
  CHECK(c.last_flick == HB_FLICK_UP);

  /* a tap lands on the cursor, and a drag follows it */
  hb_controls_init(&c, W, H);
  f = frame();
  f.tap_held = 1;
  run(&c, &f, 1);
  CHECK(g_n == 1 && g_ev[0].action == HB_DOWN && g_ev[0].x == c.x && g_ev[0].y == c.y);
  f.lx = 1;
  run(&c, &f, 5);
  CHECK(count(HB_MOVE) == 5 && g_ev[g_n - 1].x > W / 2);

  /* the cursor stays on screen */
  hb_controls_init(&c, W, H);
  f = frame();
  f.lx = 1, f.ly = 1;
  run(&c, &f, 600);
  CHECK(c.x == W - 1 && c.y == 0);

  /* the right stick moves it the same way, but slower */
  HbControls fast, slow;
  hb_controls_init(&fast, W, H);
  hb_controls_init(&slow, W, H);
  f = frame();
  f.lx = 1;
  run(&fast, &f, 5);
  f = frame();
  f.rx = 1;
  run(&slow, &f, 5);
  const float gone_fast = fast.x - W / 2, gone_slow = slow.x - W / 2;
  CHECK(gone_slow > 0 && gone_slow < gone_fast * 0.5f);
  CHECK(g_n == 0); /* and it is no flick */
  f.rx = 0, f.ry = 1;
  run(&slow, &f, 5);
  CHECK(slow.y < H / 2);

  /* near the edge a flick stops at the edge */
  hb_controls_init(&c, W, H);
  hb_controls_set(&c, 400, 10);
  press(&c, HB_FLICK_RIGHT, 12);
  CHECK(g_ev[g_n - 1].x == W - 1);

  printf(fails ? "%d FAILED\n" : "all passed\n", fails);
  return fails ? 1 : 0;
}
