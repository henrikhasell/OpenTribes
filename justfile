builddir    := "build"
shaders     := justfile_directory() / "My Projects/OpenTribes/game/shaders"
gamedata    := env_var_or_default("TRIBES2_DIR", home_directory() + "/.wine/drive_c/Dynamix/Tribes2/GameData")
telnet_rw   := "rw"
telnet_ro   := "ro"

default: build

build config="release":
    cmake -S . -B {{builddir}}/{{config}} -G Ninja -DCMAKE_BUILD_TYPE={{capitalize(config)}}
    cmake --build {{builddir}}/{{config}}

install config="release": (build config)
    cp "{{justfile_directory() / builddir / config / "game"}}/OpenTribes" "{{gamedata}}/OpenTribes"
    ln -sfn "{{shaders}}" "{{gamedata}}/shaders"

opentribes config="debug" telnet_port="2323": (install config)
    PYTHONPATH="{{justfile_directory()}}/scripts" \
    python -m launch --telnet-port "{{telnet_port}}" --connect local opentribes

retail telnet_port="2324":
    PYTHONPATH="{{justfile_directory()}}/scripts" \
    python -m launch --telnet-port "{{telnet_port}}" --connect local retail

dedicated telnet_port="2325":
    PYTHONPATH="{{justfile_directory()}}/scripts" \
    python -m launch --telnet-port "{{telnet_port}}" --dedicated retail

telnet telnet_port:
    { printf '%s\n' '{{telnet_rw}}'; cat; } | socat - TCP:127.0.0.1:{{telnet_port}},crnl
