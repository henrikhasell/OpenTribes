# External dependencies.
#
# Policy: everything is fetched and built by CMake. Nothing is taken from the
# system, and no find_package/pkg-config is used for a project dependency.
# GIT_TAG is always an exact tag or commit SHA, never a branch, so a given
# checkout always resolves to the same dependency tree.
#
# Offline builds: pass -DFETCHCONTENT_SOURCE_DIR_<NAME>=/path/to/clone to point
# any entry at a local checkout without touching this file.

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

find_package(Threads REQUIRED)   # toolchain feature, not a project dependency

# --- SDL3 -------------------------------------------------------------------
# Phase 3 replaces platformWin32/platformX86UNIX/platformMac with a single SDL3
# implementation. Fetched here so Phase 0 proves the policy end to end.
set(SDL_SHARED         OFF CACHE BOOL "" FORCE)
set(SDL_STATIC         ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY   OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.24
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(SDL3)

# --- Deferred to their own phases -------------------------------------------
#
# These are deliberately not fetched yet. Each arrives with the phase that
# actually consumes it, so the cold-build cost lands incrementally rather than
# all at once in Phase 0:
#
#   SDL3_ttf     -> Phase 5, replaces the three per-platform PlatformFont impls
#   SDL3_image   -> Phase 6, replaces gfx/bitmap/loaders/ and retires
#                   Engine/lib/{lpng,ljpeg,lmng,lungif}
#   SDL3_net     -> Phase 7, replaces platform/platformNet.cpp
#   openal-soft  -> Phase 8, replaces Engine/lib/openal's per-platform headers
#                   and the LoadOAL dlopen shims
#   zlib, libogg, libvorbis, libtheora
#                -> fetched when their vendored copies in Engine/lib are retired
#
# zlib ownership: when SDL3_image lands in Phase 6, its vendored path pulls its
# own libpng, which pulls its own zlib. That collides with a fetched zlib target.
# Resolve it there by disabling SDL3_image's vendoring and handing it our
# targets -- do not let both exist.
