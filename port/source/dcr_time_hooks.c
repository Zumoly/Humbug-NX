/* dcr_time_hooks.c -- nothing to hook. Unity 2019.2 keeps its Time functions in
 * libil2cpp's icall table, and the engine's own clock already stops across
 * sleep and HOME (the runtime's clock_gettime), so the game keeps its clock.
 * MIT. */
#include "dcr_time.h"

void dcr_time_tick(void) {}
