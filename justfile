builddir  := "build"
gamedata  := env_var_or_default("TRIBES2_DIR", "")
project   := "My Projects/Apple Picker/game"
template  := "Templates/Full/game"
toolchain := "cmake/mingw-w64.cmake"

default: linux-release

linux-debug:
    cmake -S . -B {{builddir}}/linux-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build {{builddir}}/linux-debug

linux-release:
    cmake -S . -B {{builddir}}/linux-release -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build {{builddir}}/linux-release

windows-debug:
    cmake -S . -B {{builddir}}/windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE={{toolchain}}
    cmake --build {{builddir}}/windows-debug

windows-release:
    cmake -S . -B {{builddir}}/windows-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE={{toolchain}}
    cmake --build {{builddir}}/windows-release

# Apple Picker is a standalone project now: its main.cs names apple-picker as
# the one mod and puts tools beside it, so there is no -game argument and the
# editors are on the mod path from the start.
# Play Apple Picker.
run config="linux-release": (stage config)
    cd "{{project}}" && ./OpenTribes

# Both editors need a tools build (-DTORQUE_TOOLS=ON, the default); without
# one main.cs leaves tools off the mod path and the flags do nothing.
# Open Apple Picker in the Mission Editor.
edit config="linux-release": (stage config)
    cd "{{project}}" && ./OpenTribes -worldEditor

# Open the GUI Editor.
guiedit config="linux-release": (stage config)
    cd "{{project}}" && ./OpenTribes -guiEditor

# The stock Full template, for comparing against unmodified engine behaviour.
run-template config="linux-release": (_stage template config)
    cd "{{template}}" && ./OpenTribes

# The engine roots itself at the executable's directory, so the binary is staged
# into the GameData rather than run from the build tree.  That is the whole
# directory mapping: no engine change, and the game's data is the game root.
#
# What the engine brings itself is symlinked in beside it -- its shaders, which
# a GameData has no reason to carry and without which the GL device dies on the
# first frame, and the fonts the build leaves next to the binary.  Links rather
# than copies so a rebuild is picked up and so ShaderGen's generated output
# lands under build/ instead of in the install.
#
# Both flags are the game's own: console_start.cs parses $Game::argv itself,
# calling setModPaths for -mod and telnetSetParameters for -telnetParams.  The
# passwords are empty, so a bare newline grants full read/write access, and
# nothing binds it to loopback.  See the notes in Apple Picker's defaults.cs.
#
# The directory defaults to $TRIBES2_DIR.
#
# Run against a Tribes 2 GameData with telnet open: just tribes [dir] [port]
tribes dir=gamedata port="4711" config="linux-release": (_engineassets config)
    @test -n "{{dir}}" || { echo "no GameData: pass one or set TRIBES2_DIR"; exit 1; }
    @test -f "{{dir}}/console_start.cs" || { echo "{{dir}} has no console_start.cs"; exit 1; }
    cp {{builddir}}/{{config}}/game/OpenTribes "{{dir}}/OpenTribes"
    ln -sfn "{{justfile_directory()}}/{{builddir}}/{{config}}/game/shaders" "{{dir}}/shaders"
    ln -sfn "{{justfile_directory()}}/{{builddir}}/{{config}}/game/fonts" "{{dir}}/fonts"
    cd "{{dir}}" && ./OpenTribes -telnetParams {{port}} "" "" -nologin -mod base

# The engine's shaders are not put beside the binary by CMake, and the tribes
# recipe links the GameData at them, so they have to be there to link to.
_engineassets config: (_build config)
    rm -rf {{builddir}}/{{config}}/game/shaders
    cp -r "{{template}}/shaders" {{builddir}}/{{config}}/game/shaders

# Stage the binary and fonts without launching, for running by hand.
stage config="linux-release": (_stage project config)
    @echo "staged in {{project}} -- run ./OpenTribes there"

# The engine resolves main.cs, its assets and fonts next to the executable, so
# both the binary and the staged fonts have to land in the game directory.
#
# Every prefs.cs is deleted rather than just core's: the engine exports $pref::*
# on exit, so a value a test harness flipped -- $pref::Decals::enabled is the
# one that has cost real time -- otherwise survives into every later run.
_stage dir config: (_build config)
    cp {{builddir}}/{{config}}/game/OpenTribes "{{dir}}/OpenTribes"
    rm -rf "{{dir}}/fonts"
    cp -r {{builddir}}/{{config}}/game/fonts "{{dir}}/fonts"
    rm -f "{{dir}}/core/fonts/"*.uft
    find "{{dir}}" -name 'prefs.cs' -delete
    just _clearcaches "{{dir}}"

_build config:
    just {{config}}

# Two things the engine writes beside the game data and then reuses without
# ever checking whether what produced them has changed.  Both survive a
# rebuild and a branch switch, and a stale one makes a fixed tree render
# exactly like the broken one.
#
#   shaders/procedural   ShaderGen names its output after a hash of the
#                        feature set, not of the code that generates it, so
#                        editing a shader feature leaves every cached shader
#                        in place.
#   *_basetex.dds        The terrain's composited base texture, which is not
#                        invalidated by baseTexSize or by an edit to any of
#                        the TerrainMaterials that went into it.
_clearcaches dir:
    rm -f "{{dir}}/shaders/procedural/"*.glsl "{{dir}}/shaders/procedural/"*.hlsl
    find "{{dir}}" -name '*_basetex.dds' -delete

clean:
    rm -rf {{builddir}}
