builddir    := "build"
shaders     := justfile_directory() / "My Projects/OpenTribes/game/shaders"
gamedata    := env_var_or_default("TRIBES2_DIR", home_directory() + "/.wine/drive_c/Dynamix/Tribes2/GameData")
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

# A dedicated server on our own engine
opentribes-dedicated config="debug" telnet_port="2326" mission="Equinox" game_port="28010": (install config)
    #!/usr/bin/env bash
    # The port is the server prefs', so a one-line file execs those and then
    # says otherwise -- which keeps off 28000, the retail oracle container's.
    set -euo pipefail
    printf 'exec("prefs/serverPrefs.cs");\n$Host::Port = %s;\n' "{{game_port}}" \
        > "{{gamedata}}/base/prefs/serverPrefs-just.cs"
    PYTHONPATH="{{justfile_directory()}}/scripts" \
    exec python -m launch --telnet-port "{{telnet_port}}" --dedicated opentribes \
        -- -serverprefs "prefs/serverPrefs-just.cs" \
           -mission "{{mission}}" "CTF"

[positional-arguments]
telnet telnet_port *commands:
    PYTHONPATH="{{justfile_directory()}}/scripts" \
    python -m probe "$@"
