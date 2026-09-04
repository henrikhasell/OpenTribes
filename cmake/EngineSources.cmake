# Engine source set.
#
# Replaces Tools/projectGenerator/modules/*.inc. The directory lists below are
# transcribed from core.inc, T3D.inc, advancedLighting.inc, basicLighting.inc,
# theora.inc, collada.inc and tools.inc, minus everything the migration drops.
#
# Corrections made while transcribing -- the generator asked for directories
# that do not exist in this tree, and nobody noticed because the Linux target
# was never built:
#
#   core.inc:135  gfx/D3D              -> does not exist (only gfx/D3D9 does)
#   core.inc:153  shaderGen/glsl       -> directory is shaderGen/GLSL
#   core.inc:138  shaderGen/hlsl       -> directory is shaderGen/HLSL
#   core.inc      windowManager/torque -> does not exist
#   T3D.inc       physics              -> does not exist (it is T3D/physics)
#
# Case matters here in a way it never did for the Windows and macOS builds.

set(_engine_dirs
    # --- core.inc ---------------------------------------------------------
    app
    app/net
    cinterface
    component
    component/interfaces
    console
    core
    core/stream
    core/strings
    core/util
    core/util/journal
    core/util/zip
    core/util/zip/compressors
    gfx
    gfx/Null
    gfx/bitmap
    gfx/bitmap/loaders
    gfx/sim
    gfx/util
    gfx/video
    gui
    gui/buttons
    gui/containers
    gui/controls
    gui/core
    gui/game
    gui/shiny
    gui/utility
    i18n
    math
    math/util
    platform
    platform/async
    platform/menus
    platform/nativeDialogs
    platform/sdl
    platform/threads
    sfx
    sfx/media
    sfx/null
    shaderGen
    sim
    unit
    util
    util/messaging
    windowManager

    # --- GL-only render path (D3D9/HLSL dropped) --------------------------
    gfx/gl
    gfx/gl/ggl
    gfx/gl/ggl/generated
    shaderGen/GLSL

    # --- T3D.inc ----------------------------------------------------------
    T3D
    T3D/decal
    T3D/examples
    T3D/fps
    T3D/fx
    T3D/gameBase
    T3D/gameBase/std
    T3D/physics
    T3D/sfx
    T3D/turret
    T3D/vehicles
    collision
    environment
    forest
    forest/glsl
    forest/ts
    gui/3d
    interior
    lighting
    lighting/common
    materials
    postFx
    renderInstance
    scene
    scene/culling
    scene/mixin
    scene/zones
    terrain
    terrain/glsl
    ts
    ts/arch

    # --- advancedLighting.inc / basicLighting.inc -------------------------
    lighting/advanced
    lighting/advanced/glsl
    lighting/basic
    lighting/shadowMap

    # --- theora.inc -------------------------------------------------------
    core/ogg
    gui/theora)

# T3D/gameBase/hifi is the alternative networking model, off by default in the
# original build ($TORQUE_HIFI_NET = false in project.conf).

if(TORQUE_TOOLS)
    list(APPEND _engine_dirs
        environment/editors
        forest/editor
        gui/editor
        gui/editor/inspector
        gui/worldEditor)
endif()

if(TORQUE_COLLADA)
    list(APPEND _engine_dirs
        ts/collada
        ts/loader)
endif()

if(TORQUE_TESTS)
    list(APPEND _engine_dirs
        core/util/journal/test
        core/util/test
        core/util/zip/unitTests
        gfx/test
        math/test
        platform/test
        unit/tests
        windowManager/test)
endif()

set(TORQUE_ENGINE_SOURCES "")
foreach(dir IN LISTS _engine_dirs)
    if(NOT IS_DIRECTORY "${ENGINE_SOURCE_DIR}/${dir}")
        message(FATAL_ERROR
            "engine source directory '${dir}' does not exist. If this is a "
            "case mismatch, note that the original build only ever ran on "
            "case-insensitive filesystems.")
    endif()
    file(GLOB _found
        "${ENGINE_SOURCE_DIR}/${dir}/*.cpp"
        "${ENGINE_SOURCE_DIR}/${dir}/*.c"
        "${ENGINE_SOURCE_DIR}/${dir}/*.cc")
    list(APPEND TORQUE_ENGINE_SOURCES ${_found})
endforeach()

# Excluded by filename rather than by directory. The original build did this
# with setDontCompilePatterns() in btargets/targets.inc; the Windows-only and
# macOS-only translation units live alongside portable ones.
set(_engine_exclude
    "/gfx/gl/ggl/mac/"
    "/gfx/gl/ggl/win32/"
    "/gfx/gl/ggl/x11/"       # Phase 4 replaces GLX binding with SDL_GL_GetProcAddress
    "\\.win\\.cpp$"
    "\\.mac\\.cpp$"
    "win32.*\\.cpp$"
    "D3D.*\\.cpp$")

foreach(pattern IN LISTS _engine_exclude)
    list(FILTER TORQUE_ENGINE_SOURCES EXCLUDE REGEX "${pattern}")
endforeach()

list(LENGTH TORQUE_ENGINE_SOURCES TORQUE_ENGINE_SOURCE_COUNT)
