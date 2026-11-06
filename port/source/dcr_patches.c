/* dcr_patches.c -- the engine patches, each guard-checked against the game's
 * own code before a single word is written.
 *
 * CHOREOGRAPHER. Unity 2019.2's Android frame pacing is a Java FrameCallback
 * that bumps a counter WaitVSync() sleeps on. Two engine functions start and
 * stop it through a singleton that needs a Java Looper, which does not exist
 * here. Both become `bx lr`, and dcr_vsync.c pumps the counter instead.
 * libunity is Thumb code in this build, so the patch is the 16-bit bx lr.
 * MIT.
 */
#include <string.h>

#include "config.h"
#include "dcr_offsets.h"
#include "dcr_patches.h"
#include "util.h"

extern so_module unity_mod;

#define THUMB_BX_LR 0x4770u

int dcr_guard_ok(so_module *mod, uint32_t rva, const uint32_t guard[4], const char *what) {
  const uint32_t *p = (const uint32_t *)((uintptr_t)mod->load_virtbase + rva);
  if (!memcmp(p, guard, 16))
    return 1;
  debugPrintf("[patch] %s at +0x%lx: GUARD MISMATCH (found %08lx %08lx %08lx %08lx) -- "
              "not touched; regenerate dcr_offsets.h for this build\n",
              what, (unsigned long)rva, (unsigned long)p[0], (unsigned long)p[1],
              (unsigned long)p[2], (unsigned long)p[3]);
  return 0;
}

int dcr_hook(so_module *mod, uint32_t rva, const uint32_t guard[4], void *dst, const char *what) {
  if (!dcr_guard_ok(mod, rva, guard, what))
    return 0;
  hook_arm((uintptr_t)mod->load_virtbase + rva, (uintptr_t)dst);
  return 1;
}

static int patch_ret_thumb(uint32_t rva, const uint32_t guard[4], const char *what) {
  if (!dcr_guard_ok(&unity_mod, rva, guard, what))
    return 0;
  const uint16_t insn = THUMB_BX_LR;
  if (so_patch_code((void *)((uintptr_t)unity_mod.load_virtbase + rva), &insn, 2) != 0) {
    debugPrintf("[patch] %s: write failed\n", what);
    return 0;
  }
  debugPrintf("[patch] %s -> bx lr\n", what);
  return 1;
}

void dcr_patches_apply(void) {
#if DCR_PATCH_CHOREOGRAPHER
  int n = 0;
  n += patch_ret_thumb(DCR_RVA_ChoreographerEnableVSync, DCR_GUARD_ChoreographerEnableVSync,
                       "ChoreographerEnableVSync");
  n += patch_ret_thumb(DCR_RVA_ChoreographerStopVSync, DCR_GUARD_ChoreographerStopVSync,
                       "ChoreographerStopVSync");
  debugPrintf("[patch] Choreographer: %d/2 patched\n", n);
#endif
}
