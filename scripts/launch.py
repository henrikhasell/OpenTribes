#!/usr/bin/env python3
import argparse
import os
import re
import shlex
import signal
import socket
import subprocess
import sys
import time
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
    if address := os.environ.get("TRIBES2_LOCAL_IP"):
        return address
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.connect(("1.1.1.1", 80))
        return s.getsockname()[0]


def port_in_use(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            s.bind(("", port))
        except OSError:
            return True
        return False


def delete_dso_files() -> None:
    for root, _, files in os.walk(GAMEDATA):
        for name in files:
            if name.lower().endswith(".dso"):
                path = Path(root) / name
                print("- rm", path, file=sys.stderr, flush=True)
                path.unlink()


def exec(*args) -> None:
    print("+", shlex.join(args), file=sys.stderr, flush=True)
    os.chdir(GAMEDATA)
    os.execv(args[0], args)


# Wine logs the crash dialog's text on the msgbox channel when it opens it:
#   msgbox:MSGBOX_OnInit L"Unhandled exception: c0000005\nAt address: 0066d073\n"
FAULT_RE = re.compile(
    r'msgbox:MSGBOX_OnInit L"Unhandled exception: ([0-9a-fA-F]+)\\nAt address:\s*([0-9a-fA-F]+)'
)


def kill_game(proc: subprocess.Popen) -> None:
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    subprocess.run("pkill -9 -x Tribes2.exe", shell=True)
    subprocess.run("wineserver -k", shell=True)


def run_supervised(args: list[str]) -> int:
    """Run the game as a child, stop it at the first unhandled exception.

    Wine's crash dialog blocks the game until someone closes it, so the
    game is killed as soon as its message box is logged. The crash text is
    echoed to stderr so it can be read without a screenshot.
    """
    print("+", shlex.join(args), file=sys.stderr, flush=True)
    env = dict(os.environ, WINEDEBUG="err+all,+msgbox")
    proc = subprocess.Popen(
        args, cwd=GAMEDATA, env=env, stderr=subprocess.PIPE,
        start_new_session=True,
    )
    for raw in proc.stderr:
        line = raw.decode(errors="replace")
        sys.stderr.write(line)
        sys.stderr.flush()
        if match := FAULT_RE.search(line):
            code, address = match.groups()
            print(
                f"UNHANDLED EXCEPTION code=0x{code} at address=0x{address}; "
                "killing the game",
                file=sys.stderr, flush=True,
            )
            kill_game(proc)
            return 1
    return proc.wait()


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
        if port_in_use(telnet_port):
            sys.exit(f"telnet port {telnet_port} is already in use")
        argv += [
            "-telnetParams", str(telnet_port), TELNET_RW, TELNET_RO
        ]

    argv += [
        "-mod", "Classic"
    ]

    delete_dso_files()
    if mode == "retail":
        sys.exit(run_supervised(argv))
    exec(*argv)


def restart_clients():
    subprocess.run("pkill -9 -x Tribes2.exe", shell=True)
    subprocess.run("pkill -9 -x OpenTribes", shell=True)
    time.sleep(4)
    for cmd in ('just retail 2327', 'just opentribes release 2326'):
        subprocess.Popen(f'cd {REPO} && {cmd} > {LOG}/{cmd.split()[1]}.log 2>&1',
                         shell=True, start_new_session=True)
        time.sleep(2)
    for _ in range(100):
        out = subprocess.run("ss -ltn", shell=True, capture_output=True, text=True).stdout
        if ":2326" in out and ":2327" in out:
            break
        time.sleep(3)
    time.sleep(10)


def main() -> None:
    parser = argparse.ArgumentParser(description="Launch Tribes 2 from the GameData directory.")
    parser.add_argument("mode", choices=["opentribes", "retail"])
    parser.add_argument("--dedicated", action="store_true")
    parser.add_argument("--telnet-port", type=int)
    parser.add_argument(
        "--connect", nargs="?", const="local", metavar="HOST[:PORT]",
        help=f"join a server on start; with no address, the dedicated server on this machine "
             f"at $TRIBES2_LOCAL_IP, or the address that routes to the internet (port {GAME_PORT} by default)",
    )
    args = parser.parse_args()

    if args.dedicated and args.connect:
        parser.error("a dedicated server can't --connect")

    connect_to = local_ip() if args.connect == "local" else args.connect
    launch(args.mode, args.telnet_port, connect_to, args.dedicated)


if __name__ == "__main__":
    main()
