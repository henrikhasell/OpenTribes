# Replaces Tools/projectGenerator/modules/*.inc, minus everything the migration
# drops. The generator also asked for four directories that do not exist
# (gfx/D3D, windowManager/torque, physics) or differ only in case
# (shaderGen/glsl, shaderGen/hlsl); those are corrected below.

set(_engine_dirs
    # core.inc
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
    gui/hud
    gui/shell
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
    sfx/openal
    shaderGen
    sim
    unit
    util
    util/messaging
    windowManager

    # GL-only render path
    gfx/gl
    shaderGen/GLSL

    # T3D.inc
    T3D
    T3D/decal
    T3D/examples
    T3D/fps
    T3D/fx
    T3D/gameBase
    T3D/gameBase/std
    T3D/physics
    T3D/sfx
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

    # advancedLighting.inc / basicLighting.inc
    lighting/advanced
    lighting/advanced/glsl
    lighting/basic
    lighting/shadowMap

    # theora.inc
    core/ogg
    gui/theora)

# T3D/gameBase/hifi is off by default ($TORQUE_HIFI_NET in project.conf).

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
        message(FATAL_ERROR "engine source directory '${dir}' does not exist")
    endif()
    file(GLOB _found
        "${ENGINE_SOURCE_DIR}/${dir}/*.cpp"
        "${ENGINE_SOURCE_DIR}/${dir}/*.c"
        "${ENGINE_SOURCE_DIR}/${dir}/*.cc")
    list(APPEND TORQUE_ENGINE_SOURCES ${_found})
endforeach()

# Windows- and macOS-only translation units living alongside portable ones.
# The original build did this with setDontCompilePatterns() in targets.inc.
set(_engine_exclude
    "\\.win\\.cpp$"
    "\\.mac\\.cpp$"
    "win32.*\\.cpp$"
    "D3D.*\\.cpp$")

foreach(pattern IN LISTS _engine_exclude)
    list(FILTER TORQUE_ENGINE_SOURCES EXCLUDE REGEX "${pattern}")
endforeach()

list(LENGTH TORQUE_ENGINE_SOURCES TORQUE_ENGINE_SOURCE_COUNT)
