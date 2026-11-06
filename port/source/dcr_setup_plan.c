/* dcr_setup_plan.c -- Humbug's part of the first
 * launch: its setup plan for the runtime's dcr_setup.c.
 *
 * The runtime's dcr_setup.c does the work from this plan: game.apk rewritten
 * stored when its assets/ are compressed, then the three libraries, then
 * classes.txt, then "Starting the game".
 *
 * .setup keys, which must not change: libmain.so, libunity.so, libil2cpp.so,
 * classes.txt. MIT.
 */
#include <stdio.h>
#include <string.h>

#include "dcr_path.h"
#include "dcr_setup.h"
#include "rt_settings.h"
#include "util.h"

static const char *const k_libs[] = {"libmain.so", "libunity.so", "libil2cpp.so"};

const RtSetupPlan port_setup_plan = {
    .libs = k_libs,
    .nlibs = sizeof k_libs / sizeof k_libs[0],
    .libs_what = "Unpacking the game's libraries",
    .apk_requirement = "This port needs the 32-bit ARM (armeabi-v7a) build of Humbug\n"
                       "(com.dunderbit.humbug): use an APK with lib/armeabi-v7a.",
    .libs_p0 = 820,
    .libs_p1 = 960,
    .classes_p0 = 960,
    .classes_p1 = 990,
    .store_apk_prefix = "assets/",
    .store_p0 = 0,
    .store_p1 = 800,
};
