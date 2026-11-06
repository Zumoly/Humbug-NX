/* il2cpp_rt.c -- what IL2CPP's garbage collector needs from the OS, and does
 * not get from Horizon.
 *
 * STOP-THE-WORLD. libil2cpp's Boehm collector stops every other thread with
 * tgkill(pid, tid, SIG_SUSPEND) and restarts them with SIG_THR_RESTART; each
 * thread's signal handler records where its stack is, acknowledges on
 * GC_suspend_ack_sem and sleeps in sigsuspend until the world restarts. Horizon
 * has no signals, so the handler's work is done FOR the thread, from the
 * collector, as the handler in this libil2cpp does it (libil2cpp+0x1049d94):
 *
 *   suspend:  find the thread's GC_Thread in the GC_threads table; if its
 *             last_stop_count is GC_stop_count, stop (a retry for a thread
 *             already stopped). Otherwise pause the thread
 *             (svcSetThreadActivity), read its registers, push r0-r12 and lr
 *             below its sp so the conservative scan sees them, publish that as
 *             stack_ptr, post the semaphore, set last_stop_count.
 *   restart:  resume the thread, and acknowledge if signals are retried.
 *
 * Nothing in this path may log or allocate: another thread may be paused while
 * holding the log's or the allocator's lock. Counters are reported later, from
 * the main loop.
 *
 * The addresses are for Humbug 2.2.4's libil2cpp (Unity 2019.2.15f1): see
 * dcr_offsets.h. The handler's first words are checked before any of them is
 * used. Unlike Unity 2017.4's collector this one hashes thread ids, keeps
 * no tid in GC_Thread, and with GC_retry_signals also waits for restarts:
 * bit 0 of last_stop_count, and one more post of the semaphore. MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "bionic_pthread.h"
#include "dcr_offsets.h"
#include "dcr_patches.h"
#include "imports.h"
#include "so_util.h"
#include "util.h"

extern so_module il2cpp_mod;

#define RVA_SUSPEND_HANDLER HB_RVA_GC_HANDLER
#define k_guard_handler HB_GUARD_GC_HANDLER

/* GC_Thread, read off GC_suspend_handler */
typedef struct GcThread {
  struct GcThread *next;
  uintptr_t id; /* pthread_t */
  volatile uint32_t last_stop_count; /* bit 0: restarted, when signals are retried */
  void *volatile stack_ptr;
} GcThread;

typedef int (*fn_sem_post)(void *sem);

static fn_sem_post g_sem_post;
static volatile uint32_t *g_stop_count;
static volatile int *g_retry_signals;
static volatile int *g_sig_suspend, *g_sig_restart;
static GcThread **g_threads;
static void *g_ack_sem;
static int g_gc_ready;

static volatile uint32_t g_n_suspend, g_n_restart, g_n_retry, g_n_ctx_fail, g_n_unknown;
uint32_t dcr_gc_suspends(void) { return g_n_suspend; }

/* svcGetThreadContext3 with the kernel's own 0x320-byte ThreadContext. For an
 * AArch32 thread the kernel fills r[0..14] (sp = r[13], lr = r[14]). */
typedef struct {
  uint64_t r[29];
  uint64_t fp, lr, sp, pc;
  uint32_t psr, _pad;
  uint8_t v[32][16];
  uint32_t fpcr, fpsr;
  uint64_t tpidr;
} KThreadContext;
_Static_assert(sizeof(KThreadContext) == 0x320, "kernel ThreadContext is 0x320 bytes");

static Result get_thread_context(KThreadContext *ctx, Handle h) {
  register uint32_t r0 __asm__("r0") = (uint32_t)(uintptr_t)ctx;
  register uint32_t r1 __asm__("r1") = h;
  /* The 32-bit SVC ABI returns with r1-r3 zeroed: they must be clobbers. */
  __asm__ volatile("svc 0x33" : "+r"(r0), "+r"(r1) : : "r2", "r3", "r12", "lr", "memory");
  return r0;
}

int dcr_il2cpp_install(void) {
  const uintptr_t b = (uintptr_t)il2cpp_mod.load_virtbase;
  if (dcr_guard_ok(&il2cpp_mod, RVA_SUSPEND_HANDLER, k_guard_handler, "GC suspend handler")) {
    g_stop_count = (volatile uint32_t *)(b + HB_RVA_GC_STOP_COUNT);
    g_retry_signals = (volatile int *)(b + HB_RVA_GC_RETRY_SIGNALS);
    g_sig_suspend = (volatile int *)(b + HB_RVA_GC_SIG_SUSPEND);
    g_sig_restart = (volatile int *)(b + HB_RVA_GC_SIG_RESTART);
    g_threads = (GcThread **)(b + HB_RVA_GC_THREADS);
    g_ack_sem = (void *)(b + HB_RVA_GC_ACK_SEM);
    g_sem_post = (fn_sem_post)dcr_import_lookup("sem_post");
    g_gc_ready = g_sem_post != NULL;
  }
  debugPrintf("[il2cpp] GC bridge %s\n", g_gc_ready ? "armed" : "NOT ARMED -- the first collection will hang");
  return g_gc_ready;
}

static GcThread *gc_lookup(const BThread *t) {
  const uintptr_t id = (uintptr_t)t;
  GcThread *me = g_threads[(id ^ (id >> 8) ^ (id >> 16)) & 0xff];
  while (me && me->id != (uintptr_t)t)
    me = me->next;
  return me;
}

static int in_stack(const BThread *t, uintptr_t p) {
  uintptr_t lo = (uintptr_t)t->stack_base;
  return lo && p >= lo && p <= lo + t->stack_size;
}

static void gc_suspend(BThread *t) {
  GcThread *me = gc_lookup(t);
  if (!me) {
    g_n_unknown++;
    return; /* not a GC thread: the collector does not wait for it */
  }
  const uint32_t stop = *g_stop_count;
  if ((me->last_stop_count & ~1u) == stop) {
    g_n_retry++;
    return;
  }
  KThreadContext ctx;
  uintptr_t sp = 0;
  b_pause_lock();
  svcSetThreadActivity(t->handle, ThreadActivity_Paused);
  t->gc_paused = 1;
  if (R_SUCCEEDED(get_thread_context(&ctx, t->handle)))
    sp = (uintptr_t)ctx.r[13];
  b_pause_unlock();
  if (!in_stack(t, sp)) {
    /* Unknown sp: scan the whole stack. Conservative, never unsafe. */
    g_n_ctx_fail++;
    me->stack_ptr = t->stack_base;
  } else {
    uint32_t *save = (uint32_t *)((sp - 16 * 4) & ~7u);
    for (int i = 0; i < 13; i++)
      save[i] = (uint32_t)ctx.r[i];
    save[13] = (uint32_t)ctx.r[14];
    me->stack_ptr = save;
  }
  me->last_stop_count = stop;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  g_n_suspend++;
  g_sem_post(g_ack_sem);
}

static void gc_restart(BThread *t) {
  b_pause_lock();
  if (t->gc_paused) {
    t->gc_paused = 0;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    svcSetThreadActivity(t->handle, ThreadActivity_Runnable);
  }
  b_pause_unlock();
  g_n_restart++;
  if (*g_retry_signals) {
    GcThread *me = gc_lookup(t);
    if (me)
      me->last_stop_count = *g_stop_count | 1;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    g_sem_post(g_ack_sem);
  }
}

/* Called by bionic_signal.c for every tkill/pthread_kill. */
int port_gc_signal(BThread *t, int sig) {
  if (!g_gc_ready || !t)
    return 0;
  if (sig == *g_sig_suspend) {
    gc_suspend(t);
    return 1;
  }
  if (sig == *g_sig_restart) {
    gc_restart(t);
    return 1;
  }
  return 0;
}

int b_tkill(int tid, int sig);
int b_tgkill(int pid, int tid, int sig) {
  (void)pid;
  return b_tkill(tid, sig);
}

void dcr_il2cpp_report(void) {
  if (!g_n_suspend && !g_n_unknown)
    return;
  debugPrintf("[il2cpp] GC bridge: %lu suspends, %lu restarts, %lu retries, %lu full-stack scans, %lu non-GC threads\n",
              (unsigned long)g_n_suspend, (unsigned long)g_n_restart, (unsigned long)g_n_retry,
              (unsigned long)g_n_ctx_fail, (unsigned long)g_n_unknown);
}

/* Humbug plays no intro movie through su_video.c. */
void dcr_il2cpp_video_done(void) {}

/* dcr_prof.c's names for managed frames came from Mono's tables, and its
 * stop-the-world total from the Mono bridge. */
int dcr_mono_method_name(uint32_t addr, char *buf, size_t cap) {
  (void)addr, (void)buf, (void)cap;
  return 0;
}

uint64_t dcr_gc_stopped_ns(void) { return 0; }
