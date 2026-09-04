# All dependencies are fetched and built here. Nothing comes from the system.
# GIT_TAG is always an exact tag or SHA. For offline builds pass
# -DFETCHCONTENT_SOURCE_DIR_<NAME>=/path/to/clone.

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

find_package(Threads REQUIRED)

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

# Arriving with the phase that consumes them: SDL3_ttf (5), SDL3_image (6),
# SDL3_net (7), openal-soft (8), zlib and the Xiph libraries when their
# vendored copies retire. SDL3_image vendors its own libpng and zlib -- pick a
# single owner in Phase 6 or the targets collide.
