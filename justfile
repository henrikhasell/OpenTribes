builddir := "build"
config   := "release"
game     := justfile_directory() / builddir / config / "game"
shaders  := justfile_directory() / "My Projects/OpenTribes/game/shaders"
gamedata := env_var_or_default("TRIBES2_DIR", home_directory() + "/.wine/drive_c/Dynamix/Tribes2/GameData")

default: release

debug:
    cmake -S . -B {{builddir}}/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build {{builddir}}/debug

release:
    cmake -S . -B {{builddir}}/release -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build {{builddir}}/release

opentribes dir=gamedata: release
    cp "{{game}}/OpenTribes" "{{dir}}/OpenTribes"
    ln -sfn "{{shaders}}" "{{dir}}/shaders"
    cd "{{dir}}" && ./OpenTribes -nologin -telnetParams 2323 '' '' -mod Classic

stock dir=gamedata:
    cd "{{dir}}" && WINEPREFIX="{{home_directory()}}/.wine" /usr/bin/wine Tribes2.exe -nologin -telnetParams 2323 '' '' -mod Classic
