#---------------------------------------------------------------------------------
# Humbug -- Nintendo Switch wrapper (32-bit / AArch32, Unity 2019.2 IL2CPP)
#
# Ships NO game code and NO game assets: the game's own APK and expansion
# file (the player's copy) are read at run time, and its libraries are
# unpacked from the APK on the first launch.
#
# Built on aks796's dcr_sea_nx (Disney Crossy Road, MIT): the same Unity layer,
# with the Mono parts replaced by IL2CPP's (source/il2cpp_rt.c).
#
# The build is the android32 runtime's (runtime/runtime.mk: devkitARM +
# libnx32 + mesa32 from portlibs32/); ./build.sh runs it in the toolchain
# container. Output: teenytitans_nx.nsp, which the launcher NRO carries (launcher/).
#---------------------------------------------------------------------------------
TARGET               := humbug_nx
PORT_NPDM_PROGRAM_ID := 0x0100D4B81C7A5E30
PORT_NPDM_VERSION    := 0.1.0
# The game's movies (Handheld.PlayFullScreenMovie: the logos): FFmpeg's MP4
# demuxer and H.264 / AAC decoders, built by ffmpeg32's build.sh (LGPL) and
# copied into portlibs32/. Without them su_video.c skips the movies.
DCR_VIDEO := $(if $(wildcard portlibs32/lib/libavcodec.a),1,0)
ifeq ($(DCR_VIDEO),1)
PORT_LIBS := -L$(CURDIR)/portlibs32/lib -lavformat -lavcodec -lavutil
endif
include runtime/runtime.mk

# FFmpeg is built with int-sized enums (-fno-short-enums), and so is its one
# user here.
$(BUILD)/su_video.o: $(SOURCES)/su_video.c $(RENDERER_STAMP) | $(BUILD)
	@echo $(notdir $<)
	@$(CC) -MMD -MP $(CFLAGS) -fno-short-enums -DDCR_VIDEO=$(DCR_VIDEO) -c $< -o $@
