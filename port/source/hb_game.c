/* hb_game.c -- Y, X and Plus press the buttons on the game's own level screen.
 *
 * Game.UI.Game.GameMenu holds Restart, Undo and the way back to the main menu.
 * Humbug has no pause screen: leaving a level for the main menu keeps it for
 * Resume, so that is what Plus does.
 *
 * C# hands the menu to nobody, so it is caught at SetRestartEnabled and
 * SetUndoEnabled, which the game calls on it as levels start and moves finish.
 * The replacements do what the originals do: set the button's interactable.
 * Those flags also say whether Y and X may press, just as the buttons would allow.
 *
 * The addresses are in dcr_offsets.h, each with a guard. MIT.
 */
#include <stdint.h>
#include <switch.h>

#include "dcr_offsets.h"
#include "dcr_patches.h"
#include "hb_game.h"
#include "util.h"

extern so_module il2cpp_mod;

typedef void (*fn_self)(void *self, void *method);
typedef void (*fn_set_bool)(void *self, int value, void *method);
typedef void *(*fn_get_obj)(void *self, void *method);
typedef int (*fn_get_bool)(void *self, void *method);

static fn_self g_restart, g_undo, g_main_menu;
static fn_set_bool g_set_interactable;
static fn_get_obj g_get_game_object;
static fn_get_bool g_active_in_hierarchy;
static void *volatile g_menu;
static volatile int g_restart_ok, g_undo_ok;

/* read off SetRestartEnabled and SetUndoEnabled */
#define GAMEMENU_RESTART_BUTTON 0x48
#define GAMEMENU_UNDO_BUTTON 0x4c

static void set_button(void *self, int offset, int enabled) {
  g_menu = self;
  void *button = *(void **)((char *)self + offset);
  if (button)
    g_set_interactable(button, enabled, NULL);
}

static void w_set_restart_enabled(void *self, int enabled, void *method) {
  (void)method;
  g_restart_ok = enabled & 1;
  set_button(self, GAMEMENU_RESTART_BUTTON, enabled);
}

static void w_set_undo_enabled(void *self, int enabled, void *method) {
  (void)method;
  g_undo_ok = enabled & 1;
  set_button(self, GAMEMENU_UNDO_BUTTON, enabled);
}

static uintptr_t site(const DcrSite *s) {
  if (!dcr_guard_ok(&il2cpp_mod, s->rva, s->guard, s->name))
    return 0;
  return (uintptr_t)il2cpp_mod.load_virtbase + s->rva;
}

void hb_game_install(void) {
  g_restart = (fn_self)site(&HB_SITE_RestartClicked);
  g_undo = (fn_self)site(&HB_SITE_UndoClicked);
  g_main_menu = (fn_self)site(&HB_SITE_MainMenuClicked);
  g_set_interactable = (fn_set_bool)site(&HB_SITE_SetInteractable);
  g_get_game_object = (fn_get_obj)site(&HB_SITE_GetGameObject);
  g_active_in_hierarchy = (fn_get_bool)site(&HB_SITE_ActiveInHierarchy);
  int hooked = 0;
  if (g_set_interactable) {
    hooked += dcr_hook(&il2cpp_mod, HB_SITE_SetRestartEnabled.rva, HB_SITE_SetRestartEnabled.guard,
                       (void *)w_set_restart_enabled, HB_SITE_SetRestartEnabled.name);
    hooked += dcr_hook(&il2cpp_mod, HB_SITE_SetUndoEnabled.rva, HB_SITE_SetUndoEnabled.guard,
                       (void *)w_set_undo_enabled, HB_SITE_SetUndoEnabled.name);
  }
  debugPrintf("[game] Restart %s, Undo %s, menu %s, level check %s, GameMenu watch %d/2\n", g_restart ? "ok" : "MISSING",
              g_undo ? "ok" : "MISSING", g_main_menu ? "ok" : "MISSING",
              g_get_game_object && g_active_in_hierarchy ? "ok" : "MISSING", hooked);
}

/* The menu, while the level screen is what the player sees. */
static void *level_menu(void) {
  void *menu = g_menu;
  if (!menu || !g_get_game_object || !g_active_in_hierarchy)
    return NULL;
  void *go = g_get_game_object(menu, NULL);
  return go && g_active_in_hierarchy(go, NULL) ? menu : NULL;
}

static int logged_press(const char *what, void *menu, int allowed) {
  static int n;
  if (n++ < 24)
    debugPrintf("[game] %s: %s\n", what, !menu ? "not on a level" : allowed ? "pressed" : "button is off");
  return menu && allowed;
}

void hb_game_restart(void) {
  void *m = level_menu();
  if (logged_press("restart", m, g_restart_ok) && g_restart)
    g_restart(m, NULL);
}

void hb_game_undo(void) {
  void *m = level_menu();
  if (logged_press("undo", m, g_undo_ok) && g_undo)
    g_undo(m, NULL);
}

void hb_game_pause(void) {
  void *m = level_menu();
  if (logged_press("pause", m, 1) && g_main_menu)
    g_main_menu(m, NULL);
}
