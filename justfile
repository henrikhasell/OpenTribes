builddir    := "build"
shaders     := justfile_directory() / "My Projects/OpenTribes/game/shaders"
gamedata    := env_var_or_default("TRIBES2_DIR", home_directory() + "/.wine/drive_c/Dynamix/Tribes2/GameData")
telnet_port := "2324"
telnet_rw   := "rw"
telnet_ro   := "ro"

default: release

build config="release":
    cmake -S . -B {{builddir}}/{{config}} -G Ninja -DCMAKE_BUILD_TYPE={{capitalize(config)}}
    cmake --build {{builddir}}/{{config}}

debug: (build "debug")

release: (build "release")

opentribes config="debug" telnet_port=telnet_port: (build config)
    cp "{{justfile_directory() / builddir / config / "game"}}/OpenTribes" "{{gamedata}}/OpenTribes"
    ln -sfn "{{shaders}}" "{{gamedata}}/shaders"
    cd "{{gamedata}}" && ./OpenTribes -nologin -telnetParams {{telnet_port}} '{{telnet_rw}}' '{{telnet_ro}}' -mod Classic

stock telnet_port=telnet_port:
    cd "{{gamedata}}" && /usr/bin/wine Tribes2.exe -nologin -telnetParams {{telnet_port}} '{{telnet_rw}}' '{{telnet_ro}}' -mod Classic

dedicated telnet_port=telnet_port:
    cd "{{gamedata}}" && /usr/bin/wine Tribes2.exe -nologin -dedicated -telnetParams {{telnet_port}} '{{telnet_rw}}' '{{telnet_ro}}' -mod Classic

telnet telnet_port=telnet_port:
    { printf '%s\n' '{{telnet_rw}}'; cat; } | socat - TCP:127.0.0.1:{{telnet_port}},crnl
