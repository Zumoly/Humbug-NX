/* hb_stubs.c -- the few libc imports of Unity 2019.2 the runtime does not have.
 * None of them matters on a console with no network interfaces or named
 * semaphores. MIT. */
#include <errno.h>
#include <stddef.h>

#include "imports.h"

/* Libraries look this one up with dlsym, so the generated table lacks it. */
unsigned long b_getauxval(unsigned long type);
const DynLibFunction port_imports[] = {{"getauxval", (uintptr_t)b_getauxval}};
const int port_imports_count = 1;

unsigned b_if_nametoindex(const char *name) {
  (void)name;
  return 0;
}

int b_msync(void *addr, size_t len, int flags) {
  (void)addr, (void)len, (void)flags;
  return 0;
}

void *b_sem_open(const char *name, int flags, ...) {
  (void)name, (void)flags;
  errno = ENOSYS;
  return NULL;
}
