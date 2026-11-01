#!/usr/bin/env python3
import argparse
import os
import shlex
import socket
import sys
from pathlib import Path
from typing import Literal, Optional

GAMEDATA = Path(
    os.environ.get("TRIBES2_DIR")
    or Path.home() / ".wine/drive_c/Dynamix/Tribes2/GameData"
)

TELNET_RW = "rw"
TELNET_RO = "ro"
GAME_PORT = 28000


def local_ip() -> str:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.connect(("1.1.1.1", 80))
        return s.getsockname()[0]


def exec(*args) -> None:
    print("+", shlex.join(args), file=sys.stderr, flush=True)
    os.chdir(GAMEDATA)
    os.execv(args[0], args)


def launch(
    mode: Literal["retail", "opentribes"],
    telnet_port: Optional[int] = None,
    connect_to: Optional[str] = None,
    dedicated: bool = False,
) -> None:
    if dedicated and connect_to:
        raise ValueError("a dedicated server can't connect to another server")

    if mode == "opentribes":
        argv = ["./OpenTribes"]
    else:
        argv = ["/usr/bin/wine", "Tribes2.exe"]

    argv += ["-nologin"]

    if dedicated:
        argv += ["-dedicated"]

    if connect_to:
        if ":" not in connect_to:
            connect_to = f"{connect_to}:{GAME_PORT}"
        argv += ["-connect", f"IP:{connect_to}"]

    if telnet_port:
        argv += [
            "-telnetParams", str(telnet_port), TELNET_RW, TELNET_RO
        ]

    argv += [
        "-mod", "Classic"
    ]

    exec(*argv)


def main() -> None:
    parser = argparse.ArgumentParser(description="Launch Tribes 2 from the GameData directory.")
    parser.add_argument("mode", choices=["opentribes", "retail"])
    parser.add_argument("--dedicated", action="store_true")
    parser.add_argument("--telnet-port", type=int)
    parser.add_argument(
        "--connect", nargs="?", const="local", metavar="HOST[:PORT]",
        help=f"join a server on start; with no address, the dedicated server on this machine (port {GAME_PORT} by default)",
    )
    args = parser.parse_args()

    if args.dedicated and args.connect:
        parser.error("a dedicated server can't --connect")

    connect_to = local_ip() if args.connect == "local" else args.connect
    launch(args.mode, args.telnet_port, connect_to, args.dedicated)


if __name__ == "__main__":
    main()
