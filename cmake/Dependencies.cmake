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

# --- SDL3_ttf ---------------------------------------------------------------
set(SDLTTF_VENDORED   ON  CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES    OFF CACHE BOOL "" FORCE)
set(SDLTTF_INSTALL    OFF CACHE BOOL "" FORCE)
set(SDLTTF_HARFBUZZ   OFF CACHE BOOL "" FORCE)
set(SDLTTF_PLUTOSVG   OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG        release-3.2.2
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(SDL3_ttf)

# The engine asks for "Arial", which SDL3_ttf cannot resolve -- it has no font
# discovery, and reaching for fontconfig would put platform knowledge back into
# the platform layer. Ship a font instead and map requested names onto it.
FetchContent_Declare(dejavu
    URL      https://github.com/dejavu-fonts/dejavu-fonts/releases/download/version_2_37/dejavu-fonts-ttf-2.37.tar.bz2
    URL_HASH SHA256=fa9ca4d13871dd122f61258a80d01751d603b4d3ee14095d65453b4e846e17d7
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(dejavu)

set(TORQUE_FONT_DIR "${dejavu_SOURCE_DIR}/ttf" CACHE INTERNAL "")

# --- openal-soft ------------------------------------------------------------
set(ALSOFT_UTILS         OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS         OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL       OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(LIBTYPE              "STATIC" CACHE STRING "" FORCE)

FetchContent_Declare(openal
    GIT_REPOSITORY https://github.com/kcat/openal-soft.git
    GIT_TAG        1.24.3
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(openal)

# --- SDL3_image -------------------------------------------------------------
# STB backend rather than vendored libpng/libjpeg: it decodes everything the
# content uses without pulling in a second zlib and libpng alongside the ones
# Engine/lib already carries for the zip reader and the Xiph libraries.
set(SDLIMAGE_VENDORED    OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON  CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES     OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS       OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_INSTALL     OFF CACHE BOOL "" FORCE)

# These four are the only backends that reach outside the checkout, and they
# would come from the system. Nothing in the content is AVIF, TIFF, WebP or JXL.
set(SDLIMAGE_AVIF        OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF         OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP        OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL         OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-3.2.4
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(SDL3_image)

# --- SDL3_net ---------------------------------------------------------------
set(SDLNET_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLNET_TESTS   OFF CACHE BOOL "" FORCE)
set(SDLNET_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3_net
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
    GIT_TAG        release-3.2.0
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(SDL3_net)

# --- glad -------------------------------------------------------------------
# The GL loader, generated at build time rather than checked in. REPRODUCIBLE
# makes it read the gl.xml and khrplatform.h packaged inside glad instead of
# fetching whatever is at the head of the Khronos registry, so GIT_TAG pins the
# generated bindings the same way it pins everything else here.
set(GLAD_PROFILE        "core"    CACHE STRING "" FORCE)
set(GLAD_API            "gl=4.5"  CACHE STRING "" FORCE)
set(GLAD_GENERATOR      "c"       CACHE STRING "" FORCE)
set(GLAD_REPRODUCIBLE   ON  CACHE BOOL "" FORCE)
set(GLAD_ALL_EXTENSIONS ON  CACHE BOOL "" FORCE)
set(GLAD_INSTALL        OFF CACHE BOOL "" FORCE)

# glad 0.1 asks for CMake 3.0, which CMake 4 refuses outright.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(glad)

unset(CMAKE_POLICY_VERSION_MINIMUM)

# Still to arrive: zlib and the Xiph libraries when their vendored copies
# retire.
