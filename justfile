builddir    := "build"
config      := "release"
game        := justfile_directory() / builddir / config / "game"
shaders     := justfile_directory() / "My Projects/OpenTribes/game/shaders"
gamedata    := env_var_or_default("TRIBES2_DIR", home_directory() + "/.wine/drive_c/Dynamix/Tribes2/GameData")
telnet_port := "2324"
telnet_rw   := "rw"
telnet_ro   := "ro"

default: release

debug:
    cmake -S . -B {{builddir}}/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build {{builddir}}/debug

release:
    cmake -S . -B {{builddir}}/release -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build {{builddir}}/release

opentribes dir=gamedata telnet_port=telnet_port: release
    cp "{{game}}/OpenTribes" "{{dir}}/OpenTribes"
    ln -sfn "{{shaders}}" "{{dir}}/shaders"
    cd "{{dir}}" && ./OpenTribes -nologin -telnetParams {{telnet_port}} '{{telnet_rw}}' '{{telnet_ro}}' -mod Classic

stock dir=gamedata telnet_port=telnet_port:
    cd "{{dir}}" && WINEPREFIX="{{home_directory()}}/.wine" /usr/bin/wine Tribes2.exe -nologin -telnetParams {{telnet_port}} '{{telnet_rw}}' '{{telnet_ro}}' -mod Classic

telnet telnet_port=telnet_port:
    { printf '%s\n' '{{telnet_rw}}'; cat; } | socat - TCP:127.0.0.1:{{telnet_port}},crnl
