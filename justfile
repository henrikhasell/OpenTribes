builddir  := "build"
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

# Play Apple Picker.
run config="linux-release": (stage config)
    cd "{{project}}" && ./OpenTribes

# The stock Full template, for comparing against unmodified engine behaviour.
run-template config="linux-release": (_stage template config)
    cd "{{template}}" && ./OpenTribes

# Stage the binary and fonts without launching, for running by hand.
stage config="linux-release": (_stage project config)
    @echo "staged in {{project}} -- run ./OpenTribes there"

_stage dir config: (_build config)
    cp {{builddir}}/{{config}}/game/OpenTribes "{{dir}}/OpenTribes"
    rm -rf "{{dir}}/fonts"
    cp -r {{builddir}}/{{config}}/game/fonts "{{dir}}/fonts"
    rm -f "{{dir}}/core/fonts/"*.uft
    find "{{dir}}" -name 'prefs.cs' -delete
    just _clearcaches "{{dir}}"

_build config:
    just {{config}}

_clearcaches dir:
    rm -f "{{dir}}/shaders/procedural/"*.glsl "{{dir}}/shaders/procedural/"*.hlsl
    find "{{dir}}" -name '*_basetex.dds' -delete

clean:
    rm -rf {{builddir}}
