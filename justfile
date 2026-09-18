builddir  := "build"
gamedir   := "Templates/Full/game"
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

# `game` is a directory name under the game folder, so `just run apple-picker`.
# Empty means the stock Full template, and is deliberately not spelled
# "-game scripts": -game replaces the whole directory list rather than adding
# to it, which would drop tools and with it the editor.
#
# The engine resolves main.cs, its assets and fonts next to the executable, so
# both the binary and the staged fonts have to land in the game directory.
run game="" config="linux-release": (_build config)
    cp {{builddir}}/{{config}}/game/OpenTribes {{gamedir}}/OpenTribes
    rm -rf {{gamedir}}/fonts
    cp -r {{builddir}}/{{config}}/game/fonts {{gamedir}}/fonts
    rm -f "{{gamedir}}/core/fonts/"*.uft
    rm -f {{gamedir}}/core/prefs.cs {{gamedir}}/scripts/client/prefs.cs {{gamedir}}/scripts/server/prefs.cs
    just _clearcaches
    cd {{gamedir}} && ./OpenTribes{{ if game == "" { "" } else { " -game " + game } }}

# Stage the binary and fonts without launching, for running by hand.
stage game="" config="linux-release": (_build config)
    cp {{builddir}}/{{config}}/game/OpenTribes {{gamedir}}/OpenTribes
    rm -rf {{gamedir}}/fonts
    cp -r {{builddir}}/{{config}}/game/fonts {{gamedir}}/fonts
    rm -f "{{gamedir}}/core/fonts/"*.uft
    rm -f {{gamedir}}/core/prefs.cs {{gamedir}}/scripts/client/prefs.cs {{gamedir}}/scripts/server/prefs.cs
    just _clearcaches
    @echo "staged in {{gamedir}} -- run ./OpenTribes{{ if game == "" { "" } else { " -game " + game } }} there"

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
_clearcaches:
    rm -f "{{gamedir}}/shaders/procedural/"*.glsl "{{gamedir}}/shaders/procedural/"*.hlsl
    find {{gamedir}} -name '*_basetex.dds' -delete

clean:
    rm -rf {{builddir}}
