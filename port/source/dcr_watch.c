/* dcr_watch.c -- Humbug's part of the hang reports (the runtime's
 * watchdog.c does the rest): the GC's thread suspensions (il2cpp_rt.c), printed
 * with each report. A rising counter means slow, not stuck. MIT.
 */
#include <stdint.h>

#include "watchdog.h"

uint32_t dcr_gc_suspends(void); /* il2cpp_rt.c */

void dcr_watch_counters(void) { rt_watchdog_add_counter("GC suspends", dcr_gc_suspends); }
