/* hb_game.h -- the game's own Restart, Undo and menu buttons. MIT. */
#ifndef HB_GAME_H
#define HB_GAME_H

/* After the modules are bound (dcr_main.c). */
void hb_game_install(void);

/* Main thread, between frames. Each does nothing outside a level. */
void hb_game_restart(void);
void hb_game_undo(void);
void hb_game_pause(void);

#endif
