/* port_config.h -- Humbug's settings for the
 * android32 runtime (taken over from aks796's dcr_sea_nx).
 *
 * Macros only: the runtime's C files, its assembly and the launcher all read
 * this (runtime/source/rt_settings.h). What each setting does is next to its
 * default in the runtime; runtime/docs/ lists them all. MIT.
 */
#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/* ------------------------------------------------------------------ the game */
#define PORT_TITLE    "Humbug"
#define PORT_NAME     "humbug_nx"
#define PORT_PACKAGE  "com.dunderbit.humbug"
#define PORT_BANNER   "humbug_nx: Humbug (Unity 2019.2.15f1 / IL2CPP, armeabi-v7a)"
/* each module has a region: libil2cpp is 21 MB on disk, with room;
 * under-reserving shows up as so_load() -3 */
#define PORT_SO_REGION_BYTES (64u * 1024 * 1024)

/* The APK, by its package: it becomes game.apk. The game's data is in its
 * expansion file, main.<version>.com.turner.ttgfigures.obb, in obb/ of the
 * game folder (dcr_path.c maps Android/obb/<package>/ there). Humbug has none:
 * its data is inside the APK. */
#define PORT_APK_DESC "Humbug (armeabi-v7a)"
#define PORT_APK_ROLES                                                                        \
  {.what = "Humbug", .name = "game.apk",   \
   .package = "com.dunderbit.humbug", .flags = RT_APK_ADOPT}
#define PORT_LAUNCHER_START_NOTE "(the first start unpacks the game's libraries from the APK)"
#define PORT_LAUNCHER_BYLINE     "the game by Dunderbit; Switch wrapper on android32"

/* ------------------------------------------------------------------ libc */
#define RT_PROC_COMM        "dunderbit.humbu"
#define RT_DL_HAS_OPENSLES  0 /* FMOD falls back to its Java output (dcr_audio.c) */
#define RT_ZLIB_HOLDBACK    0

/* ------------------------------------------------------------------ JNI, NDK */
#define RT_JNI_UNHANDLED_INSTANCE_SINGLETON   1 /* Unity's C# plugins */
#define RT_JNI_UNHANDLED_BUILDER_RETURNS_SELF 1
#define RT_LOOPER_MAIN_IMPLICIT               0

/* ------------------------------------------------------------------ frames */
#define RT_GL_SWAP_ENDS_FRAME 0 /* the frame is nativeRender (dcr_boot.c) */
#define RT_CAPTURE_TRIES      300
#define RT_BOOST_LAUNCH_AFTER_PICTURE_MS 25000
#define RT_MAIN_THREAD_NAME   "UnityMain"
#define RT_PAD_MAX_PLAYERS    1

/* ------------------------------------------------------------------ watchdog */
#define RT_WATCHDOG_PRIO 0x1C
#define RT_WATCHDOG_CORE 2
#define RT_WATCHDOG_BOOT 1

/* ------------------------------------------------------------------ files */
#define RT_PATH_CWD_RELATIVE   0
#define RT_DIRCACHE            0 /* Unity probes and writes under data/ */
#define RT_MANIFEST_LOG_PREFIX "unity"
#define RT_EMU_FIXUPS          0 /* its self-fix runs after the module fix (dcr_main.c) */

#endif
