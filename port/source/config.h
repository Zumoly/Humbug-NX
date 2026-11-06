/* config.h -- Humbug's own build-wide constants
 * (the runtime's settings are in port_config.h).
 *
 * Unity 2019.2.15f1, IL2CPP backend, armeabi-v7a; the data is in the APK. MIT.
 */
#ifndef DCR_CONFIG_H
#define DCR_CONFIG_H

#include "rt_settings.h"

/* The game's modules, and where Unity's data is inside the APK. */
#define DCR_LIB_MAIN    "libmain.so"
#define DCR_LIB_UNITY   "libunity.so"
#define DCR_LIB_IL2CPP  "libil2cpp.so"
#define DCR_DATA_DIR    "assets/bin/Data"
#define DCR_PKG_NAME    PORT_PACKAGE

/* Feature switches wired against dcr_offsets.h. */
#define DCR_PATCH_CHOREOGRAPHER 1   /* no Java looper here: pumped by dcr_vsync.c */
#define DCR_PATCH_VSYNC         1   /* pump the vsync counter from a thread   */


#endif /* DCR_CONFIG_H */
